#include "alloc.h"
#include "macros.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    void *p = alloc(100);
    size_t size_p = GET_BLOCKSIZE(HDRP(p));
    printf("Allocated %zu bytes at %p\n", size_p, p);

    void *q = alloc(16284);
    size_t size_q = GET_BLOCKSIZE(HDRP(q));
    printf("Allocated %zu bytes at %p\n", size_q, q);

    freemem(p);
    printf("Freed memory at %p\n", p);
    freemem(q);
    printf("Freed memory at %p\n", q);

    p = alloc(10);
    size_t size_r = GET_BLOCKSIZE(HDRP(p));
    printf("Allocated %zu bytes at %p\n", size_r, p);

    return 0;
}