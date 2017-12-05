/* Technically a memory allocator. */

#include <stddef.h>
#include <badmalloc.h>

extern size_t *kernel_end;

void *badmalloc(size_t size) {
    static size_t *ptr = 0;

    if (ptr == 0) {
        ptr = kernel_end;
    }

    ptr += size;

    return (ptr - size);
}