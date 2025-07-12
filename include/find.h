#ifndef FIND_H
#define FIND_H

#include "seglist.h"

/**
 * Find the a fit for a specified block_size using a first-fit search
 * @param block_size Number of bytes requested to find fit
 *
 * @return Address for the block if found. NULL otw
 */
void *find_list(size_t block_size);

/**
 * Searches the quick lists for a block of the requested size
 *
 * @param block_size Size of block needed
 * @return Pointer to a suitable block from quick lists, NULL if none found
 */
void *find_quick_list(size_t block_size);

#endif
