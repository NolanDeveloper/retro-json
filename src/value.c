#include <assert.h>

#include "json_internal.h"

extern void json_value_free(struct jsonValue *value) {
    if (!value) {
        return;
    }
    value_free_internal(value);
    json_free(value);
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



