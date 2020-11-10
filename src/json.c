#include <assert.h>
#include <string.h>
#include <math.h>

#include "json_internal.h"

extern bool json_init(void) {
    return error_init();
}

extern void json_exit(void) {
    error_exit();
}

extern void value_free_internal(struct jsonValue *value) {
    if (!value) {
        return;
    }
    switch (value->kind) {
    case JVK_STR:
        string_free_internal(&value->v.string);
        return;
    case JVK_NUM:
        return;
    case JVK_OBJ:
        object_free_internal(&value->v.object);
        return;
    case JVK_ARR:
        array_free_internal(&value->v.array);
        return;
    case JVK_BOOL:
        return;
    case JVK_NULL:
        return;
    default:
        assert(false);
    }
}

extern void json_value_free(struct jsonValue *value) {
    if (!value) {
        return;
    }
    value_free_internal(value);
    json_free(value);
}

extern struct jsonValue *json_create_number(double number) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_NUM;
    json->v.number = number;
    return json;
}

extern struct jsonValue *json_create_string(const char *string) {
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

extern struct jsonValue *json_create_object(size_t initial_capacity) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_OBJ;
    object_init(&json->v.object);
    if (!object_reserve(&json->v.object, initial_capacity)) {
        object_free_internal(&json->v.object);
        json_free(json);
        return NULL;
    }
    return json;
}

extern struct jsonValue *json_create_array(size_t initial_capacity) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_ARR;
    array_init(&json->v.array);
    if (!array_reserve(&json->v.array, initial_capacity)) {
        array_free_internal(&json->v.array);
        json_free(json);
        return NULL;
    }
    return json;
}

extern struct jsonValue *json_create_boolean(bool boolean) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_BOOL;
    json->v.boolean = boolean;
    return json;
}

extern struct jsonValue *json_create_null(void) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_NULL;
    return json;
}

extern bool json_get_number(struct jsonValue *number, double *value) {
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    if (!number) {
        errorf("number == NULL");
        *value = NAN;
        return false;
    }
    if (number->kind != JVK_NUM) {
        errorf("argument is not json number");
        *value = NAN;
        return false;
    }
    *value = number->v.number;
    return true;
}

extern bool json_get_string(struct jsonValue *string, const char **value) {
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    if (!string) {
        errorf("string == NULL");
        *value = NULL;
        return false;
    }
    if (string->kind != JVK_STR) {
        errorf("argument is not json string");
        *value = NULL;
        return false;
    }
    *value = string->v.string.data;
    return true;
}

extern bool json_get_string_length(struct jsonValue *string, size_t *length) {
    if (!length) {
        errorf("length == NULL");
        return false;
    }
    if (!string) {
        errorf("string == NULL");
        *length = 0;
        return false;
    }
    if (string->kind != JVK_STR) {
        errorf("argument is not json string");
        *length = 0;
        return false;
    }
    return string->v.string.size - 1;
}

extern bool json_get_boolean(struct jsonValue *boolean, bool *value) {
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    if (!boolean) {
        errorf("boolean == NULL");
        *value = false;
        return false;
    }
    if (boolean->kind != JVK_BOOL) {
        errorf("argument is not json boolean");
        *value = false;
        return false;
    }
    *value = boolean->v.boolean;
    return true;
}

extern bool json_set_number(struct jsonValue *number, double value) {
    if (!number) {
        errorf("number == NULL");
        return false;
    }
    if (number->kind != JVK_NUM) {
        errorf("argument is not json number");
        return false;
    }
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    number->v.number = value;
    return true;
}

extern bool json_set_string(struct jsonValue *string, const char *value) {
    if (!string) {
        errorf("string == NULL");
        return false;
    }
    if (string->kind != JVK_STR) {
        errorf("argument is not json string");
        return false;
    }
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    string_free_internal(&string->v.string);
    return string_init_str(&string->v.string, value);
}

extern bool json_set_boolean(struct jsonValue *boolean, bool value) {
    if (!boolean) {
        errorf("boolean == NULL");
        return false;
    }
    if (boolean->kind != JVK_BOOL) {
        errorf("argument is not json boolean");
        return false;
    }
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    boolean->v.boolean = value;
    return true;
}

extern bool json_array_append(struct jsonValue *array, struct jsonValue *value) {
    if (!array) {
        errorf("array == NULL");
        return false;
    }
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    if (array->kind != JVK_ARR) {
        errorf("argument is not json array");
        return false;
    }
    return array_append(&array->v.array, value);
}

