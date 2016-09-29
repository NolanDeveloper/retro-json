// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define ARRAY_INITIAL_CAPACITY 4
#define OBJECT_INITIAL_CAPACITY 8

enum LexemeKind {
    JLK_L_BRACE, JLK_R_BRACE, JLK_L_SQUARE_BRACKET, JLK_R_SQUARE_BRACKET,
    JLK_COMMA, JLK_COLON, JLK_TRUE, JLK_FALSE, JLK_NULL, JLK_STRING, JLK_NUMBER,
};

struct LexemeData {
    enum LexemeKind kind;
    size_t          bytes_read;
    const char *    begin;
    size_t          unescaped_length;
};

static int utf8_bytes_left[256] = {
    [ 0x00 ... 0x7f ] =  0,
    [ 0x80 ... 0xbf ] = -1,
    [ 0xc0 ... 0xdf ] =  1,
    [ 0xe0 ... 0xef ] =  2,
    [ 0xf0 ... 0xf7 ] =  3,
    [ 0xf8 ... 0xff ] = -1,
};

static uint32_t ctoh[256] = {
    [ '0' ] = 0,    [ 'a' ] = 10,   [ 'A' ] = 10,
    [ '1' ] = 1,    [ 'b' ] = 11,   [ 'B' ] = 11,
    [ '2' ] = 2,    [ 'c' ] = 12,   [ 'C' ] = 12,
    [ '3' ] = 3,    [ 'd' ] = 13,   [ 'D' ] = 13,
    [ '4' ] = 4,    [ 'e' ] = 14,   [ 'E' ] = 14,
    [ '5' ] = 5,    [ 'f' ] = 15,   [ 'F' ] = 15,
    [ '6' ] = 6,
    [ '7' ] = 7,
    [ '8' ] = 8,
    [ '9' ] = 9
};

static size_t get_utf8_code_point_length(int32_t hex) {
    if (hex <= 0x007f)
        return 1;
    else if (0x0080 <= hex && hex <= 0x07ff)
        return 2;
    return 3;
}

static char * put_utf8_code_point(int32_t hex, char * out) {
    if (hex <= 0x007f) {
        *out++ = hex;
    } else if (0x0080 <= hex && hex <= 0x07ff) {
        *out++ = 0xc0 | hex >> 6;
        *out++ = 0x80 | (hex & 0x3f);
    } else {
        *out++ = 0xe0 | hex >> 12;
        *out++ = 0x80 | ((hex >> 6) & 0x3f);
        *out++ = 0x80 | (hex & 0x3f);
    }
    return out;
}

static int copy_code_point(const unsigned char ** code_point, char ** out, char * end) {
    const unsigned char * p = *code_point;
    char * o = *out;
    int bytes_left = utf8_bytes_left[*p];
    if (-1 == bytes_left) return 0;
    *o++ = *p++;
    if (o >= end) return 0;
    while (bytes_left--) {
        if (*p >> 6 != 0x2) return 0;
        *o++ = *p++;
        if (o>= end) return 0;
    }
    *out = o;
    *code_point = p;
    return 1;
}

#define E(c) case (c): *o++ = (c); break;

#define F(c, d) case (c): *o++ = (d); break;

static int unescape_code_point(const char ** escaped, char ** out, char * end) {
    const char * p = *escaped;
    char * o = *out;
    char c;
    uint32_t hex;
    int i;
    switch (*p++) {
    E('\\') E('\"') E('/') F('b', '\b') F('f', '\f') F('n', '\n') F('r', '\r') F('t', '\t')
    case 'u':
        hex = 0;
        for (i = 0; i < 4; ++i) {
            c = *p++;
            if (!isxdigit(c)) return 0;
            hex = (hex << 4) | ctoh[*(unsigned char *)&c];
        }
        if (o + get_utf8_code_point_length(hex) >= end) return 0;
        o = put_utf8_code_point(hex, o);
        break;
    default: return 0;
    }
    *out = o;
    *escaped = p;
    return 1;
}

