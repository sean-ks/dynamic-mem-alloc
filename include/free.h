#ifndef FREE_H
#define FREE_H


/**
 * Coalesces a free block with any adjacent free blocks
 * 
 * Handles four cases:
 * 1. Both previous and next blocks are allocated
 * 2. Previous block is allocated, next block is free
 * 3. Previous block is free, next block is allocated
 * 4. Both previous and next blocks are free
 * 
 * Definition mostly taken from CS:APP txtbook
 * 
 * @param block_ptr Pointer to the header of the block to coalesce
 * @return Pointer to the header of the resulting coalesced block
 */
void *coalesce(void *block_ptr);


/*
 * Marks a dynamically allocated region as no longer in use.
 * Adds the newly freed block to the free list.
 *
 * @param ptr Address of memory returned by the function alloc.
 *
  * If ptr is invalid, the function calls abort() to exit the program.
 */
void freemem(void *ptr);

#endif
