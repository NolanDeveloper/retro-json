#include <assert.h>
#include <string.h>

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

extern struct jsonValue *json_create_object(void) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_OBJ;
    object_init(&json->v.object);
    return json;
}

extern struct jsonValue *json_create_array(void) {
    struct jsonValue *json = json_malloc(sizeof(struct jsonValue));
    if (!json) {
        return NULL;
    }
    json->kind = JVK_ARR;
    array_init(&json->v.array);
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
        errorf("array->kind != JVK_ARR");
        return false;
    }
    return array_append(&array->v.array, value);
}

extern size_t json_array_size(struct jsonValue *array) {
    return array ? array->v.array.size : 0;
}

extern struct jsonValue *json_array_at(struct jsonValue *array, size_t index) {
    return (array && index < array->v.array.size) ? array->v.array.values[index] : NULL;
}

extern const char *json_strerror(void) {
    char *error = tss_get(error_key);
    return error ? error : "NULL";
}

extern struct jsonValue *json_parse(const char *json, bool all) {
    if (!json) {
        errorf("json == NULL");
        return NULL;
    }
    parser_begin(json);
    struct jsonValue *value = parse_json_text(all);
    parser_end();
    assert(!!value != !!strcmp(json_strerror(), "NULL"));
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
    struct jsonString *jkey = json_malloc(sizeof(struct jsonString));
    return jkey &&
        string_init_str(jkey, key) &&
        object_add(&object->v.object, jkey, value);
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
