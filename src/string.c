#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "json_internal.h"

#define INITIAL_CAPACITY    32

#define FNV_OFFSET_BASIS    2166136261u
#define FNV_PRIME           16777619u

extern struct jsonString *string_create(void) {
    struct jsonString *string = json_malloc(sizeof(struct jsonString));
    if (string) {
        string_init(string);
    }
    return string;
}

extern struct jsonString *string_create_str(const char *str) {
    struct jsonString *string = json_malloc(sizeof(struct jsonString));
    if (string) {
        string_init_str(string, str);
    }
    return string;
}

extern void string_init(struct jsonString *string) {
    assert(string);
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
    string->hash = FNV_OFFSET_BASIS;
}

extern bool string_init_str(struct jsonString *string, const char *str) {
    assert(string);
    assert(str);
    string->size = string->capacity = strlen(str) + 1;
    string->data = json_malloc(string->size);
    if (!string->data) {
        return false;
    }
    strcpy(string->data, str);
    string->hash = string_hash(str);
    return true;
}

extern bool string_init_mem(struct jsonString *string, const char *mem, size_t n) {
    assert(string);
    assert(mem);
    string->size = string->capacity = n + 1;
    string->data = json_malloc(string->size);
    if (!string->data) {
        return false;
    }
    memcpy(string->data, mem, n);
    string->data[n] = '\0';
    string->hash = string_hash(string->data);
    return true;
}

extern void string_free_internal(struct jsonString *string) {
    if (!string) {
        return;
    }
    json_free(string->data);
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
    string->hash = FNV_OFFSET_BASIS;
}

static bool string_reserve(struct jsonString *string, size_t min_capacity) {
    assert(string);
    if (min_capacity <= string->capacity) {
        return true;
    }
    size_t new_capacity = string->capacity ? string->capacity : INITIAL_CAPACITY;
    do {
        new_capacity *= 2;
    } while (new_capacity < min_capacity);
    string->data = json_realloc(string->data, new_capacity);
    if (!string->data) {
        return false;
    }
    string->capacity = new_capacity;
    return true;
}

extern bool string_append(struct jsonString *string, char c) {
    assert(string);
    if (!string_reserve(string, string->size + 1)) {
        return false;
    }
    string->data[string->size++] = c;
    string->hash = (string->hash ^ c) * FNV_PRIME;
    return true;
}

extern bool string_shrink(struct jsonString *string) {
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

extern unsigned string_hash(const char *str) {
    unsigned n = FNV_OFFSET_BASIS;
    if (!str) {
        return n;
    }
    do {
        n = (n ^ *str) * FNV_PRIME;
    } while (*str++);
    return n;
}
