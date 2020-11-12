#include <assert.h>
#include <limits.h>
#include <string.h>

#include "json_internal.h"

#define INVERSE_MAX_OCCUPANCY 2

const struct jsonString key_deleted;
static thread_local unsigned long long uniq = 1;

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

extern void object_init(struct jsonObject *object) {
    assert(object);
    object->capacity = 0;
    object->size = 0;
    object->unique_size = 0;
    object->entries = NULL;
}

extern void object_free_internal(struct jsonObject *object) {
    assert(object);
    for (size_t i = 0; i < object->capacity; ++i) {
        struct jsonObjectEntry *entry = &object->entries[i];
        if (!entry->key) {
            assert(!entry->value);
            continue;
        }
        if (entry->key == &key_deleted) {
            assert(!entry->value);
            continue;
        }
        assert(entry->value);
        string_free_internal(entry->key);
        free(entry->key);
        assert(entry->value);
        json_value_free(entry->value);
    }
    free(object->entries);
    object->capacity = 0;
    object->size = 0;
    object->entries = NULL;
}

/* Make object internal buffer big enough to hold `size` elements and keep good
 * performance of operations. */
extern bool object_reserve(struct jsonObject *object, size_t size) {
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
    struct jsonObjectEntry *new_entries = calloc(new_capacity, sizeof(struct jsonObjectEntry));
    if (!new_entries) {
        return false;
    }
    struct jsonObjectEntry *old_entries = object->entries;
    size_t old_capacity = object->capacity;
    object_init(object);
    object->capacity = new_capacity;
    object->entries = new_entries;
    object->size = 0;
    for (size_t i = 0; i < old_capacity; ++i) {
        struct jsonObjectEntry *entry = &old_entries[i];
        if (!entry->key || entry->key == &key_deleted) {
            continue;
        }
        assert(object_add(object, entry->key, entry->value));
        entry->key = NULL;
        entry->value = NULL;
    }
    free(old_entries);
    return true;
}

extern bool object_add(struct jsonObject *object, struct jsonString *key, struct jsonValue *value) {
    assert(object);
    assert(key);
    assert(value);
    if (!object_reserve(object, object->size + 1)) {
        return false;
    }
    unsigned hash = key->hash;
    bool is_duplicate = false;
    for (size_t i = hash % object->capacity; ; i = (i + 1 == object->capacity ? 0 : i + 1)) {
        struct jsonObjectEntry *entry = &object->entries[i];
        // empty or deleted
        if (!entry->key || entry->key == &key_deleted) {
            entry->key = key;
            entry->value = value;
            entry->id = uniq++;
            break;
        }
        // duplicate
        if (!is_duplicate && entry->key->hash == hash && !strcmp(entry->key->data, key->data)) {
            is_duplicate = true;
        }
        // otherwise it's occupied with a different key
    }
    ++object->size;
    if (!is_duplicate) {
        ++object->unique_size;
    }
    return true;
}

extern void
object_get_entry(struct jsonObject *object, size_t i, struct jsonString **out_key, struct jsonValue **out_value) {
    assert(object);
    assert(i < object->capacity);
    struct jsonValue *value = object->entries[i].value;
    struct jsonString *key = object->entries[i].key;
    if (key == &key_deleted) {
        key = NULL;
    }
    assert(!!value == !!key);
    *out_key = key;
    *out_value = value;
}

static unsigned long long id_of(struct jsonObject *object, unsigned hash, const char *key, struct jsonValue *prev) {
    assert(object);
    assert(key);
    assert(object->capacity);
    assert_slow(hash == string_hash(key));
    if (!prev) {
        return LLONG_MAX;
    }
    for (size_t i = hash % object->capacity; ; i = (i + 1 == object->capacity ? 0 : i + 1)) {
        struct jsonObjectEntry *entry = &object->entries[i];
        // empty
        if (!entry->key) {
            return 0;
        }
        // deleted
        if (entry->key == &key_deleted) {
            continue;
        }
        // matching && value == prev
        if (entry->key->hash == hash && !strcmp(entry->key->data, key) && entry->value == prev) {
            return entry->id;
        }
        // otherwise it's occupied with a different key
    }
    assert(false);
}

extern struct jsonValue *object_next(struct jsonObject *object, const char *key, struct jsonValue *prev) {
    assert(object);
    assert(key);
    if (!object->capacity) {
        return NULL;
    }
    unsigned hash = string_hash(key);
    unsigned long long prev_id = id_of(object, hash, key, prev);
    if (!prev_id) {
        return NULL;
    }
    unsigned long long id_greatest_less_than_prev_id = 0;
    struct jsonValue *value = NULL;
    for (size_t i = hash % object->capacity; ; i = (i + 1 == object->capacity ? 0 : i + 1)) {
        struct jsonObjectEntry *entry = &object->entries[i];
        // empty
        if (!entry->key) {
            break;
        }
        // deleted
        if (entry->key == &key_deleted) {
            continue;
        }
        // matching
        if (entry->key->hash == hash
                && !strcmp(entry->key->data, key)
                && entry->id > id_greatest_less_than_prev_id
                && entry->id < prev_id) {
            id_greatest_less_than_prev_id = entry->id;
            value = entry->value;
        }
    }
    return value;
}

extern struct jsonValue *object_at(struct jsonObject *object, const char *key) {
    return object_next(object, key, NULL);
}
