#include "alloc.h"
#include "find.h"
#include "macros.h"
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
    if(block_size < MIN_SIZE) block_size = MIN_SIZE;

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
    if(block_ptr == NULL)
    {
        // No fit found. Get more memory and place the block.
        if ((block_ptr = extend_heap(block_size)) == NULL) {
            return NULL;
        }
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


void *extend_heap(size_t size)
{
    void *block_ptr;
    size_t new_size;

    new_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if ((block_ptr = sbrk(new_size)) == (void *)-1) {
        errno = ENOMEM;
        return NULL;
    }

    mem_brk = sbrk(0);


    PUT2W((char *)block_ptr, PACK(new_size, 0)); // header
    PUT2W(FTRP_HEADER((char *)block_ptr), PACK(new_size, 0)); //footer


    PUT2W((char *)mem_brk - DSIZE, PACK(0, 1));


    void *coalesced_block = coalesce(block_ptr);


    add_to_seglist(coalesced_block);
    
    return coalesced_block;
}


int validate_free_ptr(void *pp)
{
    printf("test: %p\n", pp);
    if(pp == NULL || pp == 0 || pp == (void *)-1) // null ptr
    {
        printf("null");
        fflush(NULL);
        return -1;
    }
    if(pp < list_p || pp > mem_brk) //not in heap
    {
        printf("not in heap");
        return -1;
    }
    if(((uintptr_t)pp & (DSIZE - 1)) != 0) //ptr not aligned
    {
        printf("not aligned");
        fflush(NULL);
        return -1;
    }

    block *block_ptr = (block *)((char *)pp - DSIZE); // already free
    if(!((block_ptr->header) & THIS_BLOCK_ALLOCATED))
    {
        printf("already free");
        fflush(NULL);
        return -1;
    }
    if(((block_ptr->header) & IN_QUICK_LIST)) // in quick list
    {
        printf("in ql");
        fflush(NULL);
        return -1;
    }

    header *footer = (header *)(FTRP_HEADER(block_ptr));
    if((*footer) != (block_ptr->header)) //Footer and header don't match
    {
        printf("ftr and hdr dont match");
        fflush(NULL);
        return -1;
    }

    size_t size = GET_BLOCKSIZE(block_ptr);
    if(size < MIN_SIZE) // Size is less than minimum
    {
        printf("size is less than min");
        fflush(NULL);
        return -1;
    }
    if((size & 0xF) != 0) //Size is not a multiple of 16
    {
        printf("size is not mult of 16");
        fflush(NULL);
        return -1;
    }
    if((char *)footer >= (char *)mem_brk - DSIZE) //Footer is after or on epilogue
    {
        printf("footer after epilogue");
        fflush(NULL);
        return -1;
    }
    if((char *)block_ptr < (char *)list_p) //block_ptr is on prologue or before it
    {
        printf("block_ptr is on or b4 prologue");
        fflush(NULL);
        return -1;
    }
    printf("works?");
    fflush(NULL);
    return 0;
}


void allocate_block(void *block_ptr, size_t block_size, size_t payload_size)
{
    size_t fb_size = GET_BLOCKSIZE(block_ptr);
    size_t remainder = fb_size - block_size;
    remove_from_seglist(block_ptr);
    if (remainder >= MIN_SIZE)
    {
        PUT2W(block_ptr, ALLOC_PACK(payload_size, block_size)); //Header for block
        PUT2W((char *)block_ptr + block_size - DSIZE, ALLOC_PACK(payload_size, block_size)); //Footer for block

        /* We don't have to worry about figuring out if there's another allocated block after this
        We have enough remaining to just create a new free block
        */
        void *new_block = (char *)block_ptr + block_size;
        PUT2W(new_block, PACK(remainder, 0)); //Header for free
        PUT2W(FTRP_HEADER(new_block), PACK(remainder, 0)); //Footer for free

        add_to_seglist(new_block);
    }
    else
    {
        PUT2W((char *)block_ptr, ALLOC_PACK(payload_size, fb_size));
        PUT2W(FTRP_HEADER((char *)block_ptr), ALLOC_PACK(payload_size, fb_size));
    }
}

void *coalesce(void *block_ptr)
{
    size_t prev_alloc = (*((header *)((char *)block_ptr - DSIZE))) & THIS_BLOCK_ALLOCATED;
    size_t size = GET_BLOCKSIZE(block_ptr);
    size_t next_alloc = (*((header *)((char *)block_ptr + size - DSIZE))) & THIS_BLOCK_ALLOCATED;
    //Case 1, in between two allocs
    if(prev_alloc && next_alloc) 
        return block_ptr;

    //Case 2, next is free
    else if (prev_alloc && !next_alloc) 
    {
        size_t next_size = (*((header *)((char *)block_ptr + size))) & 0x0FFFFFFF0;
        remove_from_seglist((char *)block_ptr + size);
        
        size += next_size;
        PUT2W(block_ptr, PACK(size, 0)); //header
        PUT2W((char *)block_ptr + size - DSIZE, PACK(size, 0)); //footer
    }

    //Case 3, prev is free
    else if (!prev_alloc && next_alloc) 
    {
        size_t prev_size = (*((header *)((char *)block_ptr - DSIZE))) & 0x0FFFFFFF0;

        remove_from_seglist((char *)block_ptr - prev_size);
        size += prev_size;
        block_ptr = (char *)block_ptr - prev_size;
        PUT2W(block_ptr, PACK(size, 0));
        PUT2W((char *)block_ptr + size - DSIZE, PACK(size, 0));
    }

    //Case 4, both are free
    else 
    {
        size_t next_size = (*((header *)(block_ptr + size))) & 0x0FFFFFFF0;
        size_t prev_size = (*((header *)(block_ptr - DSIZE))) & 0x0FFFFFFF0;
        
        remove_from_seglist((char *)block_ptr + size);
        remove_from_seglist((char *)block_ptr - prev_size);

        size += next_size + prev_size;
        block_ptr = (char *)block_ptr - prev_size;
        PUT2W(block_ptr, PACK(size, 0));
        PUT2W((char *)block_ptr + size - DSIZE, PACK(size, 0));
    }

    return block_ptr;

}

void freemem(void *pp) {
    int valid = validate_free_ptr(pp);
    if(valid) abort();

    block *b = (block *)((char *)pp - DSIZE);
    size_t block_size = GET_BLOCKSIZE(b);
    current_payload -= block_size;

    // Check if block should be added to a quick list
    if (block_size <= (MIN_SIZE + (NUM_QUICK_LISTS - 1) * 16))
    {
        int ql_index = (block_size - MIN_SIZE) / 16;

        // Check if quick list isn't full
        if (quick_lists[ql_index].length < QUICK_LIST_MAX)
        {
            SET_QUICK(b);
            PUT2W(FTRP(pp), b->header); //footer
            GET_NEXT(b) = quick_lists[ql_index].first;

            quick_lists[ql_index].first = b;
            quick_lists[ql_index].length++;
            return;
        }
        else
        {
            //Flush quicklist
            block *current = quick_lists[ql_index].first;
            block *next;

            while(current != NULL)
            {
                next = GET_NEXT(current);
                current->header = ((current->header) & ~(THIS_BLOCK_ALLOCATED | IN_QUICK_LIST ));


                PUT2W(FTRP_HEADER(current), current->header); //footer

                add_to_seglist(coalesce(current));
                current = next;
            }
            // reset
            quick_lists[ql_index].first = NULL;
            quick_lists[ql_index].length = 0;

            /*
                After flushing the quick list,
                the block currently being freed is inserted into the
                now-empty list, leaving just one block in that list.
            */
            SET_QUICK(b);
            PUT2W(FTRP_HEADER(b), b->header); //set quick for footer
            b->body.links.next = NULL;
            quick_lists[ql_index].first = b;
            quick_lists[ql_index].length = 1;
            return;
        }
    }

    //Too big to be in quicklist, add to seglist instead
    b->header = ((b->header) & ~THIS_BLOCK_ALLOCATED);

    PUT2W(FTRP_HEADER(b), b->header); //footer
    add_to_seglist(coalesce(b));
}