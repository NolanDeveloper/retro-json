#include <stdbool.h>
#include <stdlib.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

void *json_malloc_(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        set_error(error_out_of_memory);
    }
    return ptr;
}

void *json_calloc_(size_t size) {
    void *ptr = calloc(size, 1);
    if (!ptr) {
        set_error(error_out_of_memory);
    }
    return ptr;
}

void *json_realloc_(void *ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (!ptr) {
        set_error(error_out_of_memory);
    }
    return ptr;
}

void json_free_(void *ptr) {
    free(ptr);
}

