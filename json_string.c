/*
 * author: Nolan <sullen.goose@gmail.com>
 * license: license.terms
 */

#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "parser.h"
#include "allocator.h"
#include "json_string.h"

#define INITIAL_CAPACITY (16)

#define STRING_SIZE(capacity) (sizeof(struct jsonString) + capacity)

extern size_t json_string_length(struct jsonString * string) {
    return string->size;
}

extern const char * json_string_data(struct jsonString * string) {
    return string->data;
}

extern void json_string_init(struct jsonString * string) {
    string->capacity = 0;
    string->size = 0;
}

extern struct jsonString * json_string_create(struct allocAllocator * allocator) {
    struct jsonString * string;
    string = alloc_malloc(allocator, sizeof(struct jsonString));
    if (!string) return NULL;
    json_string_init(string);
    return string;
}

static int json_string_born(struct jsonString * string,
        struct allocAllocator * allocator) {
    assert(!string->size);
    assert(!string->capacity);
    string->data = alloc_malloc(allocator, INITIAL_CAPACITY);
    if (!string->data) return 0;
    string->capacity = INITIAL_CAPACITY;
    return 1;
}

static int json_string_grow(struct jsonString * string,
        struct allocAllocator * allocator) {
    size_t new_capacity;
    assert(string->size);
    assert(string->capacity);
    new_capacity = string->capacity * 2;
    string->data = alloc_realloc(allocator, string->data, string->capacity, new_capacity);
    if (!string->data) return 0;
    string->capacity = new_capacity;
    return 1;
}

static int json_string_ensure_has_free_space(struct jsonString * string,
        struct allocAllocator * allocator) {
    if (string->size < string->capacity) return 1;
    if (string->size) return json_string_grow(string, allocator);
    return json_string_born(string, allocator);
}

extern int json_string_append(struct jsonString * string,
        struct allocAllocator * allocator, char c) {
    if (!json_string_ensure_has_free_space(string, allocator)) return 0;
    string->data[string->size++] = c;
    return 1;
}

extern int json_string_shrink(struct jsonString * string,
        struct allocAllocator * allocator) {
    if (!string->capacity) return 1;
    assert(string->size);
    string->data = alloc_realloc(allocator, string->data, string->capacity, string->size);
    return !!string->data;
}
