#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

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
    size_t i;
    if (!array) return;
    for (i = 0; i < array->size; ++i) {
        json_value_free(array->values[i]);
    }
    json_free(array->values);
    array->size = 0;
    array->capacity = 0;
    array->values = NULL;
}

static int json_array_ensure_has_free_space(struct jsonArray * array, size_t n) {
    size_t new_capacity;
    if (array->size + n <= array->capacity) return 1;
    new_capacity = array->capacity;
    do {
        new_capacity = new_capacity ? 2 * new_capacity : INITIAL_CAPACITY;
    } while (array->size + n > new_capacity);
    array->values = json_realloc(array->values, new_capacity * sizeof(struct jsonValue *));
    array->capacity = new_capacity;
    return 1;
}

extern int json_array_append(struct jsonArray * array, struct jsonValue * value) {
    if (!json_array_ensure_has_free_space(array, 1)) return 0;
    array->values[array->size++] = value;
    return 1;
}

size_t json_value_array_size(struct jsonValue * array) {
    return array ? array->v.array.size : 0;
}

struct jsonValue * json_value_array_at(struct jsonValue * array, size_t index) {
    return (array && index < array->v.array.size) ? array->v.array.values[index] : NULL;
}
