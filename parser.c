// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

static struct jsonLexemeData lexeme_data;

static size_t hash(unsigned char * str) {
    size_t hash = 5381;
    size_t c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

static int json_object_grow(struct jsonObject * object);

static void json_object_add_pair(struct jsonObject * object, char * name, 
        struct jsonValue value) {
    if ((double) object->size / object->capacity > 0.75) {
        json_object_grow(object);
    }
    size_t h = hash((unsigned char *) name) % object->capacity;
    while (object->buckets[h].name) h = (h + 1) % object->capacity;
    object->buckets[h].name = name;
    object->buckets[h].value = value;
    ++object->size;
}

static int json_object_grow(struct jsonObject * object) {
    size_t old_capacity = object->capacity;
    struct jsonPair * old_buckets = object->buckets;
    size_t i;
    object->size = 0;
    object->capacity *= 2;
    object->buckets = json_malloc(object->capacity * sizeof(struct jsonPair));
    if (!object->buckets) return 0;
    memset(object->buckets, 0, object->capacity * sizeof(struct jsonPair));
    for (i = 0; i < old_capacity; ++i) {
        if (!old_buckets[i].name) continue;
        json_object_add_pair(object, old_buckets[i].name, old_buckets[i].value);
    }
    return 1;
}

static struct jsonObject * json_object_create(size_t initial_capacity) {
    struct jsonObject * object = json_malloc(sizeof(struct jsonObject));
    if (!object) return 0;
    object->capacity = initial_capacity;
    object->size = 0;
    if (initial_capacity) {
        object->buckets = json_malloc(initial_capacity * sizeof(struct jsonPair));
        return object->buckets ? object : 0;
    } 
    object->buckets = NULL;
    return object;
}

static void json_object_free(struct jsonObject * object) {
    size_t i;
    if (object->buckets) {
        for (i = 0; i < object->capacity; ++i) {
            if (!object->buckets[i].name) continue;
            json_free(object->buckets[i].name);
            json_value_free(object->buckets[i].value);
        }
        json_free(object->buckets);
    }
    json_free(object);
}

// '{' was read
static size_t json_parse_object(const char * json, struct jsonObject ** object) {
    const char * begin = json;
    char * name;
    struct jsonValue value;
    enum jsonLexemeKind kind = json_next_lexeme(json, &lexeme_data);
    size_t bytes_read;
    json += lexeme_data.bytes_read;
    if (kind == JS_R_BRACE) {
        *object = json_object_create(0);
        return lexeme_data.bytes_read;
    }
    *object = json_object_create(10);
    while (1) {
        if (JS_STRING != kind) goto fail;
        name = json_malloc(lexeme_data.measured_length);
        if (!name) goto fail;
        if (!json_get_string(lexeme_data.begin, name)) goto fail;
        if (JS_COLON != json_next_lexeme(json, &lexeme_data)) goto fail;
        json += lexeme_data.bytes_read;
        bytes_read = json_parse_value(json, &value);
        if (!bytes_read) goto fail;
        json += bytes_read;
        json_object_add_pair(*object, name, value);
        kind = json_next_lexeme(json, &lexeme_data);
        json += lexeme_data.bytes_read;
        if (JS_R_BRACE == kind) break;
        if (JS_COMMA != kind) goto fail;
        kind = json_next_lexeme(json, &lexeme_data);
        json += lexeme_data.bytes_read;
    }
    return json - begin;
fail:
    json_object_free(*object);
    return 0;
}

static struct jsonArray * json_array_create(size_t initial_capacity) {
    struct jsonArray * array = json_malloc(sizeof(struct jsonArray));
    if (!array) return 0;
    array->size = 0;
    array->capacity = initial_capacity;
    if (initial_capacity) {
        array->values = json_malloc(initial_capacity * sizeof(struct jsonValue));
        return array->values ? array : 0;
    }
    array->values = NULL;
    return array;
}

static int json_array_add(struct jsonArray * array, struct jsonValue value) {
    struct jsonValue * new_values;
    if (array->size == array->capacity) {
        if (!array->capacity) {
            array->capacity = 4;
            array->values = json_malloc(array->capacity * sizeof(struct jsonValue));
            if (!array->values) return 0;
        } else {
            array->capacity *= 2;
            new_values = json_malloc(array->capacity * sizeof(struct jsonValue));
            if (!new_values) return 0;
            memcpy(new_values, array->values, array->capacity * sizeof(struct jsonValue));
            json_free(array->values);
            array->values = new_values;
        }
    }
    array->values[array->size++] = value;
    return 1;
}

static void json_array_free(struct jsonArray * array) {
    size_t i;
    if (array->values) {
        for (i = 0; i < array->size; ++i) {
            json_value_free(array->values[i]);
        }
        json_free(array->values);
    }
    json_free(array);
}

// '[' was read
static size_t json_parse_array(const char * json, struct jsonArray ** array) {
    const char * begin = json;
    struct jsonValue value;
    size_t bytes_read;
    enum jsonLexemeKind kind;
    if (JS_R_SQUARE_BRACKET == json_next_lexeme(json, &lexeme_data)) {
        *array = json_array_create(0);
        return lexeme_data.bytes_read;
    }
    *array = json_array_create(4);
    while (1) {
        bytes_read = json_parse_value(json, &value);
        if (!bytes_read) goto fail;
        json += bytes_read;       
        kind = json_next_lexeme(json, &lexeme_data);
        json += lexeme_data.bytes_read;
        json_array_add(*array, value);
        if (kind == JS_R_SQUARE_BRACKET) break;
        if (kind != JS_COMMA) goto fail;
    }
    return json - begin;
fail:
    json_array_free(*array);
    return 0;
}

extern size_t json_parse_value(const char * json, struct jsonValue * value) {
    const char * begin = json;
    struct jsonValue ret;
    size_t bytes_read;
    switch (json_next_lexeme(json, &lexeme_data)) {
    case JS_L_BRACE: 
        ret.kind = JVK_OBJ;
        ret.obj = json_malloc(sizeof(struct jsonObject));
        if (!ret.obj) return 0;
        json += lexeme_data.bytes_read;
        bytes_read = json_parse_object(json, &ret.obj);
        if (!bytes_read) {
            json_value_free(ret);
            return 0;
        }
        json += bytes_read;
        *value = ret;
        return json - begin;
    case JS_L_SQUARE_BRACKET: 
        ret.kind = JVK_ARR;
        ret.arr = json_malloc(sizeof(struct jsonArray));
        if (!ret.arr) return 0;
        json += lexeme_data.bytes_read;
        bytes_read = json_parse_array(json, &ret.arr);
        if (!bytes_read) {
            json_value_free(ret);
            return 0;
        }
        json += bytes_read;
        *value = ret;
        return json - begin;
    case JS_TRUE:
        ret.kind = JVK_BOOL;
        ret.bul = 1;
        break;
    case JS_FALSE:
        ret.kind = JVK_BOOL;
        ret.bul = 0;
        break;
    case JS_NULL:
        ret.kind = JVK_NULL;
        break;
    case JS_STRING:
        ret.kind = JVK_STR;
        ret.str = json_malloc(sizeof(lexeme_data.measured_length));
        if (!ret.str) return 0;
        json_get_string(lexeme_data.begin, ret.str);
        break;
    case JS_NUMBER:
        ret.kind = JVK_NUM;
        ret.num = strtod(lexeme_data.begin, NULL);
        break;
    default: return 0;
    }
    *value = ret;
    return lexeme_data.bytes_read;
}

extern void json_value_free(struct jsonValue value) {
    switch (value.kind) {
    case JVK_OBJ: json_object_free(value.obj); break;
    case JVK_ARR: json_array_free(value.arr);  break;
    case JVK_STR: json_free(value.str);        break;
    default:;
    }
}
