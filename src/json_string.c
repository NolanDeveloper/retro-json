#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "json_parser.h"
#include "json_string.h"

#define INITIAL_CAPACITY (16)

extern size_t json_string_length(struct jsonString * string) {
    return string->size;
}

extern const char * json_string_data(struct jsonString * string) {
    return string->data;
}

extern void json_string_init(struct jsonString * string) {
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
}

extern void json_string_free_internal(struct jsonString * string) {
    if (!string) return;
    json_free(string->data);
    string->capacity = 0;
    string->size = 0;
    string->data = NULL;
}

static int json_string_ensure_has_free_space(struct jsonString * string) {
    size_t new_capacity;
    if (string->size < string->capacity) return 1;
    if (!string->capacity) {
        string->data = json_malloc(INITIAL_CAPACITY);
        if (!string->data) return 0;
        string->capacity = INITIAL_CAPACITY;
        return 1;
    }
    new_capacity = string->capacity * 2;
    string->data = json_realloc(string->data, new_capacity);
    if (!string->data) return 0;
    string->capacity = new_capacity;
    return 1;
}

extern int json_string_append(struct jsonString * string, char c) {
    if (!json_string_ensure_has_free_space(string)) return 0;
    string->data[string->size++] = c;
    return 1;
}

extern int json_string_shrink(struct jsonString * string) {
    if (!string->capacity) return 1;
    assert(string->size);
    string->data = json_realloc(string->data, string->size);
    return !!string->data;
}
