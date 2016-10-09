/*
 * author: Nolan <sullen.goose@gmail.com>
 * Copy if you can.
 */

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "allocator.h"
#include "json_string.h"
#include "json_array.h"
#include "json_object.h"

#define ALLOCATOR AK_STACK

#define FAIL { return 0; }

enum LexemeKind {
    JLK_OBJ_BEG, JLK_OBJ_END, JLK_ARR_BEG, JLK_ARR_END, JLK_COMMA, JLK_COLON,
    JLK_TRUE,    JLK_FALSE,   JLK_NULL,    JLK_STRING,  JLK_NUMBER
};

struct jsonValue {
    enum jsonValueKind kind;
    union {
        double number;
        struct jsonString string;
        struct jsonObject object;
        struct jsonArray array;
        int boolean;
    } value;
};

struct jsonDocument {
    struct jsonValue * value;
    struct allocAllocator * allocator;
};

struct LexemeData {
    enum LexemeKind kind;
    union {
        struct jsonString * unescaped;
        double number;
    } extra;
};

/* Marker for trailing bytes */
#define TR (-1)

/* Marker for illegal bytes */
#define IL (-2)

static int utf8_bytes_left[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
   TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
   TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
   TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  5,  5, IL, IL,
};

extern enum jsonValueKind json_value_kind(struct jsonValue * value) {
    return value->kind;
}

extern struct jsonString * json_value_string(struct jsonValue * value) {
    return &value->value.string;
}

extern double json_value_number(struct jsonValue * value) {
    return value->value.number;
}

extern struct jsonObject * json_value_object(struct jsonValue * value) {
    return &value->value.object;
}

extern struct jsonArray * json_value_array(struct jsonValue * value) {
    return &value->value.array;
}

extern int json_value_bool(struct jsonValue * value) {
    return value->value.boolean;
}

static size_t parse_value(struct allocAllocator * allocator,
        const char * json, struct jsonValue * value);

extern struct jsonDocument * json_parse(const char * json) {
    struct allocAllocator * allocator;
    struct jsonDocument * document;
    allocator = alloc_create_allocator(ALLOCATOR);
    document = alloc_malloc(allocator, sizeof(struct jsonDocument));
    if (!document) {
        goto fail;
    }
    document->allocator = allocator;
    if (!(document->value = alloc_malloc(allocator, sizeof(struct jsonValue)))) goto fail;
    if (!parse_value(allocator, json, document->value)) {
        goto fail;
    }
    return document;
fail:
    alloc_free_allocator(AK_STACK, allocator);
    return NULL;
}

extern struct jsonValue * json_document_value(struct jsonDocument * document) {
    return document->value;
}

extern void json_document_free(struct jsonDocument * document) {
    alloc_free_allocator(AK_STACK, document->allocator);
}

/* Writes code point representing 'hex' unicode character into 'out' and returns
   pointer to first byte after written code point. */
static void put_utf8_code_point(int32_t hex, struct jsonString * unescaped,
        struct allocAllocator * allocator) {
    if (hex <= 0x007f) {
        json_string_append(unescaped, allocator, (char) hex);
    } else if (0x0080 <= hex && hex <= 0x07ff) {
        json_string_append(unescaped, allocator, (char) (0xc0 | hex >> 6));
        json_string_append(unescaped, allocator, (char) (0x80 | (hex & 0x3f)));
    } else {
        json_string_append(unescaped, allocator, (char) (0xe0 | hex >> 12));
        json_string_append(unescaped, allocator, (char) (0x80 | ((hex >> 6) & 0x3f)));
        json_string_append(unescaped, allocator, (char) (0x80 | (hex & 0x3f)));
    }
}

#define SINGLE_CHAR_LEXEME(c, k) \
    case (c): \
        ++json; \
        data->kind = (k); \
        return json - begin;

#define CONCRETE_WORD_LEXEME(c, w, k) \
    case (c):  \
        if (is_prefix((w), json)) return 0; \
        json += sizeof(w) - 1; \
        data->kind = (k); \
        return json - begin;

#define SKIPWS while (isspace(*json)) ++json

static size_t parse_string(struct allocAllocator * allocator,
        const char * json, struct jsonString * string) {
    const char * begin;
    char * end;
    int i;
    int left;
    uint32_t hex;
    begin = json;
    SKIPWS;
    assert('"' == *json);
    ++json;
    while (1) {
        switch (*json) {
        case '"':
            if (!json_string_append(string, allocator, '\0')) FAIL;
            if (!json_string_shrink(string, allocator)) FAIL;
            ++json;
            return json - begin;
        case '\\':
            ++json;
            switch (*json++) {
            case '\\': json_string_append(string, allocator, '\\'); break;
            case '"': json_string_append(string, allocator, '"'); break;
            case '/': json_string_append(string, allocator, '/'); break;
            case 'b': json_string_append(string, allocator, '\b'); break;
            case 'f': json_string_append(string, allocator, '\f'); break;
            case 'n': json_string_append(string, allocator, '\n'); break;
            case 'r': json_string_append(string, allocator, '\r'); break;
            case 't': json_string_append(string, allocator, '\t'); break;
            case 'u':
                /* FIXME:strtol takes as many characters as can */
                hex = strtoul(json, &end, 16);
                json += 4;
                if (end != json) FAIL;
                put_utf8_code_point(hex, string, allocator);
                break;
            default: FAIL;
            }
            break;
        default:
            left = utf8_bytes_left[*(const unsigned char *)json];
            if (left < 0) FAIL;
            json_string_append(string, allocator, *json++);
            for (i = 0; i < left; ++i) {
                if (TR != utf8_bytes_left[*(const unsigned char *)json]) FAIL;
                json_string_append(string, allocator, *json++);
            }
            break;
        }
    }
}

