#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

#include "allocator.h"

#ifndef BLOCK_COUNT
#define BLOCK_COUNT 128
#endif

struct Allocator
{
    size_t size;
    unsigned int *memory;
};

Allocator *allocator_create(void *const memory, const size_t size)
{
    if (memory == NULL || size < sizeof(Allocator))
        return NULL;
    Allocator *list_alloc = (Allocator *)memory;
    list_alloc->size = BLOCK_COUNT; // 128 4 byte blocks
    if (list_alloc->size % 2 == 1)
        list_alloc->size++;

    list_alloc->memory =
        mmap(NULL, list_alloc->size * sizeof(unsigned int), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (list_alloc->memory == MAP_FAILED)
        return NULL;
    *list_alloc->memory = list_alloc->size;
    *(list_alloc->memory + list_alloc->size - 1) = list_alloc->size;
    return list_alloc;
}

void allocator_destroy(Allocator *const allocator)
{
    munmap(allocator->memory, allocator->size * sizeof(unsigned int));
}

void *allocator_alloc(Allocator *const allocator, const size_t size)
{
    size_t size_in_blocks = (size + 3) / 4;
    size_t full_size = size_in_blocks + 2;
    unsigned int *end = allocator->memory + allocator->size;

    size_t diff = -1;
    unsigned int *ptr = NULL;

    for (unsigned int *p = allocator->memory; p < end; p = p + (*p & ~1))
    {
        if (*p & 1)
            continue;
        if (*p < full_size) // we need memory for headers
            continue;

        if (size - *p < diff)
        {
            diff = size - *p;
            ptr = p;
        }
    }
    if (!ptr)
        return NULL;

    size_t newsize =
        full_size % 2 == 0 ? full_size : full_size + 1; // round to even bytes

    size_t oldsize = *ptr & ~1;
    *ptr = newsize | 1;          // front header
    *(ptr + newsize - 1) = *ptr; // back header

    if (newsize < oldsize)
    {
        *(ptr + newsize) = oldsize - newsize;
        *(ptr + oldsize - 1) = oldsize - newsize;
    }
    return ptr + 1;
}

void allocator_free(Allocator *const allocator, void *const memory)
{
    (void)allocator;

    unsigned int *ptr = (unsigned int *)memory - 1;
    *ptr = *ptr & ~1;

    unsigned int *front = ptr;
    unsigned int *back = ptr + *ptr - 1;
    size_t size = *ptr;

    unsigned int *next = ptr + *ptr;
    if ((*next & 1) == 0)
    {
        back += *next;
        size += *next;
    }

    if (ptr > allocator->memory)
    {
        unsigned int *prev_back = ptr - 1;
        if (((*prev_back) & 1) == 0)
        {
            front -= *prev_back;
            size += *prev_back;
        }
    }
    *front = size;
    *back = size;
}