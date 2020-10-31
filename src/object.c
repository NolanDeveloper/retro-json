#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

#define INVERSE_MAX_OCCUPANCY 2

static const struct jsonString key_deleted;

// array of prime numbers of strictly growing bit length
static const size_t prime_capacities[] = {
    13ull, 29ull, 59ull, 97ull, 223ull, 457ull, 977ull, 1831ull, 3643ull,
    8009ull, 16087ull, 30367ull, 60257ull, 122387ull, 246473ull, 445931ull,
    984461ull, 2071873ull, 3329129ull, 6645757ull, 14753681ull, 27844433ull,
    62198197ull, 130849643ull, 232540541ull, 420468661ull, 874249081ull,
    1825850717ull, 4177971107ull, 7997985953ull, 15006665651ull,
    32109786613ull, 52234947971ull, 103684385887ull, 233215828553ull,
    482890090867ull, 1017518287769ull, 1808739102661ull, 3820730969873ull,
    7143951075221ull, 15911846005817ull, 34549382138311ull, 60873837348941ull,
    114125472132457ull, 225020097064313ull, 483814914675413ull,
    995330423937163ull, 2052422557552501ull, 3827369225572561ull,
    8029744166750309ull, 15947206541890957ull, 28292002491075067ull,
    65400623661884507ull, 116875762939333753ull, 233389247835623543ull,
    538660358633970413ull, 1017699188710033703ull, 1825687239835937623ull,
    4124828355664746773ull, 9080577297740147699ull, 13948657424817642929ull,
};

extern void json_object_init(struct jsonObject *object) {
    assert(object);
    object->capacity = 0;
    object->size = 0;
    object->entries = NULL;
    object->first = NULL;
    object->last = NULL;
}

extern void json_object_free_internal(struct jsonObject *object) {
    assert(object);
    for (size_t i = 0; i < object->capacity; ++i) {
        struct jsonObjectEntry *entry = &object->entries[i];
        if (!entry->key) {
            continue;
        }
        if (entry->key == &key_deleted) {
            assert(!entry->value);
            assert(!entry->next);
            assert(!entry->prev);
            continue;
        }
        json_string_free_internal(entry->key);
        json_free(entry->key);
        assert(entry->value);
        json_value_free(entry->value);
    }
    json_free(object->entries);
    object->capacity = 0;
    object->size = 0;
    object->entries = NULL;
    object->first = NULL;
    object->last = NULL;
}

/* Make object internal buffer big enough to hold `size` elements and keep good
 * performance of operations. */
extern bool json_object_reserve(struct jsonObject *object, size_t size) {
    assert(object);
    size_t min_capacity = size * INVERSE_MAX_OCCUPANCY;
    if (min_capacity <= object->capacity) {
        return true;
    }
    size_t new_capacity = 0;
    for (size_t i = 0; i < sizeof(prime_capacities) / sizeof(*prime_capacities); ++i) {
        new_capacity = prime_capacities[i];
        if (min_capacity <= new_capacity) {
            break;
        }
    }
    struct jsonObjectEntry *new_entries = json_calloc(new_capacity * sizeof(struct jsonObjectEntry));
    if (!new_entries) {
        return false;
    }
    struct jsonObjectEntry *old_entries = object->entries;
    struct jsonObjectEntry *entry = object->first;
    json_object_init(object);
    object->capacity = new_capacity;
    object->entries = new_entries;
    object->size = 0;
    while (entry) {
        assert(json_object_add(object, entry->key, entry->value));
        entry->key = NULL;
        entry->value = NULL;
        entry = entry->next;
    }
    json_free(old_entries);
    return true;
}

/* Remove entry from linked list. */
static void cut(struct jsonObject *object, struct jsonObjectEntry *entry) {
    assert(object);
    assert(entry);
    if (object->first == entry) {
        object->first = entry->next;
    }
    if (object->last == entry) {
        object->last = entry->prev;
    }
    if (entry->prev) {
        assert(entry->prev->next == entry);
        entry->prev->next = entry->next;
    }
    if (entry->next) {
        assert(entry->next->prev == entry);
        entry->next->prev = entry->prev;
    }
    entry->next = entry->prev = NULL;
}

/* Add entry to linked list after `prev` or put it first if `prev` == NULL. */
static void insert(struct jsonObject *object, struct jsonObjectEntry *prev, struct jsonObjectEntry *entry) {
    assert(object);
    assert(entry);
    assert(!entry->prev);
    assert(!entry->next);
    if (!prev) {
        entry->next = object->first;
        object->first = entry;
        if (!object->last) {
            object->last = entry;
        }
    } else {
        entry->next = prev->next;
        if (entry->next) {
            entry->next->prev = entry;
        }
        entry->prev = prev;
        prev->next = entry;
        if (prev == object->last) {
            object->last = entry;
        }
    }
}

extern bool json_object_add(struct jsonObject *object, struct jsonString *key, struct jsonValue *value) {
    assert(object);
    assert(key);
    assert(value);
    if (!json_object_reserve(object, object->size + 1)) {
        return false;
    }
    unsigned hash = key->hash;
    struct jsonObjectEntry *prev = object->last;
    for (size_t i = hash % object->capacity; ; i = (i + 1 == object->capacity ? 0 : i + 1)) {
        struct jsonObjectEntry *entry = &object->entries[i];
        // empty
        if (!entry->key) {
            entry->key = key;
            entry->value = value;
            entry->prev = entry->next = NULL;
            insert(object, prev, entry);
            break;
        }
        // deleted
        if (entry->key == &key_deleted) {
            continue;
        }
        // duplicate
        if (entry->key->hash == hash && !strcmp(entry->key->data, key->data)) {
            struct jsonObjectEntry *new_prev = entry->prev;
            cut(object, entry);
            struct jsonValue *old_value = entry->value;
            entry->value = value;
            value = old_value;
            insert(object, prev, entry);
            prev = new_prev;
        }
    }
    ++object->size;
    return true;
}

extern struct jsonValue *json_object_next(struct jsonObject *object, const char *key, struct jsonValue *prev) {
    assert(object);
    assert(key);
    if (!object->capacity) {
        return NULL;
    }
    bool prev_was_met = !prev;
    unsigned hash = json_string_hash(key);
    for (size_t i = hash % object->capacity; ; i = (i + 1 == object->capacity ? 0 : i + 1)) {
        struct jsonObjectEntry *entry = &object->entries[i];
        // empty
        if (!entry->key) {
            return NULL;
        }
        // deleted
        if (entry->key == &key_deleted) {
            continue;
        }
        // match
        if (entry->key->hash == hash && !strcmp(entry->key->data, key)) {
            if (prev_was_met) {
                return entry->value;
            }
            prev_was_met = prev == entry->value;
        }
    }
}

extern struct jsonValue *json_object_at(struct jsonObject *object, const char *key) {
    return json_object_next(object, key, NULL);
}

extern bool json_value_object_add(struct jsonValue *object, const char *key, struct jsonValue *value) {
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
        json_string_init_str(jkey, key) && 
        json_object_add(&object->v.object, jkey, value);
}

extern struct jsonValue *
json_value_object_lookup_next(struct jsonValue *object, const char *key, struct jsonValue *value) {
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
    return json_object_next(&object->v.object, key, value);
}

extern struct jsonValue *json_value_object_lookup(struct jsonValue *object, const char *key) {
    return json_value_object_lookup_next(object, key, NULL);
}

