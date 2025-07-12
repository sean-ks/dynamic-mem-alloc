#ifndef MACROS_H
#define MACROS_H

/*Macros and Definitions From the CS:APP textbook*/
#define WSIZE       4           /* Word and header/footer size (bytes) */
#define DSIZE       8           /* Double word size (bytes)            */
#define MIN_SIZE    32          /* min block size                      */
#define PAGE_SIZE   (1 << 12)   /* Size                                */
#define ALIGNMENT_POINTERS 16   /* Alignment in 16 bytes/double        */ 


#define MAX(x, y) ((x) > (y)? (x) : (y))


/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))
#define ALLOC_PACK(payload, block) (((uint64_t)(payload) << 32) | (block) | (THIS_BLOCK_ALLOCATED))


/* Read and write a word at address p */
#define GET(p)        (*(unsigned int *) (p)) /* cast is needed */
#define PUT(p, val)   (*(unsigned int *)(p) = (val))


/* Double Word */
#define GET2W(p)       ((char *) *(unsigned int **)(p))
#define PUT2W(p, val)  (*(unsigned int **)(p) = (unsigned int *)(val))


/* Read the block size from address p*/
#define GET_BLOCKSIZE(p)  ((((block *)p)->header) & 0x0FFFFFFF0)
#define GET_PAYLOAD(p)    ((((block *)p)->header) >> MIN_SIZE)


/* Get next and prev free block from header */
#define GET_NEXT(ptr) (((block *)ptr)->body.links.next)
#define GET_PREV(ptr) (((block *)ptr)->body.links.prev)


/* assume block pointer is pointing to payload*/
#define HDRP(bp) ((char *)(bp) - DSIZE)
#define FTRP(bp) ((char *)(bp) + GET_BLOCKSIZE(HDRP(bp)) - 2*DSIZE)
#define FTRP_HEADER(bp) ((char *)(bp) + GET_BLOCKSIZE(bp) - DSIZE)


/* Adjust block size for alignment */
#define ALIGN(size) (ALIGNMENT_POINTERS * ((size + ALIGNMENT_POINTERS + (ALIGNMENT_POINTERS - 1)) / ALIGNMENT_POINTERS))


#define SET_QUICK(ptr) ((ptr)->header = (((ptr)->header) | IN_QUICK_LIST) )
#define SET_ALLOC(ptr) ((ptr)->header = (((ptr)->header) | THIS_BLOCK_ALLOCATED) )


/* Shorthand for getting next in free list header */
#define FREE_LST_HEAD_NEXT(block_num) (free_list_heads[block_num].body.links.next)
#define FREE_LST_HEAD_PREV(block_num) (free_list_heads[block_num].body.links.prev)

#endif
