#include "alloc.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    void *p = alloc(100);
    printf("Allocated 100 bytes at %p\n", p);
    freemem(p);
    printf("Freed memory at %p\n", p);
    return 0;
}