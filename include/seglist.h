#ifndef SEGLIST_H
#define SEGLIST_H

#include <stddef.h> 

/**
 * Determines the min seglist index for a given block size
 * 
 * @param block_size Size of the block
 * @return index of seglist
 */
int min_seglist_block(size_t block_size);

/**
 * Removes a free block from its segregated list
 * 
 * @param free_ptr Pointer to the header of the free block to remove
 */
void remove_from_seglist(void *free_ptr);

/**
 * Adds a free block to the appropriate segregated list
 * 
 * Inserts the block at the beginning of its list
 * 
 * @param free_ptr Pointer to the header of the free block to add
 */
void add_to_seglist(void *free_ptr);

#endif