extern char * unescape_string(struct LexemeData * data) {
    const char * json = data->begin;
    char * unescaped = json_malloc(data->unescaped_length);
    char * end = unescaped + data->unescaped_length;
    char * out = unescaped;
    if (!unescaped) return NULL;
    while (1) {
        if (out >= end) goto fail;
        switch (*json) {
        case '"':
            *out++ = '\0';
            assert(out == end);
            return unescaped;
        case '\\':
            ++json;
            if (!unescape_code_point(&json, &out, end)) goto fail;
            break;
        default:
            if (!copy_code_point((const unsigned char **) &json, &out, end)) goto fail;
            break;
        }
    }
fail:
    json_free(unescaped);
    return NULL;
}

static int measure_escaped_code_point(const char ** code_point,
        size_t * unescaped_length) {
    uint32_t hex;
    int i;
    char c;
    switch (**code_point) {
    case '\\': case '\"': case '/': case 'b':
    case 'f': case 'n': case 'r': case 't':
        ++*unescaped_length;
        ++*code_point;
        return 1;
    case 'u':
        hex = 0;
        for (i = 0; i < 4; ++i) {
            c = *(*code_point)++;
            if (!isxdigit(c)) return 0;
            hex = (hex << 4) | ctoh[*(unsigned char *)&c];
        }
        *unescaped_length = get_utf8_code_point_length(hex);
        return 1;
    default: return 0;
    }
}

static size_t measure_code_point(const char ** code_point, size_t * unescaped_length) {
    int bytes_left = utf8_bytes_left[**(const unsigned char **)code_point];
    const char * cp = *code_point;
    size_t ul = 0;
    if (-1 == bytes_left) return 0;
    ++cp; ++ul;
    while (bytes_left--) {
        if (*cp >> 6 != 0x2) return 0;
        ++cp; ++ul;
    }
    *code_point = cp;
    *unescaped_length += ul;
    return 1;
}

static size_t read_string_lexeme(const char * json, size_t * unescaped_length) {
    const char * begin = json;
    size_t ul = 0;
    while (1) {
        switch (*json) {
        case '"':
            ++json;
            ++ul;
            *unescaped_length = ul;
            return json - begin;
        case '\\':
            ++json;
            if (!measure_escaped_code_point(&json, &ul)) return 0;
            break;
        default:
            if (!measure_code_point(&json, &ul)) return 0;
            break;
        }
    }
}

static size_t read_number_lexeme(const char * json) {
    const char * begin = json;
    if ('-' == *json) ++json;
         if ('0' == *json) ++json;
    else if ('1' <= *json && *json <= '9') {
        ++json;
        while (isdigit(*json)) ++json;
    } else return 0;
    if ('.' == *json) {
        ++json;
        while (isdigit(*json)) ++json;
    }
    if ('e' != *json && 'E' != *json) {
        return json - begin;
    }
    ++json;
    if ('+' == *json || '-' == *json) ++json;
    while (isdigit(*json)) ++json;
    return json - begin;
}

static int is_prefix(const char * prefix, const char * str) {
    while (*prefix && *str && *prefix++ == *str++);
    return !!*prefix;
}

#define SINGLE_CHAR_LEXEME(c, k) \
    case (c): \
        data->kind = (k); \
        data->bytes_read = json + 1 - begin; \
        return 1;

#define CONCRETE_WORD_LEXEME(c, w, k) \
    case (c):  \
        if (is_prefix((w), json)) return 0; \
        data->kind = (k); \
        data->bytes_read = json - begin + sizeof(w) - 1; \
        return 1;

