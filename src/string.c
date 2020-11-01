#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

#define INITIAL_CAPACITY    16

#define FNV_OFFSET_BASIS    2166136261u
#define FNV_PRIME           16777619u

extern struct jsonString *json_string_create(void) {
    struct jsonString *string = json_malloc(sizeof(struct jsonString));
    if (string) {
        json_string_init(string);
    }
    return string;
}

extern void json_string_init(struct jsonString *string) {
    assert(string);
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
    string->hash = FNV_OFFSET_BASIS;
}

extern bool json_string_init_str(struct jsonString *string, const char *str) {
    assert(string);
    assert(str);
    string->size = string->capacity = strlen(str) + 1;
    string->data = json_malloc(string->size);
    if (!string->data) {
        return false;
    }
    strcpy(string->data, str);
    string->hash = json_string_hash(str);
    return true;
}

extern bool json_string_init_mem(struct jsonString *string, const char *mem, size_t n) {
    assert(string);
    assert(mem);
    string->size = string->capacity = n + 1;
    string->data = json_malloc(string->size);
    if (!string->data) {
        return false;
    }
    memcpy(string->data, mem, n);
    string->data[n] = '\0';
    string->hash = json_string_hash(string->data);
    return true;
}

extern void json_string_free_internal(struct jsonString *string) {
    if (!string) {
        return;
    }
    json_free(string->data);
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
    string->hash = FNV_OFFSET_BASIS;
}

static bool json_string_ensure_has_free_space(struct jsonString *string, size_t n) {
    size_t new_capacity = string->capacity;
    if (string->size + n <= string->capacity) {
        return true;
    }
    do {
        new_capacity = new_capacity ? 2 *new_capacity : INITIAL_CAPACITY;
    } while (string->size + n > new_capacity);
    string->data = json_realloc(string->data, new_capacity);
    if (!string->data) {
        return false;
    }
    string->capacity = new_capacity;
    return true;
}

extern bool json_string_append(struct jsonString *string, char c) {
    if (!json_string_ensure_has_free_space(string, 1)) {
        return false;
    }
    string->data[string->size++] = c;
    if (c) {
        string->hash ^= c;
        string->hash *= FNV_PRIME;
    }
    return true;
}

extern bool json_string_shrink(struct jsonString *string) {
    assert(string);
    if (!string->capacity) {
        return true;
    }
    assert(string->size);
    void *new_data = json_realloc(string->data, string->size);
    if (new_data) {
        string->data = new_data;
    }
    return new_data;
}

extern unsigned json_string_hash(const char *str) {
    unsigned n;
    n = FNV_OFFSET_BASIS;
    if (!str) return n;
    while (*str) {
        n ^= *str++;
        n *= FNV_PRIME;
    }
    return n;
}
