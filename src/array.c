#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <threads.h>
#include <uchar.h>

#include "json_internal.h"

#define INITIAL_CAPACITY 6

extern size_t array_size(struct jsonArray *array) {
    return array->size;
}

extern void array_init(struct jsonArray *array) {
    array->capacity = 0;
    array->size = 0;
    array->values = NULL;
}

extern void array_free_internal(struct jsonArray *array) {
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

/* Makes this true: new array->capacity = max(old array->capacity, new_capacity)  */
extern bool array_reserve(struct jsonArray *array, size_t new_capacity) {
    assert(array);
    if (new_capacity <= array->capacity) {
        return true;
    }
    struct jsonValue **new_values = json_realloc(array->values, new_capacity * sizeof(struct jsonValue *));
    if (!new_values) {
        return false;
    }
    array->values = new_values;
    array->capacity = new_capacity;
    return true;
}

/* Increase array capacity by doubling (possibly 0 times) until it's greater
 * than new_size. */
extern bool array_double(struct jsonArray *array, size_t min_capacity) {
    assert(array);
    if (min_capacity <= array->capacity) {
        return true;
    }
    size_t new_capacity = array->capacity ? array->capacity : INITIAL_CAPACITY;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }
    return array_reserve(array, new_capacity);
}

extern bool array_append(struct jsonArray *array, struct jsonValue *value) {
    assert(array);
    if (!array_double(array, array->size + 1)) {
        return false;
    }
    array->values[array->size++] = value;
    return true;
}
