// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define ARRAY_INITIAL_CAPACITY 4
#define OBJECT_INITIAL_CAPACITY 8

enum jsonLexemeKind { 
    JS_ERROR, JS_L_BRACE, JS_R_BRACE, JS_L_SQUARE_BRACKET, JS_R_SQUARE_BRACKET, 
    JS_COMMA, JS_COLON, JS_TRUE, JS_FALSE, JS_NULL, JS_STRING, JS_NUMBER, 
};

struct jsonLexemeData {
    size_t bytes_read;      //  - set for all
    const char * begin;     //  - set for JS_STRING and JS_NUMBER
                            // for JS_STRING begin points to first char inside quotes
                            // for JS_NUMBER begin points to first char of number
    size_t measured_length; //  - set for string
};

static int utf8_bytes_left[256] = {
    [ 0x00 ... 0x7f ] =  0,
    [ 0x80 ... 0xbf ] = -1,
    [ 0xc0 ... 0xdf ] =  1,
    [ 0xe0 ... 0xef ] =  2,
    [ 0xf0 ... 0xf7 ] =  3,
    [ 0xf8 ... 0xff ] = -1,
};

static struct jsonLexemeData lexeme_data;

static size_t get_utf8_code_point_length(int32_t hex) {
    if (hex <= 0x007f)
        return 1;
    else if (0x0080 <= hex && hex <= 0x07ff)
        return 2;
    return 3;
}

static size_t read_string_lexeme(const char * json, struct jsonLexemeData * data) {
    enum State { 
        S_NEXT_CHAR, S_ONE_LEFT, S_TWO_LEFT, S_THREE_LEFT, S_ESCAPE
    }; 
    int state = S_NEXT_CHAR;
    int bytes_left;
    size_t measured_length = 0;
    const char * begin = json;
    uint32_t hex = 0;
    if (*json++ != '"') return 0;
    data->begin = json;
    while (1) {
        switch (state) {
        case S_NEXT_CHAR: 
            bytes_left = utf8_bytes_left[*(unsigned char *)json];
            if (-1 == bytes_left) return 0;
            switch (*json) {
            case '"': 
                ++json;
                data->measured_length = measured_length + 1;
                return json - begin;
            case '\\': state = S_ESCAPE; break;
            default: ++measured_length; state = S_NEXT_CHAR + bytes_left; break;
            }
            ++json;
            break;
        case S_THREE_LEFT: 
            if (*(unsigned char *)(json++) >> 6 != 0x2) return 0;
            ++measured_length;
        case S_TWO_LEFT:   
            if (*(unsigned char *)(json++) >> 6 != 0x2) return 0;
            ++measured_length;
        case S_ONE_LEFT:   
            if (*(unsigned char *)(json++) >> 6 != 0x2) return 0;
            ++measured_length;
            state = S_NEXT_CHAR;
            break;
        case S_ESCAPE:
            switch (*json) {
            case '\\': case '"': case '/': case 'b':
            case 'f':  case 'n': case 'r': case 't': 
                ++measured_length;
                ++json; 
                break;
            case 'u': 
                for (int i = 0; i < 4; ++i) {
                    char c = *++json;
                    if (!isxdigit(c)) return 0;
                    hex <<= 4;
                         if (c < 'A') hex |= c - '0';
                    else if (c < 'a') hex |= c - 'A' + 10;
                    else              hex |= c - 'a' + 10;
                }
                measured_length += get_utf8_code_point_length(hex);
                break;
            default: return 0;
            }
            state = S_NEXT_CHAR;
            break;
        default: exit(1); // unreachable statement
        }
    }
}

static size_t read_number_lexeme(const char * json, struct jsonLexemeData * data) {
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
        data->begin = begin;
        return json - begin;
    }
    ++json;
    if ('+' == *json || '-' == *json) ++json;
    while (isdigit(*json)) ++json;
    data->begin = begin;
    return json - begin;
}

static int is_prefix(const char * prefix, const char * str) {
    while (*prefix && *str && *prefix++ == *str++);
    return !!*prefix;
}

#define SINGLE_CHAR_LEXEME(c, k) \
    case (c): data->bytes_read = json + 1 - begin; return k;
