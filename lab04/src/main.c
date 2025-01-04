
#include <stdint.h>
#include <dlfcn.h>
#include <unistd.h>

#include <string.h>

#include <sys/mman.h>

#include "allocator.h"


static allocator_create_func *allocator_create;
static allocator_destroy_func *allocator_destroy;
static allocator_alloc_func *allocator_alloc;
static allocator_free_func *allocator_free;

struct Allocator
{
    size_t size;
    uint32_t *memory;
};

Allocator *allocator_create_def(void *const memory, const size_t size)
{
    (void)size;
    (void)memory;
    return NULL;
}

void allocator_destroy_def(Allocator *const allocator)
{
    (void)allocator;
}

void *allocator_alloc_def(Allocator *const allocator, const size_t size)
{
    (void)allocator;
    uint32_t *memory = mmap(NULL, size + 1, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED)
    {
        return NULL;
    }
    *memory = size + 1;
    return memory + 1;
}

void allocator_free_def(Allocator *const allocator, void *const memory)
{
    (void)allocator;
    if (memory == NULL)
        return;
    uint32_t *mem = memory;
    mem--;
    munmap(mem, *mem);
}

#define LOAD_FUNCTION(name)                                                    \
    {                                                                          \
        name = dlsym(library, #name);                                          \
        if (allocator_create == NULL)                                          \
        {                                                                      \
            char message[] =                                                   \
                "WARNING: failed to find " #name " function implementation\n"; \
            write(STDERR_FILENO, message, strlen(message));                    \
            name = name##_def;                                                 \
        }                                                                      \
    }

void load_dynamic(const char *path)
{
    void *library = dlopen(path, RTLD_LOCAL | RTLD_NOW);
    if (path && library)
    {
        LOAD_FUNCTION(allocator_create);
        LOAD_FUNCTION(allocator_destroy);
        LOAD_FUNCTION(allocator_alloc);
        LOAD_FUNCTION(allocator_free);
    }
    else
    {
        char *message = "failed to load shared library. Using mmap implementation\n";
        write(STDERR_FILENO, message, strlen(message));
        allocator_create = allocator_create_def;
        allocator_destroy = allocator_destroy_def;
        allocator_alloc = allocator_alloc_def;
        allocator_free = allocator_free_def;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        write(STDERR_FILENO, "No path is Given!: ", 19);
        load_dynamic(NULL); // load default implementation
    }
    else {
        load_dynamic(argv[1]);
    }
    char mem[1024];
    Allocator *memAllocator = allocator_create(mem, sizeof(mem));
    char *a = allocator_alloc(memAllocator, 12 * sizeof(char));
    char *b = allocator_alloc(memAllocator, 12 * sizeof(char));
    for (int i = 0; i < 10; i++)
    {
        a[i] = '0' + i;
        b[10 - i - 1] = '0' + i;
    }
    b[10] = 0;
    a[10] = 0;

    write(STDOUT_FILENO, "contents of a: ", 15);
    write(STDOUT_FILENO, a, strlen(a));
    write(STDOUT_FILENO, "\n", 1);
    write(STDOUT_FILENO, "contents of b: ", 15);
    write(STDOUT_FILENO, b, strlen(b));
    write(STDOUT_FILENO, "\n", 1);
    strcpy(a, b);
    write(STDOUT_FILENO, "contents of a after copy from b: ", 33);
    write(STDOUT_FILENO, b, strlen(b));
    write(STDOUT_FILENO, "\n", 1);
    allocator_free(memAllocator, a);
    allocator_free(memAllocator, b);
    allocator_destroy(memAllocator);
}