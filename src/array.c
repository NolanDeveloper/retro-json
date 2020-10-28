#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <threads.h>

#include "json.h"
#include "json_internal.h"

#define INITIAL_CAPACITY 6

extern size_t json_array_size(struct jsonArray * array) {
    return array->size;
}

extern void json_array_init(struct jsonArray * array) {
    array->capacity = 0;
    array->size = 0;
    array->values = NULL;
}

extern void json_array_free_internal(struct jsonArray * array) {
    if (!array) {
        return;
    }
    for (size_t i = 0; i < array->size; ++i) {
        json_value_free(array->values[i]);
    }
    json_free(array->values);
    array->size = 0;
    array->capacity = 0;
    array->values = NULL;
}

static bool json_array_ensure_has_free_space(struct jsonArray * array, size_t n) {
    if (array->size + n <= array->capacity) {
        return true;
    }
    size_t new_capacity = array->capacity;
    do {
        new_capacity = new_capacity ? 2 * new_capacity : INITIAL_CAPACITY;
    } while (array->size + n > new_capacity);
    struct jsonValue ** new_values = json_realloc(array->values, new_capacity * sizeof(struct jsonValue *));
    if (!new_values) {
        return false;
    }
    array->values = new_values;
    array->capacity = new_capacity;
    return true;
}

extern bool json_array_append(struct jsonArray * array, struct jsonValue * value) {
    if (!json_array_ensure_has_free_space(array, 1)) {
        return false;
    }
    array->values[array->size++] = value;
    return true;
}

size_t json_value_array_size(struct jsonValue * array) {
    return array ? array->v.array.size : 0;
}

struct jsonValue * json_value_array_at(struct jsonValue * array, size_t index) {
    return (array && index < array->v.array.size) ? array->v.array.values[index] : NULL;
}
