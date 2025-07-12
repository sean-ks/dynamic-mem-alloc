#include "alloc.h"
#include "find.h"
#include "macros.h"
#include "free.h"
#include "seglist.h"
#include <errno.h>


/* global variables */
static void *list_p           = 0;  /* Pointer to first block */
static void *mem_brk          = 0;  /* Points to last byte of heap */
static size_t max_payload     = 0;
size_t current_payload = 0;


/**
 * Initializes the memory allocator
 * 
 * Creates the initial heap, sets up the prologue and epilogue blocks,
 * initializes the segregated free lists and quick lists, and creates the 
 * first free block.
 * 
 * @return 0 on success, -1 on failure
 */
int mm_init(void)
{
    if((list_p = sbrk(PAGE_SIZE)) == (void *)-1){errno = ENOMEM; return -1;}

    //initialize heads as circular doubly linked lists
    for(int i = 0; i < NUM_FREE_LISTS; i++)
    {
        FREE_LST_HEAD_NEXT(i) = free_list_heads + i;
        FREE_LST_HEAD_PREV(i) = free_list_heads + i;
    }

    //initialize quick lists
    for(int i = 0; i < NUM_QUICK_LISTS; i++)
    {
        quick_lists[i].length = 0;
        quick_lists[i].first = NULL;
    }

    list_p = (char *)list_p + DSIZE; //unused for alignment
    PUT(list_p, PACK(MIN_SIZE, 1));
    PUT(FTRP_HEADER(list_p), PACK(MIN_SIZE, 1));

    mem_brk = sbrk(0);

    PUT2W((char *)mem_brk - DSIZE, PACK(0, 1));
    list_p += MIN_SIZE; //first block

    /* free block header */
    //size of free block is last address - first address - epilogue
    size_t free_block_size = (char *)mem_brk - (char *)list_p - DSIZE;

    //Add free block to seglist
    int block_num = min_seglist_block(ALIGN(free_block_size));

    ((block *)list_p)->header = PACK(free_block_size, 0); //header
    PUT2W(FTRP_HEADER(list_p), PACK(free_block_size, 0)); //footers

    GET_NEXT(list_p) = free_list_heads + block_num;
    GET_PREV(list_p) = free_list_heads + block_num;
    FREE_LST_HEAD_PREV(block_num) = list_p;
    FREE_LST_HEAD_NEXT(block_num) = list_p;
    return 0;
}


void *alloc(size_t size)
{
    if (list_p == 0){
        if(mm_init() == -1){
            errno = ENOMEM;
            return NULL;
        }
    }

    if (size == 0) return NULL;

    size_t block_size;

    /*
    Block size must have:
        - at least 32 bytes
        - 8 byte header sizes, + payload size (size) + padding for alignment + 8 byte footer size
    */
    block_size = ALIGN(size);
    if(block_size < MIN_SIZE) return NULL;

    void *block_ptr;

    if((block_ptr = find_quick_list(block_size)) != NULL)
    {
        PUT2W(block_ptr, (ALLOC_PACK(size, block_size) & ~IN_QUICK_LIST));
        PUT2W(FTRP_HEADER(block_ptr), (ALLOC_PACK(size, block_size) & ~IN_QUICK_LIST));
        current_payload += size;
        if(current_payload > max_payload) max_payload = current_payload;
        return (char *)block_ptr + DSIZE; //block_ptr points to header, return pointer to payload
    }
    
    block_ptr = find_list(block_size);
    if(block_ptr != NULL)
    {
        allocate_block(block_ptr, block_size, size);
        current_payload += size;
        if(current_payload > max_payload) max_payload = current_payload;
        return (char *)block_ptr + DSIZE; //block_ptr points to header, return pointer to payload
    }

    //Heap ran out :()
    //Get more memory
    while(1)
    {
        if((block_ptr = grow_heap()) == NULL)
        {
            errno = ENOMEM; // I know this is already set, but just in case
            return NULL;
        }

        size_t new_block_size = GET_BLOCKSIZE(block_ptr);

        if(new_block_size >= block_size) break;
        //Hopefully this isn't an infinite loop, the check for NULL should be fine...
    }


    allocate_block(block_ptr, block_size, size);
    current_payload += size;
    if(current_payload > max_payload) max_payload = current_payload;
    return (char *)block_ptr + DSIZE;
}