/* Parses json object and stores pointer to it into '*object'. 'json' must
   point to next lexeme after '{'. Object will be created in heap so don't
   forget to free it using json_object_free function. */
static size_t parse_object(struct allocAllocator * allocator,
        const char * json, struct jsonObject * object) {
    const char * begin;
    struct jsonString * key;
    struct jsonValue * value;
    size_t bytes_read;
    begin = json;
    assert('{' == *json);
    ++json;
    SKIPWS;
    if ('}' == *json) return json + 1 - begin;
    while (1) {
        if (!(key = json_string_create(allocator))) FAIL;
        if (!(bytes_read = parse_string(allocator, json, key))) FAIL;
        json += bytes_read;
        SKIPWS;
        if (':' != *json++) FAIL;
        if (!(value = alloc_malloc(allocator, sizeof(struct jsonValue)))) FAIL;
        if (!(bytes_read = parse_value(allocator, json, value))) FAIL;
        json += bytes_read;
        if (!json_object_add(object, allocator, key, value)) FAIL;
        SKIPWS;
        if ('}' == *json) { ++json; break; }
        if (',' != *json++) FAIL;
    }
    return json - begin;
}

/* Parses json array. 'json' must point to next lexeme after '['. Don't forget to free. */
static size_t parse_array(struct allocAllocator * allocator,
        const char * json, struct jsonArray * array) {
    const char * begin;
    struct jsonValue * value;
    size_t bytes_read;
    begin = json;
    assert('[' == *json);
    ++json;
    SKIPWS;
    if (']' == *json) return json + 1 - begin;
    while (1) {
        if (!(value = alloc_malloc(allocator, sizeof(struct jsonValue)))) FAIL;
        if (!(bytes_read = parse_value(allocator, json, value))) FAIL;
        json += bytes_read;
        if (!json_array_add(array, allocator, value)) FAIL;
        SKIPWS;
        if (']' == *json) { ++json; break; };
        if (',' != *json++) FAIL;
    }
    return json - begin;
}

static size_t parse_value_object(struct allocAllocator * allocator, const char * json,
        struct jsonValue * value) {
    value->kind = JVK_OBJ;
    json_object_init(&value->value.object);
    return parse_object(allocator, json, &value->value.object);
}

static size_t parse_value_array(struct allocAllocator * allocator, const char * json,
        struct jsonValue * value) {
    value->kind = JVK_ARR;
    json_array_init(&value->value.array);
    return parse_array(allocator, json, &value->value.array);
}

/* Returns non zero value if 'prefix' is prefix of 'str' and 0 otherwise. */
static int is_prefix(const char * prefix, const char * str) {
    while (*prefix && *str && *prefix++ == *str++);
    return !*prefix;
}

static size_t parse_value_true(const char * json, struct jsonValue * value) {
    if (!is_prefix("true", json)) FAIL;
    value->kind = JVK_BOOL;
    value->value.boolean = 1;
    return 4;
}

static size_t parse_value_false(const char * json, struct jsonValue * value) {
    if (!is_prefix("false", json)) FAIL;
    value->kind = JVK_BOOL;
    value->value.boolean = 0;
    return 5;
}

static size_t parse_value_null(const char * json, struct jsonValue * value) {
    if (!is_prefix("null", json)) FAIL;
    value->kind = JVK_NULL;
    return 4;
}

static size_t parse_value_string(struct allocAllocator * allocator, const char * json,
        struct jsonValue * value) {
    json_string_init(&value->value.string);
    value->kind = JVK_STR;
    return parse_string(allocator, json, &value->value.string);
}

static size_t parse_value_number(const char * json, struct jsonValue * value) {
    char * end;
    value->kind = JVK_NUM;
    value->value.number = strtod(json, &end);
    return end - json;
}

/* Parses json value into 'value'. Don't forget to call json_value_free.  */
static size_t parse_value(struct allocAllocator * allocator,
        const char * json, struct jsonValue * value) {
    const char * begin;
    size_t bytes_read;
    begin = json;
    SKIPWS;
    switch (*json) {
        case '{': bytes_read = parse_value_object(allocator, json, value); break;
        case '[': bytes_read = parse_value_array(allocator, json, value);  break;
        case 't': bytes_read = parse_value_true(json, value);   break;
        case 'f': bytes_read = parse_value_false(json, value);  break;
        case 'n': bytes_read = parse_value_null(json, value);   break;
        case '"': bytes_read = parse_value_string(allocator, json, value); break;
        default:  bytes_read = parse_value_number(json, value); break;
    }
    if (!bytes_read) FAIL;
    json += bytes_read;
    return json - begin;
}

