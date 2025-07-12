#include "alloc.h"
#include "macros.h"
#include "find.h"

void *find_list(size_t block_size)
{
    int block_num = min_seglist_block(block_size);
    void *block_ptr;

    /**
     * block num is the first block where m > n
     * if we can't find a block in the list, try next larger class (in slides)
     */
    for(; block_num < NUM_FREE_LISTS; block_num++)
    {
        block_ptr = FREE_LST_HEAD_NEXT(block_num);

        while(block_ptr != free_list_heads + block_num)
        {
            //Compare free block size with needed block size
            //Free block size already takes into account header and footer
            size_t fb_size = GET_BLOCKSIZE(block_ptr);

            if(block_size <= fb_size) return (void *)block_ptr;
            block_ptr = ((block *)block_ptr)->body.links.next;
        }
    }

    return NULL;
}

void *find_quick_list(size_t block_size)
{
    if(block_size > (MIN_SIZE + (NUM_QUICK_LISTS - 1) * 16))
        return NULL; //too big for quick list

    int ql_index = (block_size - MIN_SIZE) / 16;
    if (quick_lists[ql_index].length == 0) return NULL;

    block *b = quick_lists[ql_index].first;

    quick_lists[ql_index].first = GET_NEXT(b);
    quick_lists[ql_index].length--;

    b->header = ((b->header) & ~IN_QUICK_LIST);
    PUT2W(FTRP_HEADER(b), b->header); //footer

    return b;
}