#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "allocator.h"

static void * std_malloc(size_t size) {
    return size ? malloc(size) : NULL;
}

static void * std_realloc(void * mem, size_t new_size) {
    return realloc(mem, new_size);
}

static void std_free(void * mem) {
    free(mem);
}

struct Chunk {
    struct Chunk * next;
    struct Chunk * prev;
    size_t size;
    size_t capacity;
    void * memory;
};

struct StackAllocator {
    struct Chunk * first;
    struct Chunk * last;
};

#define CHUNK_MIN_SIZE (16 * 1024)

static struct Chunk * chunk_create(size_t size) {
    struct Chunk * c;
    c = std_malloc(sizeof(struct Chunk) + size);
    if (!c) return 0;
    c->next = NULL;
    c->prev = NULL;
    c->size = 0;
    c->capacity = size;
    c->memory = c + 1;
    return c;
}

static void chunk_free(struct Chunk * c) { std_free(c); }

static int stack_new_chunk(struct StackAllocator * a, size_t size) {
    struct Chunk * new_chunk;
    size = size < CHUNK_MIN_SIZE ? CHUNK_MIN_SIZE : size;
    new_chunk = chunk_create(size);
    if (!new_chunk) return 0;
    if (a->last) {
        a->last->next = new_chunk;
        new_chunk->prev = a->last;
        a->last = new_chunk;
    } else {
        a->last = a->first = new_chunk;
    }
    return 1;
}

static int stack_ensure_has_free_space(struct StackAllocator * a, size_t size) {
    if (a->last && a->last->capacity - a->last->size > size) return 1;
    return stack_new_chunk(a, size);
}

static void * stack_malloc(struct StackAllocator * a, size_t size) {
    void * allocated;
    if (!stack_ensure_has_free_space(a, size)) return NULL;
    allocated = (char *)a->last->memory + a->last->size;
    a->last->size += size;
    return allocated;
}

static void * stack_realloc(struct StackAllocator * a,
        void * mem, size_t old_size, size_t new_size) {
    assert(a->last);
    /* Mem must be on top of stack */
    assert((char *)mem + old_size == (char *)a->last->memory + a->last->size);
    /* If is shrinking */
    if (old_size >= new_size) {
        a->last->size -= old_size - new_size;
        return mem;
    /* If last chunk is big enough to fit new size */
    } else if (a->last->capacity - a->last->size >= new_size - old_size) {
        a->last->size += new_size - old_size;
        return mem;
    } else {
        if (!stack_new_chunk(a, new_size)) return NULL;
        memcpy(a->last->memory, mem, old_size);
        a->last->size += new_size;
        return a->last->memory;
    }
}

/* Stack allocator can only free memory in order reverse to order of allocation. */
static void stack_free(struct StackAllocator * a, void * mem, size_t size) {
    assert((char *)mem + size == (char *)a->last->memory + a->last->size);
    (void) mem;
    a->last->size -= size;
}

extern void * alloc_malloc(struct allocAllocator * a, size_t size) {
    if (!size) return NULL;
    if (!a) return std_malloc(size);
    else return stack_malloc((struct StackAllocator *)a, size);
}

extern void * alloc_realloc(struct allocAllocator * a,
        void * mem, size_t old_size, size_t new_size) {
    if (!a) return std_realloc(mem, new_size);
    else return stack_realloc((struct StackAllocator *)a, mem, old_size, new_size);
}

extern void alloc_free(struct allocAllocator * a, void * mem, size_t size) {
    assert(mem);
    if (!a) std_free(mem);
    else stack_free((struct StackAllocator *)a, mem, size);
}

extern struct allocAllocator * get_stack_allocator() {
    struct StackAllocator * a;
    a = std_malloc(sizeof(struct StackAllocator));
    if (!a) return NULL;
    a->first = NULL;
    a->last = NULL;
    return (struct allocAllocator *)a;
}

static void free_stack_allocator(struct StackAllocator * a) {
    struct Chunk * chunk;
    struct Chunk * next;
    chunk = a->first;
    while (chunk) {
        next = chunk->next;
        chunk_free(chunk);
        chunk = next;
    }
    std_free(a);
}

extern struct allocAllocator * alloc_create_allocator(enum allocKind kind) {
    switch (kind) {
        case AK_STD:
            return NULL;
        case AK_STACK:
            return get_stack_allocator();
    }
    assert(0);
    return NULL;
}

extern void alloc_free_allocator(enum allocKind kind, struct allocAllocator * a) {
    switch (kind) {
        case AK_STD:
            /* Correct implementation must free all allocated memory here.
               But we will not be using this allocator though interface so far.
               This variant exist for memory issue profiling using valgrind
               as this is so much easier to detect memory access errors
               when memory is allocated with malloc but not by chunks.
               TODO: implement simple object pool
               */
            break;
        case AK_STACK:
            free_stack_allocator((struct StackAllocator *)a);
            break;
    }
}

