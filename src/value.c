#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

extern void json_value_free(struct jsonValue *value) {
    if (!value) {
        return;
    }
    json_value_free_internal(value);
    json_free(value);
}

extern void json_value_free_internal(struct jsonValue *value) {
    if (!value) {
        return;
    }
    switch (value->kind) {
    case JVK_STR:  
        json_string_free_internal(&value->v.string); 
        return;
    case JVK_NUM:  
        return;
    case JVK_OBJ:  
        json_object_free_internal(&value->v.object); 
        return;
    case JVK_ARR:  
        json_array_free_internal(&value->v.array);   
        return;
    case JVK_BOOL: 
        return;
    case JVK_NULL: 
        return;
    default:
        assert(false);
    }
}