extern size_t json_array_size(struct jsonValue *array) {
    if (!array) {
        errorf("array == NULL");
        return false;
    }
    if (array->kind != JVK_ARR) {
        errorf("argument is not json array");
        return false;
    }
    return array ? array->v.array.size : 0;
}

extern struct jsonValue *json_array_at(struct jsonValue *array, size_t index) {
    if (!array) {
        errorf("array == NULL");
        return false;
    }
    if (array->kind != JVK_ARR) {
        errorf("argument is not json array");
        return false;
    }
    return (array && index < array->v.array.size) ? array->v.array.values[index] : NULL;
}

extern const char *json_strerror(void) {
    char *error = tss_get(error_key);
    return error ? error : "";
}

extern struct jsonValue *json_parse(const char *json, bool all) {
    if (!json) {
        errorf("json == NULL");
        return NULL;
    }
    parser_begin(json, strlen(json));
    struct jsonValue *value = parse_json_text(all);
    parser_end();
    assert(!!value != !!strcmp(json_strerror(), ""));
    return value;
}

struct jsonValue *json_parse_mem(const char *buffer, size_t size, bool all) {
    if (!buffer) {
        errorf("buffer == NULL");
        return NULL;
    }
    parser_begin(buffer, size);
    struct jsonValue *value = parse_json_text(all);
    parser_end();
    assert(!!value != !!strcmp(json_strerror(), ""));
    return value;
}

extern size_t json_pretty_print(char *out, size_t size, struct jsonValue *value) {
    if (!value) {
        errorf("value == NULL");
        return 0;
    }
    pretty_print_begin(out, size);
    print_json_value(value);
    return pretty_print_end();
}

extern size_t json_object_number_of_keys(struct jsonValue *object) {
    if (!object) {
        errorf("object == NULL");
        return 0;
    }
    if (object->kind != JVK_OBJ) {
        errorf("argument is not json object");
        return 0;
    }
    return object->v.object.unique_size;
}

extern size_t json_object_number_of_values(struct jsonValue *object) {
    if (!object) {
        errorf("object == NULL");
        return 0;
    }
    if (object->kind != JVK_OBJ) {
        errorf("argument is not json object");
        return 0;
    }
    return object->v.object.size;
}

extern bool json_object_add(struct jsonValue *object, const char *key, struct jsonValue *value) {
    if (!object) {
        errorf("object == NULL");
        return false;
    }
    if (object->kind != JVK_OBJ) {
        errorf("argument is not json object");
        return false;
    }
    if (!key) {
        errorf("key == NULL");
        return false;
    }
    if (!value) {
        errorf("value == NULL");
        return false;
    }
    struct jsonString *jkey = string_create();
    string_init_str(jkey, key);
    return object_add(&object->v.object, jkey, value);
}

extern size_t json_object_capacity(struct jsonValue *object) {
    if (!object) {
        errorf("object == NULL");
        return 0;
    }
    if (object->kind != JVK_OBJ) {
        errorf("argument is not json object");
        return 0;
    }
    return object->v.object.capacity;
}

extern bool json_object_get_entry(struct jsonValue *object, size_t i, const char **key, struct jsonValue **value) {
    if (!object) {
        errorf("object == NULL");
        return false;
    }
    if (object->kind != JVK_OBJ) {
        errorf("argument is not json object");
        return false;
    }
    if (object->v.object.capacity <= i) {
        errorf("index out of range");
        return false;
    }
    struct jsonString *jkey = NULL;
    object_get_entry(&object->v.object, i, &jkey, value);
    if (jkey) {
        *key = jkey->data;
    }
    return true;
}

extern struct jsonValue *
json_object_lookup_next(struct jsonValue *object, const char *key, struct jsonValue *value) {
    if (!object) {
        errorf("object == NULL");
        return NULL;
    }
    if (object->kind != JVK_OBJ) {
        errorf("argument is not json object");
        return false;
    }
    if (!key) {
        errorf("key == NULL");
        return NULL;
    }
    return object_next(&object->v.object, key, value);
}

extern struct jsonValue *json_object_lookup(struct jsonValue *object, const char *key) {
    return json_object_lookup_next(object, key, NULL);
}

