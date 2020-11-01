#include <stdlib.h>
#include <stdbool.h>
#include <threads.h>
#include <uchar.h>

#include "json_internal.h"

struct jsonValue *json_create_number(double number) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_NUM;
    json->v.number = number;
    return json;
}

struct jsonValue *json_create_string(const char *string) {
    if (!string) {
        errorf("string == NULL");
        return NULL;
    }
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_STR;
    if (!string_init_str(&json->v.string, string)) {
        json_free(json);
        return NULL;
    }
    return json;
}

struct jsonValue *json_create_object(void) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_OBJ;
    object_init(&json->v.object);
    return json;
}

struct jsonValue *json_create_array(void) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_ARR;
    array_init(&json->v.array);
    return json;
}

struct jsonValue *json_create_boolean(bool boolean) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_BOOL;
    json->v.boolean = boolean;
    return json;
}

struct jsonValue *json_create_null(void) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_NULL;
    return json;
}

bool json_array_append(struct jsonValue *array, struct jsonValue *value) {
    if (!array) {
        errorf("array == NULL");
        return false;
    }
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    if (array->kind != JVK_ARR) {
        errorf("array->kind != JVK_ARR");
        return false;
    }
    return array_append(&array->v.array, value);
}