extern size_t next_lexeme(const char * json, struct LexemeData * data) {
    const char * begin = json;
    size_t t;
    while (isspace(*json)) ++json;
    data->begin = json;
    switch (*json) {
    case '\0': return 0;
    SINGLE_CHAR_LEXEME('{', JLK_L_BRACE)
    SINGLE_CHAR_LEXEME('}', JLK_R_BRACE)
    SINGLE_CHAR_LEXEME('[', JLK_L_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(']', JLK_R_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(',', JLK_COMMA)
    SINGLE_CHAR_LEXEME(':', JLK_COLON)
    CONCRETE_WORD_LEXEME('t', "true",  JLK_TRUE)
    CONCRETE_WORD_LEXEME('f', "false", JLK_FALSE)
    CONCRETE_WORD_LEXEME('n', "null",  JLK_NULL)
    case '"':
        ++json;
        t = read_string_lexeme(json, &data->unescaped_length);
        if (!t) return 0;
        data->begin = json;
        data->kind = JLK_STRING;
        json += t;
        data->bytes_read = json - begin;
        return 1;
    default:
        t = read_number_lexeme(json);
        if (!t) return 0;
        json += t;
        data->kind = JLK_NUMBER;
        data->bytes_read = json - begin;
        return 1;
    }
}

static size_t hash(unsigned char * str) {
    size_t hash = 5381;
    size_t c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

static int json_object_grow(struct jsonObject * object);

static int json_object_add_pair(struct jsonObject * object, char * key,
        struct jsonValue value) {
    if ((double) object->size / object->capacity > 0.75) {
        if (!json_object_grow(object)) return 0;
    }
    size_t h = hash((unsigned char *) key) % object->capacity;
    while (object->keys[h]) {
        if (!strcmp(key, object->keys[h])) return 0;
        h = (h + 1) % object->capacity;
    }
    object->keys[h] = key;
    object->values[h] = value;
    ++object->size;
    return 0;
}

static int json_object_grow(struct jsonObject * object) {
    if (!object->size) return 0; // empty once - empty forever
    size_t old_capacity = object->capacity;
    char ** old_keys = object->keys;
    struct jsonValue * old_values = object->values;
    size_t i;
    size_t alloc_size = object->capacity * (sizeof(char *) + sizeof(struct jsonValue));
    object->size = 0;
    object->capacity *= 2;
    object->keys = json_malloc(alloc_size);
    if (!object->keys) return 0;
    object->values = (void *) object->keys + object->capacity * sizeof(char *);
    memset(object->keys, 0, alloc_size);
    for (i = 0; i < old_capacity; ++i) {
        if (!old_keys[i]) continue;
        json_object_add_pair(object, old_keys[i], old_values[i]);
    }
    json_free(old_keys);
    return 1;
}

static struct jsonObject * json_object_create(size_t initial_capacity) {
    struct jsonObject * object = json_malloc(sizeof(struct jsonObject));
    if (!object) return NULL;
    object->capacity = initial_capacity;
    object->size = 0;
    if (!initial_capacity) {
        object->keys = NULL;
        object->values = NULL;
        return object;
    }
    object->keys = json_malloc(initial_capacity *
            (sizeof(char *) + sizeof(struct jsonValue)));
    if (!object->keys) {
        json_free(object);
        return NULL;
    }
    object->values = (void *) object->keys + initial_capacity * sizeof(char *);
    memset(object->keys, 0, initial_capacity *
            (sizeof(char *) + sizeof(struct jsonValue)));
    return object;
}

extern struct jsonValue json_object_get_value(struct jsonObject * object,
        const char * key) {
    size_t h = hash((unsigned char *)key) % object->capacity;
    struct jsonValue value;
    while (object->keys[h] && strcmp(object->keys[h], key)) {
        h = (h + 1) % object->capacity;
    }
    if (object->keys[h]) return object->values[h];
    memset(&value, 0, sizeof(struct jsonValue));
    return value;
}

static void json_object_free(struct jsonObject * object) {
    size_t i;
    if (object->capacity) {
        for (i = 0; i < object->capacity; ++i) {
            if (!object->keys[i]) continue;
            json_free(object->keys[i]);
            json_value_free(object->values[i]);
        }
        json_free(object->keys);
    }
    json_free(object);
}

void json_object_traverse(struct jsonObject * object,
        void (*action)(const char * key, struct jsonValue)) {
    if (!object->size) return;
    size_t i;
    for (i = 0; i < object->size; ++i) {
        if (!object->keys[i]) continue;
        action(object->keys[i], object->values[i]);
    }
}

// '{' was read
static size_t json_parse_object(const char * json, struct jsonObject ** object) {
    const char * begin = json;
    char * name;
    struct jsonValue value;
    struct LexemeData data;
    size_t bytes_read;
    if (!next_lexeme(json, &data)) return 0;
    json += data.bytes_read;
    if (data.kind == JLK_R_BRACE) {
        *object = json_object_create(0);
        return json - begin;
    }
    *object = json_object_create(10);
    while (1) {
        if (JLK_STRING != data.kind) goto fail2;
        name = unescape_string(&data);
        if (!name) goto fail2;
        if (!next_lexeme(json, &data) || JLK_COLON != data.kind) goto fail1;
        json += data.bytes_read;
        json += bytes_read = json_parse_value(json, &value);
        if (!bytes_read) goto fail1;
        json_object_add_pair(*object, name, value);
        if (!next_lexeme(json, &data)) goto fail2;
        json += data.bytes_read;
        if (JLK_R_BRACE == data.kind) break;
        if (JLK_COMMA != data.kind) goto fail2;
        if (!next_lexeme(json, &data)) goto fail2;
        json += data.bytes_read;
    }
    return json - begin;
fail1:
    json_free(name);
fail2:
    json_object_free(*object);
    *object = NULL;
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
    size_t new_capacity;
    if (array->size == array->capacity) {
        new_capacity = 2 * array->capacity;
        new_values = json_malloc(new_capacity * sizeof(struct jsonValue));
        if (!new_values) return 0;
        memcpy(new_values, array->values, array->capacity * sizeof(struct jsonValue));
        json_free(array->values);
        array->capacity = new_capacity;
        array->values = new_values;
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

extern void json_array_traverse(struct jsonArray * array,
        void (*action)(size_t, struct jsonValue)) {
    size_t i;
    for (i = 0; i < array->size; ++i) {
        action(i, array->values[i]);
    }
}

// '[' was read
static size_t json_parse_array(const char * json, struct jsonArray ** array) {
    const char * begin = json;
    struct jsonValue value;
    struct LexemeData data;
    size_t bytes_read;
    if (!next_lexeme(json, &data)) return 0;
    // don't move as this is just prediction
    // json += data.bytes_read;
    if (JLK_R_SQUARE_BRACKET == data.kind) {
        *array = json_array_create(0);
        return json - begin;
    }
    *array = json_array_create(4);
    while (1) {
        json += bytes_read = json_parse_value(json, &value);
        if (!bytes_read) goto fail;
        if (!next_lexeme(json, &data)) goto fail;
        json += data.bytes_read;
        if (!json_array_add(*array, value)) goto fail;
        if (JLK_R_SQUARE_BRACKET == data.kind) break;
        if (JLK_COMMA != data.kind) goto fail;
    }
    return json - begin;
fail:
    json_array_free(*array);
    *array = NULL;
    return 0;
}

extern size_t json_parse_value(const char * json, struct jsonValue * value) {
    const char * begin = json;
    char * number_end;
    struct LexemeData data;
    struct jsonValue ret;
    size_t bytes_read;
    if (!next_lexeme(json, &data)) return 0;
    json += data.bytes_read;
    switch (data.kind) {
    case JLK_L_BRACE:
        ret.kind = JVK_OBJ;
        bytes_read = json_parse_object(json, &ret.obj);
        if (!bytes_read) return 0;
        json += bytes_read;
        break;
    case JLK_L_SQUARE_BRACKET:
        ret.kind = JVK_ARR;
        bytes_read = json_parse_array(json, &ret.arr);
        if (!bytes_read) return 0;
        json += bytes_read;
        break;
    case JLK_TRUE:
        ret.kind = JVK_BOOL;
        ret.bul = 1;
        break;
    case JLK_FALSE:
        ret.kind = JVK_BOOL;
        ret.bul = 0;
        break;
    case JLK_NULL:
        ret.kind = JVK_NULL;
        break;
    case JLK_STRING:
        ret.kind = JVK_STR;
        ret.str = unescape_string(&data);
        if (!ret.str) return 0;
        break;
    case JLK_NUMBER:
        ret.kind = JVK_NUM;
        ret.num = strtod(data.begin, &number_end);
        assert(number_end == json);
        break;
    default: return 0;
    }
    *value = ret;
    return json - begin;
}

extern void json_value_free(struct jsonValue value) {
    switch (value.kind) {
    case JVK_OBJ: json_object_free(value.obj); break;
    case JVK_ARR: json_array_free(value.arr);  break;
    case JVK_STR: json_free(value.str);        break;
    default:;
    }
}
