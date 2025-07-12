#ifndef ALLOC_H
#define ALLOC_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define THIS_BLOCK_ALLOCATED  0x1
#define IN_QUICK_LIST         0x2

typedef size_t header;
typedef struct block {
    header header;
    union {
        /* A free block contains links to other blocks in a free list. */
        struct {
            struct block *next;
            struct block *prev;
        } links;
        /* An allocated block contains a payload (aligned), starting here. */
        char payload[0];   // Length varies according to block size.
    } body;
} block;


#define NUM_QUICK_LISTS 12  /* Number of quick lists. */
#define QUICK_LIST_MAX   5  /* Maximum number of blocks permitted on a single quick list. */
#define NUM_FREE_LISTS 12

extern size_t current_payload;
struct block free_list_heads[NUM_FREE_LISTS];

struct {
    int length;             // Number of blocks currently in the list.
    struct block *first;    // Pointer to first block in the list.
} quick_lists[NUM_QUICK_LISTS];


/*
 * @param size The number of bytes requested to be allocated.
 *
 * @return If size is 0, then NULL is returned
 * If size is nonzero, then if the allocation is successful a pointer to a valid region of
 * memory of the requested size is returned.  If the allocation is not successful, then
 * NULL is returned and errno is set to ENOMEM.
 */
void *alloc(size_t size);


/*
 * Resizes the memory pointed to by ptr to size bytes.
 *
 * @param ptr Address of the memory region to resize.
 * @param rsize The minimum size to resize the memory to.
 *
 * @return If successful, the pointer to a valid region of memory is
 * returned, else NULL is returned and errno is set appropriately.
 *
 *   If realloc is called with an invalid pointer errno should be set to EINVAL.
 *   If there is no memory available realloc should set errno to ENOMEM.
 *
 * If realloc is called with a valid pointer and a size of 0 it should free
 * the allocated block and return NULL without setting errno.
 */
void *reallocate(void *ptr, size_t size);




/**
 * Wrapper for sf_mem_grow
 * updates the epilogue
 * coalesces with adjacent free blocks if possible.
 * 
 * @return Pointer to the coalesced free block, NULL if no more mem
 */
void *grow_heap();


int validate_free_ptr(void *pp);

#endif
