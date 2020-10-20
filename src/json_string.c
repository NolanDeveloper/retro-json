#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"
#include "json_internal.h"

#define INITIAL_CAPACITY    16

#define FNV_OFFSET_BASIS    2166136261
#define FNV_PRIME           16777619

extern void json_string_init(struct jsonString * string) {
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
    string->hash = FNV_OFFSET_BASIS;
}

extern int json_string_init_str(struct jsonString * string, const char * str) {
    string->size = string->capacity = strlen(str) + 1;
    string->data = json_malloc(string->size);
    if (!string->data) return 0;
    strcpy(string->data, str);
    string->hash = json_string_hash(str);
    return 1;
}

extern void json_string_free_internal(struct jsonString * string) {
    if (!string) return;
    json_free(string->data);
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
    string->hash = FNV_OFFSET_BASIS;
}

static int json_string_ensure_has_free_space(struct jsonString * string) {
    size_t new_capacity;
    if (string->size < string->capacity) return 1;
    new_capacity = string->capacity ? 2 * string->capacity : INITIAL_CAPACITY;
    string->data = json_realloc(string->data, new_capacity);
    if (!string->data) return 0;
    string->capacity = new_capacity;
    return 1;
}

extern int json_string_append(struct jsonString * string, char c) {
    if (!json_string_ensure_has_free_space(string)) return 0;
    string->data[string->size++] = c;
    string->hash ^= c;
    string->hash *= FNV_PRIME;
    return 1;
}

extern int json_string_shrink(struct jsonString * string) {
    if (!string->capacity) return 1;
    assert(string->size);
    string->data = json_realloc(string->data, string->size);
    return !!string->data;
}

extern unsigned json_string_hash(const char * str) {
    unsigned n;
    n = FNV_OFFSET_BASIS;
    if (!str) return n;
    while (*str) {
        n ^= *str++;
        n *= FNV_PRIME;
    }
    return n;
}