#define CONCRETE_WORD_LEXEME(c, w, k) \
    case (c):  \
        if (is_prefix((w), json)) return JS_ERROR; \
        data->bytes_read = json - begin + sizeof(w) - 1; \
        return (k);
extern enum jsonLexemeKind json_next_lexeme(
        const char * json, struct jsonLexemeData * data) {
    const char * begin = json;
    size_t t;
    while (isspace(*json)) ++json;
    switch (*json) {
    case '\0': return JS_ERROR;
    SINGLE_CHAR_LEXEME('{', JS_L_BRACE)
    SINGLE_CHAR_LEXEME('}', JS_R_BRACE)
    SINGLE_CHAR_LEXEME('[', JS_L_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(']', JS_R_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(',', JS_COMMA)
    SINGLE_CHAR_LEXEME(':', JS_COLON)
    CONCRETE_WORD_LEXEME('t', "true",  JS_TRUE)
    CONCRETE_WORD_LEXEME('f', "false", JS_FALSE)
    CONCRETE_WORD_LEXEME('n', "null",  JS_NULL)
    case '"': 
        t = read_string_lexeme(json, data); 
        if (!t) return JS_ERROR;
        json += t;
        data->bytes_read = json - begin;
        return JS_STRING;
    default:
        t = read_number_lexeme(json, data); 
        if (!t) return JS_ERROR;
        json += t;
        data->bytes_read = json - begin;
        return JS_NUMBER;
    }
}

static size_t put_utf8_code_point(int32_t hex, char * out) {
    if (hex <= 0x007f) {
        *out = hex;
        return 1;
    } else if (0x0080 <= hex && hex <= 0x07ff) {
        *out++ = 0xc0 | hex >> 6;
        *out   = 0x80 | (hex & 0x3f);
        return 2;
    } else {
        *out++ = 0xe0 | hex >> 12;
        *out++ = 0x80 | ((hex >> 6) & 0x3f);
        *out   = 0x80 | (hex & 0x3f);
        return 3;
    } 
}

extern size_t unescape_string(const char * json, char * out) {
    enum State { S_NEXT_CHAR, S_ONE_LEFT, S_TWO_LEFT, S_THREE_LEFT, S_ESCAPE }; 
    enum State state = S_NEXT_CHAR;
    const char * begin = json;
    int bytes_left;
    int32_t hex;
    while (1) {
        switch (state) {
        case S_NEXT_CHAR: 
            bytes_left = utf8_bytes_left[*(unsigned char *)json];
            if (-1 == bytes_left) return 0;
            switch (*json) {
            case '"': *out = '\0'; return json + 1 - begin;
            case '\\': state = S_ESCAPE; json++; break;
            default:   
                state = S_NEXT_CHAR + bytes_left; 
                *out++ = *json++;
                break;
            }
            break;
        case S_THREE_LEFT: 
            if (*(unsigned char *)json >> 6 != 0x2) return 0;
            *out++ = *json++;
        case S_TWO_LEFT:   
            if (*(unsigned char *)json >> 6 != 0x2) return 0;
            *out++ = *json++;
        case S_ONE_LEFT:   
            if (*(unsigned char *)json >> 6 != 0x2) return 0;
            *out++ = *json++;
            state = S_NEXT_CHAR;
            break;
        case S_ESCAPE:
            state = S_NEXT_CHAR;
            switch (*json++) {
            case '\\': *out++ = '\\'; break;
            case  '"': *out++ =  '"'; break;
            case  '/': *out++ =  '/'; break;
            case  'b': *out++ = '\b'; break;
            case  'f': *out++ = '\f'; break;
            case  'n': *out++ = '\n'; break;
            case  'r': *out++ = '\r'; break;
            case  't': *out++ = '\t'; break;
            case  'u':
                hex = 0;
                for (int i = 0; i < 4; ++i) {
                    char c = *json++;
                    if (!isxdigit(c)) return 0;
                    hex <<= 4;
                         if (c < 'A') hex |= c - '0';
                    else if (c < 'a') hex |= c - 'A' + 10;
                    else              hex |= c - 'a' + 10;
                }
                out += put_utf8_code_point(hex, out);
                break;
            default: return 0;
            }
            break;
        default: exit(1); // unreachable statement
        }
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
    while (object->buckets[h].key) {
        if (!strcpy(key, object->buckets[h].key)) return 0;
        h = (h + 1) % object->capacity;
    }
    object->buckets[h].key = key;
    object->buckets[h].value = value;
    ++object->size;
    return 0;
}

static int json_object_grow(struct jsonObject * object) {
    if (!object->size) return 0; // empty once - empty forever
    size_t old_capacity = object->capacity;
    struct jsonPair * old_buckets = object->buckets;
    size_t i;
    object->size = 0;
    object->capacity *= 2;
    object->buckets = json_malloc(object->capacity * sizeof(struct jsonPair));
    if (!object->buckets) return 0;
    memset(object->buckets, 0, object->capacity * sizeof(struct jsonPair));
    for (i = 0; i < old_capacity; ++i) {
        if (!old_buckets[i].key) continue;
        json_object_add_pair(object, old_buckets[i].key, old_buckets[i].value);
    }
    json_free(old_buckets);   
    return 1;
}

static struct jsonObject * json_object_create(size_t initial_capacity) {
    struct jsonObject * object = json_malloc(sizeof(struct jsonObject));
    if (!object) return 0;
    object->capacity = initial_capacity;
    object->size = 0;
    if (!initial_capacity) {
        object->buckets = NULL;
        return object;
    } 
    object->buckets = json_malloc(initial_capacity * sizeof(struct jsonPair));
    memset(object->buckets, 0, initial_capacity * sizeof(struct jsonPair));
    return object->buckets ? object : 0;
}

extern struct jsonValue json_object_value_at(struct jsonObject * object, 
        const char * key) {
    size_t h = hash((unsigned char *)key) % object->capacity;
    struct jsonValue value;
    while (object->buckets[h].key && strcmp(object->buckets[h].key, key)) {
        h = (h + 1) % object->capacity;
    }
    if (object->buckets[h].key) return object->buckets[h].value;
    memset(&value, 0, sizeof(struct jsonValue));
    return value;
}

static void json_object_free(struct jsonObject * object) {
    size_t i;
    if (object->buckets) {
        for (i = 0; i < object->capacity; ++i) {
            if (!object->buckets[i].key) continue;
            json_free(object->buckets[i].key);
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
        if (JS_STRING != kind) goto fail2;
        name = json_malloc(lexeme_data.measured_length);
        if (!name) goto fail2;
        if (!unescape_string(lexeme_data.begin, name)) goto fail1;
        if (JS_COLON != json_next_lexeme(json, &lexeme_data)) goto fail1;
        json += lexeme_data.bytes_read;
        bytes_read = json_parse_value(json, &value);
        if (!bytes_read) goto fail1;
        json += bytes_read;
        json_object_add_pair(*object, name, value);
        kind = json_next_lexeme(json, &lexeme_data);
        json += lexeme_data.bytes_read;
        if (JS_R_BRACE == kind) break;
        if (JS_COMMA != kind) goto fail2;
        kind = json_next_lexeme(json, &lexeme_data);
        json += lexeme_data.bytes_read;
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
    if (array->size == array->capacity) {
        if (!array->capacity) {
            array->capacity = ARRAY_INITIAL_CAPACITY;
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
    *array = NULL;
    return 0;
}

extern size_t json_parse_value(const char * json, struct jsonValue * value) {
    const char * begin = json;
    struct jsonValue ret;
    size_t bytes_read;
    switch (json_next_lexeme(json, &lexeme_data)) {
    case JS_L_BRACE: 
        ret.kind = JVK_OBJ;
        json += lexeme_data.bytes_read;
        bytes_read = json_parse_object(json, &ret.obj);
        if (!bytes_read) return 0;
        json += bytes_read;
        *value = ret;
        return json - begin;
    case JS_L_SQUARE_BRACKET: 
        ret.kind = JVK_ARR;
        json += lexeme_data.bytes_read;
        bytes_read = json_parse_array(json, &ret.arr);
        if (!bytes_read) return 0;
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
        unescape_string(lexeme_data.begin, ret.str);
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
