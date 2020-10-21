#include <stdlib.h>

#include "json.h"
#include "json_internal.h"

struct jsonValue * json_value_create_number(double number) {
    struct jsonValue * json;
    json = json_malloc(sizeof(struct jsonValue));
    if (!json) return NULL;
    json->kind = JVK_NUM;
    json->v.number = number;
    return json;
}

struct jsonValue * json_value_create_string(const char * string) {
    struct jsonValue * json;
    json = json_malloc(sizeof(struct jsonValue));
    if (!json) return NULL;
    json->kind = JVK_STR;
    if (!json_string_init_str(&json->v.string, string)) return NULL;
    return json;
}

struct jsonValue * json_value_create_object(void) {
    struct jsonValue * json;
    json = json_malloc(sizeof(struct jsonValue));
    if (!json) return NULL;
    json->kind = JVK_OBJ;
    json_object_init(&json->v.object);
    return json;
}

struct jsonValue * json_value_create_array(void) {
    struct jsonValue * json;
    json = json_malloc(sizeof(struct jsonValue));
    if (!json) return NULL;
    json->kind = JVK_ARR;
    json_array_init(&json->v.array);
    return json;
}

struct jsonValue * json_value_create_boolean(int boolean) {
    struct jsonValue * json;
    json = json_malloc(sizeof(struct jsonValue));
    if (!json) return NULL;
    json->kind = JVK_BOOL;
    json->v.boolean = boolean;
    return json;
}

struct jsonValue * json_value_create_null(void) {
    struct jsonValue * json;
    json = json_malloc(sizeof(struct jsonValue));
    if (!json) return NULL;
    json->kind = JVK_NULL;
    return json;
}

int json_value_array_add(struct jsonValue * array, struct jsonValue * value) {
    if (array->kind != JVK_ARR) return 0;
    if (!json_array_add(&array->v.array, value)) return 0;
    return 1;
}

