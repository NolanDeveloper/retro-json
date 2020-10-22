#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"
#include "json_internal.h"

#define INITIAL_CAPACITY 16

extern struct jsonValue * json_object_at(struct jsonObject * object, const char * key) {
    struct jsonObjectEntry * entry, * fence;
    unsigned hash;
    if (!object->capacity || !object->size) return NULL;
    hash = json_string_hash(key);
    fence = &object->entries[object->capacity];
    entry = &object->entries[hash % object->capacity];
    while (entry->key) {
        if (entry->key->hash == hash && !strcmp(entry->key->data, key)) {
            break;
        }
        if (++entry == fence) {
            entry = object->entries;
        }
    }
    if (!entry->key) return NULL;
    return entry->value;
}

extern void json_object_init(struct jsonObject * object) {
    object->capacity = 0;
    object->size     = 0;
    object->entries  = NULL;
    object->first    = NULL;
    object->last     = NULL;
}

extern void json_object_free_internal(struct jsonObject * object) {
    struct jsonObjectEntry * entry;
    if (!object) return;
    entry = object->first;
    while (entry) {
        json_string_free_internal(entry->key);
        json_free(entry->key);
        json_value_free(entry->value);
        entry = entry->next;
    }
    json_free(object->entries);
    json_object_init(object);
}

static int json_object_ensure_has_free_space(struct jsonObject * object) {
    struct jsonObjectEntry * old_entries, * first;
    if (!object) return 0;
    if (!object->entries) {
        object->entries = json_calloc(INITIAL_CAPACITY * sizeof(struct jsonObjectEntry));
        if (!object->entries) return 0;
        object->capacity = INITIAL_CAPACITY;
        return 1;
    }
    if ((object->size + 1) * 3 / 2 < object->capacity) {
        return 1;
    }
    object->capacity *= 2;
    old_entries = object->entries;
    object->entries = json_calloc(object->capacity * sizeof(struct jsonObjectEntry));
    if (!object->entries) return 0;
    first = object->first;
    object->first = object->last = NULL;
    object->size = 0;
    while (first) {
        assert(json_object_add(object, first->key, first->value));
        first = first->next;
    }
    json_free(old_entries);
    return 1;
}

extern int json_object_add(struct jsonObject * object, struct jsonString * key, struct jsonValue * value) {
    struct jsonObjectEntry * entry, * fence;
    if (!object || !key || !value) return 0; 
    if (!json_object_ensure_has_free_space(object)) return 0;
    fence = &object->entries[object->capacity];
    entry = &object->entries[key->hash % object->capacity]; 
    while (entry->key) {
        if (entry->key->hash == key->hash && !strcmp(entry->key->data, key->data)) {
            return 0;
        }
        if (++entry == fence) {
            entry = object->entries;
        }
    }
    entry->key = key;
    entry->value = value;
    if (object->last) {
        object->last->next = entry;
        entry->prev = object->last;
        object->last = entry;
    } else {
        object->first = object->last = entry;
        entry->prev = NULL;
    }
    entry->next = NULL;
    ++object->size;
    return 1;
}

int json_value_object_add(struct jsonValue * object, const char * key, struct jsonValue * value) {
    struct jsonString * string;
    if (object->kind != JVK_OBJ) return 0;
    string = json_malloc(sizeof(struct jsonString));
    if (!string) return 0;
    if (!json_string_init_str(string, key)) return 0;
    return json_object_add(&object->v.object, string, value);
}

extern struct jsonValue * json_value_object_lookup(struct jsonValue * object, const char * key) {
    return json_object_at(&object->v.object, key);
}
