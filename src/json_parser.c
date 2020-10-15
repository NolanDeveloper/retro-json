#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "json_parser.h"
#include "json_string.h"
#include "json_array.h"
#include "json_object.h"

#ifndef NDEBUG
#include <stdio.h>
static void o_my_fail(void) { } /* useful for backtrace */
#define FAIL() o_my_fail()
#else
#define FAIL()
#endif

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

static size_t parse_value(const char * json, struct jsonValue * value);

extern struct jsonValue * json_parse(const char * json) {
    struct jsonValue * value;
    value = json_malloc(sizeof(struct jsonValue));
    if (!value) goto fail;
    if (!parse_value(json, value)) goto fail;
    return value;
fail:
    json_free(value);
    FAIL();
    return NULL;
}

extern void json_value_free(struct jsonValue * value) {
    if (!value) return;
    json_value_free_internal(value);
    json_free(value);
}

extern void json_value_free_internal(struct jsonValue * value) {
    if (!value) return;
    switch (value->kind) {
    case JVK_STR:  json_string_free_internal(&value->value.string); return;
    case JVK_NUM:  return;
    case JVK_OBJ:  json_object_free_internal(&value->value.object); return;
    case JVK_ARR:  json_array_free_internal(&value->value.array);   return;
    case JVK_BOOL: return;
    case JVK_NULL: return;
    default: exit(1);
    }
    json_free(value);
}

static void put_utf8_code_point(int32_t hex, struct jsonString * unescaped) {
    if (hex <= 0x007f) {
        json_string_append(unescaped, (char) hex);
    } else if (0x0080 <= hex && hex <= 0x07ff) {
        json_string_append(unescaped, (char) (0xc0 | hex >> 6));
        json_string_append(unescaped, (char) (0x80 | (hex & 0x3f)));
    } else {
        json_string_append(unescaped, (char) (0xe0 | hex >> 12));
        json_string_append(unescaped, (char) (0x80 | ((hex >> 6) & 0x3f)));
        json_string_append(unescaped, (char) (0x80 | (hex & 0x3f)));
    }
}

#define SKIPWS while (isspace(*json)) ++json

static size_t parse_string(const char * json, struct jsonString * string) {
    const char * begin;
    char * end;
    int i;
    int left;
    uint32_t hex;
    char buf[5];
    begin = json;
    SKIPWS;
    assert('"' == *json);
    ++json;
    while (1) {
        switch (*json) {
        case '"':
            if (!json_string_append(string, '\0')) { FAIL(); return 0; }
            if (!json_string_shrink(string)) { FAIL(); return 0; }
            ++json;
            return json - begin;
        case '\\':
            ++json;
            switch (*json++) {
            case '\\': json_string_append(string, '\\'); break;
            case '"': json_string_append(string, '"'); break;
            case '/': json_string_append(string, '/'); break;
            case 'b': json_string_append(string, '\b'); break;
            case 'f': json_string_append(string, '\f'); break;
            case 'n': json_string_append(string, '\n'); break;
            case 'r': json_string_append(string, '\r'); break;
            case 't': json_string_append(string, '\t'); break;
            case 'u':
                buf[3] = buf[4] = '\0';
                strncpy(buf, json, 4);
                if (!buf[3]) { FAIL(); return 0; } /* \uXXXX - not enought X */
                hex = strtoul(buf, &end, 16);
                json += 4;
                put_utf8_code_point(hex, string);
                break;
            default: FAIL(); return 0;
            }
            break;
        default:
            left = utf8_bytes_left[*(const unsigned char *)json];
            if (left < 0) { FAIL(); return 0; }
            json_string_append(string, *json++);
            for (i = 0; i < left; ++i) {
                if (TR != utf8_bytes_left[*(const unsigned char *)json]) { FAIL(); return 0; }
                json_string_append(string, *json++);
            }
            break;
        }
    }
}