static struct jsonValue *duplicate_object(struct jsonObject *object) {
    assert(object);
    struct jsonValue *copy = json_create_object(object->size);
    if (!copy) {
        return NULL;
    }
    struct jsonString *key_copy = NULL;
    struct jsonValue *value_copy = NULL;
    size_t n = object->capacity;
    for (size_t i = 0; i < n; ++i) {
        struct jsonString *key = NULL;
        struct jsonValue *value = NULL;
        object_get_entry(object, i, &key, &value);
        if (!key) {
            continue;
        }
        key_copy = string_create_str(key->data);
        if (!key_copy) {
            goto fail;
        }
        value_copy = json_copy(value);
        if (!value_copy) {
            goto fail;
        }
        if (!object_add(&copy->v.object, key_copy, value_copy)) {
            goto fail;
        }
        key_copy = NULL;
        value_copy = NULL;
    }
    return copy;
fail:
    string_free_internal(key_copy);
    json_free(copy);
    return NULL;
}

static struct jsonValue *duplicate_array(struct jsonArray *array) {
    assert(array);
    size_t n = array->size;
    struct jsonValue *copy = json_create_array(n);
    if (copy) {
        return NULL;
    }
    struct jsonValue *value_copy = NULL;
    for (size_t i = 0; i < n; ++i) {
        struct jsonValue *value = array->values[i];
        assert(value);
        value_copy = json_copy(value);
        if (!value_copy) {
            goto fail;
        }
        if (!json_array_append(copy, value_copy)) {
            goto fail;
        }
    }
    return copy;
fail:
    json_free(value_copy);
    return NULL;
}

extern struct jsonValue *json_copy(struct jsonValue *value) {
    if (!value) {
        return value;
    }
    switch (value->kind) {
    case JVK_STR:
        return json_create_string(value->v.string.data);
    case JVK_NUM:
        return json_create_number(value->v.number);
    case JVK_OBJ:
        return duplicate_object(&value->v.object);
    case JVK_ARR:
        return duplicate_array(&value->v.array);
    case JVK_BOOL:
        return json_create_boolean(value->v.boolean);
    case JVK_NULL:
        return json_create_null();
    default:
        assert(false);
    }
}

static thread_local struct jsonValue **left_diff;
static thread_local struct jsonValue **right_diff;

static bool are_equal(struct jsonValue *left, struct jsonValue *right);

static bool objects_are_equal(struct jsonObject *left, struct jsonObject *right) {
    assert(left->size == right->size);
    assert(left->unique_size == right->unique_size);
    size_t n = left->capacity;
    for (size_t i = 0; i < n; ++i) {
        struct jsonString *key = NULL;
        struct jsonValue *value = NULL;
        object_get_entry(left, i, &key, &value);
        if (!key) {
            continue;
        }
        struct jsonValue *right_value = NULL;
        do {
            right_value = object_next(right, key->data, right_value);
        } while (right_value && !are_equal(value, right_value));
        if (!right_value) {
            *left_diff = value;
            *right_diff = NULL;
            return false;
        }
    }
    return true;
}

static bool arrays_are_equal(struct jsonArray *left, struct jsonArray *right) {
    assert(left->size == right->size);
    for (size_t i = 0; i < left->size; ++i) {
        if (!are_equal(left->values[i], right->values[i])) {
            return false;
        }
    }
    return true;
}

static bool are_equal(struct jsonValue *left, struct jsonValue *right) {
    bool result = true;
    if (!left && !right) {
        result = true;
        goto end;
    }
    if (!left || !right) {
        result = false;
        goto end;
    }
    if (left->kind != right->kind) {
        result = false;
        goto end;
    }
    switch (left->kind) {
    case JVK_STR:
        result = !strcmp(left->v.string.data, right->v.string.data);
        break;
    case JVK_NUM:
        result = left->v.boolean == right->v.boolean;
        break;
    case JVK_OBJ:
        if (left->v.object.size != right->v.object.size
                || left->v.object.unique_size != right->v.object.unique_size) {
            result = false;
        } else {
            return objects_are_equal(&left->v.object, &right->v.object);
        }
        break;
    case JVK_ARR:
        if (left->v.array.size != right->v.array.size) {
            result = false;
        } else {
            return arrays_are_equal(&left->v.array, &right->v.array);
        }
        break;
    case JVK_BOOL:
        result = left->v.boolean == right->v.boolean;
        break;
    case JVK_NULL:
        break;
    default:
        assert(false);
    }
end:
    if (!result) {
        *left_diff = left;
        *right_diff = right;
    }
    return result;
}

extern bool json_are_equal(struct jsonValue *left, struct jsonValue *right,
        struct jsonValue **left_out, struct jsonValue **right_out) {
    struct jsonValue *stub;
    left_diff = left_out ? left_out : &stub;
    right_diff = right_out ? right_out : &stub;
    bool result = are_equal(left, right);
    left_diff = right_diff = NULL;
    return result;
}
