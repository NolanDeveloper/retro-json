#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

struct Block {
    struct Block * next;
    struct Block * prev;
    const char *   file;
    int            line;
    int            index;
    size_t         size;
    char           memory[1];
};

struct Block * blocks_first;
size_t block_list_size;
size_t block_index;

static void remove_block(struct Block * block) {
    if (block == blocks_first) {
        blocks_first = blocks_first->next;
    }
    if (block->prev) block->prev->next = block->next;
    if (block->next) block->next->prev = block->prev;
    block->next = NULL;
    block->prev = NULL;
    --block_list_size;
}

static void add_block(struct Block * block) {
    block->next = blocks_first;
    block->prev = NULL;
    if (blocks_first) {
        assert(!blocks_first->prev);
        blocks_first->prev = block;
    }
    blocks_first = block;
    ++block_list_size;
}

void * dbg_malloc(size_t size, const char * file, int line) {
    void * p;
    struct Block * block;
    p = malloc(sizeof(struct Block) + size - 1);
    if (!p) return NULL;
    block = p;
    block->file = file;
    block->line = line;
    block->index = block_index++;
    block->size = size;
    add_block(block);
    return block->memory;
}

void * dbg_calloc(size_t size, const char * file, int line) {
    void * p;
    struct Block * block;
    p = calloc(sizeof(struct Block) + size - 1, 1);
    if (!p) return NULL;
    block = p;
    block->file = file;
    block->line = line;
    block->index = block_index++;
    block->size = size;
    add_block(block);
    return block->memory;
}

void * dbg_realloc(void * ptr, size_t size, const char * file, int line) {
    struct Block * block;
    if (!ptr) return dbg_malloc(size, file, line);
    block = ptr = (char *)ptr - offsetof(struct Block, memory);
    remove_block(block);
    block = realloc(block, sizeof(struct Block) + size - 1);
    block->file = file;
    block->line = line;
    block->index = block_index++;
    block->size = size;
    add_block(block);
    return block->memory;
}

void dbg_free(void * ptr, const char * file, int line) {
    if (!ptr) return;
    (void) file;
    (void) line;
    ptr = (char *)ptr - offsetof(struct Block, memory);
    remove_block(ptr);
    free(ptr);
}

void dbg_print_blocks(void) {
    struct Block * it;
    size_t i;
    it = blocks_first;
    i = 0;
    printf("%lu blocks are not freed:\n", block_list_size);
    while (it && i < 20) {
        printf("#%d\t%lu\t%s:%d\n", it->index, it->size, it->file, it->line);
        it = it->next;
        ++i;
    }
}

int dbg_is_memory_clear(void) {
    return !block_list_size;
}
