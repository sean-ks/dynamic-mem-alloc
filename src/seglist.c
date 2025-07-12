#include "seglist.h"
#include "macros.h"
#include "alloc.h"


int min_seglist_block(size_t block_size)
{
    if (block_size == MIN_SIZE) return 0;

    size_t new_size = (block_size + MIN_SIZE - 1) / MIN_SIZE; // Ceiling division
    new_size -= 1;

    int i = 0;
    while (new_size > 0)
    {
        new_size >>= 1;
        i++;
        if(i >= NUM_FREE_LISTS - 1) break;
    }

    return i;
}


void add_to_seglist(void *free_ptr)
{
    int seglist_index = min_seglist_block(GET_BLOCKSIZE(free_ptr));

    // no need to check if list is first or not
    // this implementation works for both cases I believe
    GET_NEXT(free_ptr) = FREE_LST_HEAD_NEXT(seglist_index);
    GET_PREV(free_ptr) = free_list_heads + seglist_index;
    GET_PREV(FREE_LST_HEAD_NEXT(seglist_index)) = free_ptr;
    FREE_LST_HEAD_NEXT(seglist_index) = free_ptr;
}


void remove_from_seglist(void *free_ptr)
{
    block *next = GET_NEXT(free_ptr);
    block *prev = GET_PREV(free_ptr);

    prev->body.links.next = next;
    next->body.links.prev = prev;

    GET_NEXT(free_ptr) = NULL;
    GET_PREV(free_ptr) = NULL;
}