static size_t parse_object(const char * json, struct jsonObject * object) {
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
        if (!(key = json_malloc(sizeof(struct jsonString)))) goto fail;
        json_string_init(key);
        if (!(bytes_read = parse_string(json, key))) goto fail;
        json += bytes_read;
        SKIPWS;
        if (':' != *json++) goto fail;
        if (!(value = json_malloc(sizeof(struct jsonValue)))) goto fail;
        if (!(bytes_read = parse_value(json, value))) goto fail;
        json += bytes_read;
        if (!json_object_add(object, key, value)) goto fail;
        key = NULL;
        value = NULL;
        SKIPWS;
        if ('}' == *json) { ++json; return json - begin; }
        if (',' != *json++) goto fail;
    }
    exit(1);
fail:
    json_string_free_internal(key);
    json_free(key);
    json_value_free(value);
    json_object_free_internal(object);
    FAIL();
    return 0;
}

static size_t parse_array(const char * json, struct jsonArray * array) {
    const char * begin;
    struct jsonValue * value;
    size_t bytes_read;
    begin = json;
    assert('[' == *json);
    ++json;
    SKIPWS;
    if (']' == *json) return json + 1 - begin;
    while (1) {
        if (!(value = json_malloc(sizeof(struct jsonValue)))) goto fail;
        if (!(bytes_read = parse_value(json, value))) goto fail;
        json += bytes_read;
        if (!json_array_add(array, value)) goto fail;
        value = NULL;
        SKIPWS;
        if (']' == *json) { ++json; break; };
        if (',' != *json++) goto fail;
    }
    return json - begin;
fail:
    json_value_free(value);
    FAIL();
    return 0;
}

static size_t parse_value_object(const char * json, struct jsonValue * value) {
    value->kind = JVK_OBJ;
    json_object_init(&value->value.object);
    return parse_object(json, &value->value.object);
}

static size_t parse_value_array(const char * json, struct jsonValue * value) {
    value->kind = JVK_ARR;
    json_array_init(&value->value.array);
    return parse_array(json, &value->value.array);
}

static int is_not_prefix(const char * pref, const char * str) {
    return strncmp(pref, str, strlen(pref));
}

static size_t parse_value_true(const char * json, struct jsonValue * value) {
    if (is_not_prefix("true", json)) { FAIL(); return 0; }
    value->kind = JVK_BOOL;
    value->value.boolean = 1;
    return 4;
}

static size_t parse_value_false(const char * json, struct jsonValue * value) {
    if (is_not_prefix("false", json)) { FAIL(); return 0; }
    value->kind = JVK_BOOL;
    value->value.boolean = 0;
    return 5;
}

static size_t parse_value_null(const char * json, struct jsonValue * value) {
    if (is_not_prefix("null", json)) { FAIL(); return 0; }
    value->kind = JVK_NULL;
    return 4;
}

static size_t parse_value_string(const char * json, struct jsonValue * value) {
    json_string_init(&value->value.string);
    value->kind = JVK_STR;
    return parse_string(json, &value->value.string);
}

static size_t parse_value_number(const char * json, struct jsonValue * value) {
    char * end;
    value->kind = JVK_NUM;
    value->value.number = strtod(json, &end);
    return end - json;
}

static size_t parse_value(const char * json, struct jsonValue * value) {
    const char * begin;
    size_t bytes_read;
    begin = json;
    SKIPWS;
    switch (*json) {
        case '{': bytes_read = parse_value_object(json, value); break;
        case '[': bytes_read = parse_value_array(json, value);  break;
        case 't': bytes_read = parse_value_true(json, value);   break;
        case 'f': bytes_read = parse_value_false(json, value);  break;
        case 'n': bytes_read = parse_value_null(json, value);   break;
        case '"': bytes_read = parse_value_string(json, value); break;
        default:  bytes_read = parse_value_number(json, value); break;
    }
    if (!bytes_read) { FAIL(); return 0; }
    json += bytes_read;
    return json - begin;
}