void *reallocate(void *pp, size_t rsize) {
    int valid = validate_free_ptr(pp);
    void *ptr;
    if (valid)
    {
        errno = EINVAL;
        return NULL;
    }
    if (rsize == 0)
    {
        freemem(pp);
        return NULL;
    }

    size_t size = GET_BLOCKSIZE(HDRP(pp));
    size_t payload = GET_PAYLOAD(HDRP(pp));
    if(payload == rsize)
    {
        return pp;
    }
    else if(rsize > payload)
    {
        size_t temp_max_payload = max_payload;
        if((ptr = alloc(rsize)) == NULL)
        {
            errno = ENOMEM;
            return NULL;
        }
        memcpy(ptr, pp, payload);

        freemem(pp);
        max_payload = temp_max_payload;
        if(current_payload > max_payload) max_payload = current_payload;
        return ptr;
    }
    else
    {
        size_t aligned_size = ALIGN(rsize);
        current_payload -= payload - rsize;
        if(current_payload > max_payload) max_payload = current_payload;
        if(size - aligned_size < MIN_SIZE)
        {
            PUT2W(HDRP(pp), ALLOC_PACK(rsize, size));
            PUT2W(FTRP(pp), ALLOC_PACK(rsize, size));
            return pp;
        }
        else
        {
            PUT2W(HDRP(pp), ALLOC_PACK(rsize, aligned_size));
            PUT2W(FTRP(pp), ALLOC_PACK(rsize, aligned_size));

            //free block
            void *free_block = FTRP(pp) + DSIZE;
            PUT2W(free_block, PACK(size - aligned_size, 0));
            PUT2W(FTRP_HEADER(free_block), PACK(size - aligned_size, 0));
            add_to_seglist(coalesce(free_block));
            return pp;
        }
    }
}


void *grow_heap()
{
    void *block_ptr;
    if((block_ptr = sbrk(PAGE_SIZE)) == (void *)-1)
        return NULL;
    void *header_ptr = (char *)block_ptr - DSIZE;
    //make epilogue first
    mem_brk = sbrk(0);

    PUT2W(mem_brk - DSIZE, PACK(0,1));

    size_t free_block_size = (char *)mem_brk - (char *)block_ptr;
    PUT2W(HDRP(block_ptr), PACK(free_block_size, 0)); // header
    PUT2W(FTRP(block_ptr), PACK(free_block_size, 0)); //footer

    void *coalesced = coalesce(header_ptr);
    add_to_seglist(coalesced);
    return coalesced;
}


int validate_free_ptr(void *pp)
{
    if(pp == NULL || pp == 0 || pp == (void *)-1) // null ptr
    {
        debug("NULL");
        return -1;
    }
    if(pp < list_p || pp > mem_brk) //not in heap
    {
        debug("NOT IN HEAP");
        return -1;
    }
    if(((uintptr_t)pp & 0xF) != 0) //ptr not aligned
    {
        debug("not aligned");
        return -1;
    }

    block *block_ptr = (block *)((char *)pp - DSIZE); // already free
    if(!((block_ptr->header) & THIS_BLOCK_ALLOCATED))
    {
        debug("already free");
        return -1;
    }
    if(((block_ptr->header) & IN_QUICK_LIST)) // in quick list
    {
        debug("in quick list");
        return -1;
    }

    header *footer = (header *)(FTRP_HEADER(block_ptr));
    if((*footer) != (block_ptr->header)) //Footer and header don't match
    {
        return -1;
    }

    size_t size = GET_BLOCKSIZE(block_ptr);
    if(size < MIN_SIZE) // Size is less than minimum
    {
        return -1;
    }
    if((size & 0xF) != 0) //Size is not a multiple of 16
    {
        return -1;
    }
    if((char *)footer >= (char *)mem_brk - DSIZE) //Footer is after or on epilogue
    {
        return -1;
    }
    if((char *)block_ptr < (char *)list_p) //block_ptr is on prologue or before it
    {
        return -1;
    }
    return 0;
}