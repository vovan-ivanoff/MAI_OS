#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "allocator.h"

#ifndef BLOCK_COUNT
#define BLOCK_COUNT 128
#endif

typedef struct Block Block;
struct Block
{
    bool used;
    unsigned int path;
    unsigned char size_class;
    Block *next;
    Block *prev;
};

#define MIN_SIZE_CLASS ((size_t)ceil(log2(sizeof(Block) / sizeof(unsigned int) + 1)))
#define MAX_SIZE_CLASS ((size_t)log2(BLOCK_COUNT))

struct Allocator
{
    size_t size;
    char *mem;
    Block *free;
};

Block *buddy(Block *p)
{
    unsigned int *ptr = (unsigned int *)p;
    if (p->path & (1 << p->size_class))
        ptr -= (1 << p->size_class);
    else
        ptr += (1 << p->size_class);
    return (Block *)ptr;
}

Allocator *allocator_create(void *const memory, const size_t size)
{
    if (memory == NULL || size < sizeof(Allocator))
        return NULL;
    Allocator *buddy_alloc = (Allocator *)memory;
    buddy_alloc->size = 1 << MAX_SIZE_CLASS; // 128 4 byte blocks
    buddy_alloc->mem = mmap(NULL, buddy_alloc->size * sizeof(unsigned int), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (buddy_alloc->mem == MAP_FAILED)
        return NULL;

    buddy_alloc->free = (Block *)buddy_alloc->mem;
    buddy_alloc->free->used = false;
    buddy_alloc->free->size_class = MAX_SIZE_CLASS;
    buddy_alloc->free->next = NULL;
    buddy_alloc->free->prev = NULL;
    buddy_alloc->free->path = 0;
    return buddy_alloc;
}

void allocator_destroy(Allocator *const allocator)
{
    munmap(allocator->mem, allocator->size * sizeof(unsigned int));
}

void *allocator_alloc(Allocator *const buddy_alloc, const size_t size)
{
    if (buddy_alloc == 0 || size == 0)
        return NULL;
    size_t size_in_u32 = (size + sizeof(Block) + 3) / 4;
    size_t size_class = ceil(log2(size_in_u32));
    if (size_class > MAX_SIZE_CLASS)
        return NULL;
    Block *p = buddy_alloc->free;
    for (Block *c = buddy_alloc->free; c; c = c->next)
    {

        if (c->used || (unsigned char)c->size_class < size_class)
            continue;
        if (p->used)
        {
            p = c;
            continue;
        }
        if (c->size_class < p->size_class)
            p = c;
    }
    if (p == NULL || p->used)
        return NULL;

    if (buddy_alloc->free == p)
        buddy_alloc->free = p->next;
    if (p->next)
        p->next->prev = p->prev;
    if (p->prev)
        p->prev->next = p->next;

    while (p->size_class > size_class)
    {
        p->size_class = p->size_class - 1;
        Block *other = buddy(p);
        other->size_class = p->size_class;
        other->next = buddy_alloc->free;
        other->prev = NULL;
        other->used = false;
        other->path = p->path | 1 << p->size_class;
        if (buddy_alloc->free)
            buddy_alloc->free->prev = other;
        buddy_alloc->free = other;
    }
    p->used = true;
    return (char *)p + sizeof(Block);
}

void allocator_free(Allocator *const allocator, void *const memory)
{
    if (!allocator || !memory)
        return;
    Block *p = (Block *)((char *)memory - sizeof(Block));
    while (p->size_class < MAX_SIZE_CLASS)
    {
        Block *bud = buddy(p);
        if (bud->used)
            break;
        if (bud->prev)
            bud->prev->next = bud->next;
        if (bud->next)
            bud->next->prev = bud->prev;
        if (allocator->free == bud)
            allocator->free = bud->next;
        p = p->path & (1 << p->size_class) ? bud : p;
        p->size_class++;
    }
    p->used = false;
    p->next = allocator->free;
    p->prev = NULL;
    allocator->free = p;
}