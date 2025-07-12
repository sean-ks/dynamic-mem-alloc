#include "free.h"
#include "macros.h"
#include "alloc.h"
#include "seglist.h"

void *coalesce(void *block_ptr)
{
    size_t prev_alloc = (*((header *)((char *)block_ptr - DSIZE))) & THIS_BLOCK_ALLOCATED;
    size_t size = GET_BLOCKSIZE(block_ptr);
    size_t next_alloc = (*((header *)((char *)block_ptr + size))) & THIS_BLOCK_ALLOCATED;

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