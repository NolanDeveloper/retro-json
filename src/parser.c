#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "json.h"
#include "json_internal.h"

static bool parse_value(struct jsonValue *value);

thread_local const char *json_begin; //!< holds start of json string during json_parse recursive calls 
thread_local const char *json_end; 
thread_local const char *json_it; 

extern struct jsonValue *json_parse(const char *json) {
    if (!json) {
        errorf("json == NULL");
        goto fail;
    }
    struct jsonValue *value = json_malloc(sizeof(struct jsonValue));
    if (!value) {
        goto fail;
    }
    json_begin = json_it = json;
    json_end = json + strlen(json);
    if (!parse_value(value)) {
        goto fail;
    }
    json_begin = json_it = json_end = NULL;
    return value;
fail:
    json_free(value);
    return NULL;
}

static char peek(void) {
    if (json_it < json_end) {
        return *json_it;
    }
    return '\0';
}

static void next_char(void) {
    assert(json_it < json_end);
    ++json_it;
}

static void skip_spaces(void) {
    while (isspace(peek())) {
        next_char();
    }
}

static bool consume(const char *str) {
    size_t n = strlen(str);
    if ((size_t) (json_end - json_it) < n) {
        errorf("unexpected end of input");
        return false;
    }
    if (strncmp(str, json_it, n)) {
        errorf("'%s' was expected", str);
        return false;
    }
    json_it += n;
    return true;
}

/*! Appends utf-32 char to utf-8 code unit sequence in jsonString. */
static bool append_unicode_code_point(struct jsonString *unescaped, uint32_t hex) {
    if (hex <= 0x007f) {
        return json_string_append(unescaped, (char) hex);
    } else if (0x0080 <= hex && hex <= 0x07ff) {
        return json_string_append(unescaped, (char) (0xc0 | hex >> 6))
            && json_string_append(unescaped, (char) (0x80 | (hex & 0x3f)));
    } else {
        return json_string_append(unescaped, (char) (0xe0 | hex >> 12))
            && json_string_append(unescaped, (char) (0x80 | ((hex >> 6) & 0x3f)))
            && json_string_append(unescaped, (char) (0x80 | (hex & 0x3f)));
    }
}

static bool parse_string(struct jsonString *string) {
    assert('"' == peek());
    next_char();
    while (1) {
        switch (peek()) {
        case '"':
            if (!json_string_append(string, '\0')) {
                return false;
            }
            if (!json_string_shrink(string)) {
                return false;
            }
            next_char();
            return true;
        case '\\':
            next_char();
            switch (peek()) {
            case '\\': 
            case '"': 
            case '/': 
                if (!json_string_append(string, peek())) {
                    return false;
                }
                next_char();
                break;
            case 'b': 
                if (!json_string_append(string, '\b')) {
                    return false;
                }
                next_char();
                break;
            case 'f': 
                if (!json_string_append(string, '\f')) {
                    return false;
                }
                next_char();
                break;
            case 'n': 
                if (!json_string_append(string, '\n')) {
                    return false;
                }
                next_char();
                break;
            case 'r':
                if (!json_string_append(string, '\r')) {
                    return false;
                }
                next_char();
                break;
            case 't':
                if (!json_string_append(string, '\t')) {
                    return false;
                }
                next_char();
                break;
            case 'u': {
                char buf[5];
                next_char();
                for (uint8_t i = 0; i < sizeof(buf) - 1; ++i) {
                    buf[i] = peek();
                    next_char();
                }
                buf[sizeof(buf) - 1] = '\0';
                char *end;
                uint32_t hex = strtoul(buf, &end, 16);
                if (end != &buf[4]) {
                    errorf("Bad Unicode escape sequence.");
                    return false;
                }
                if (hex >> 11ul == 0x1B) { /* this is utf-16 surrogate pair either high or low */
                    bool is_little_end = (hex >> 10ul) & 1ul;
                    if (consume("\\u")) {
                        errorf("Bad Unicode escape sequence.");
                        return false;
                    }
                    for (uint8_t i = 0; i < sizeof(buf) - 1; ++i) {
                        buf[i] = peek();
                        next_char();
                    }
                    buf[sizeof(buf) - 1] = '\0';
                    uint32_t surrogate = strtoul(buf, &end, 16);
                    if (end != &buf[4]) {
                        errorf("Bad Unicode escape sequence.");
                        return false;
                    }
                    if (((surrogate >> 10ul) & 1ul) == is_little_end) {
                        errorf("Bad Unicode escape sequence.");
                        return false;
                    }
                    hex       &= (1ul << 10ul) - 1ul;
                    surrogate &= (1ul << 10ul) - 1ul;
                    if (is_little_end) {
                        hex |= surrogate << 10ul;
                    } else {
                        hex = (hex << 10ul) | surrogate;
                    }
                    hex += 0x10000;
                }
                if (!append_unicode_code_point(string, hex)) {
                    return false;
                }
                break;
            }
            default:
                errorf("Unknown escape sequence.");
                return false;
            }
            break;
        default: {
            int left = utf8_bytes_left[(unsigned char) peek()];
            if (left < 0) {
                errorf("Invalid UTF-8 sequence.");
                return false;
            }
            if (!json_string_append(string, peek())) {
                return false;
            }
            next_char();
            for (int i = 0; i < left; ++i) {
                if (TR != utf8_bytes_left[(unsigned char) peek()]) {
                    errorf("Invalid UTF-8 sequence.");
                    return false;
                }
                if (!json_string_append(string, peek())) {
                    return false;
                }
                next_char();
            }
            break;
        }}
    }
}

static bool parse_object(struct jsonObject *object) {
    struct jsonString *key = NULL;
    struct jsonValue *value = NULL;
    assert('{' == peek());
    next_char();
    skip_spaces();
    if ('}' == peek()) {
        return true;
    }
    while (1) {
        key = json_malloc(sizeof(struct jsonString));
        if (!key) {
            goto fail;
        }
        json_string_init(key);
        skip_spaces();
        if (!parse_string(key)) {
            goto fail;
        }
        skip_spaces();
        if (!consume(":")) {
            goto fail;
        }
        value = json_malloc(sizeof(struct jsonValue));
        if (!value) {
            goto fail;
        }
        if (!parse_value(value)) {
            goto fail;
        }
        if (!json_object_add(object, key, value)) {
            goto fail;
        }
        key = NULL;
        value = NULL;
        skip_spaces();
        if ('}' == peek()) {
            assert(consume("}"));
            return true;
        }
        if (!consume(",")) {
            goto fail;
        }
    }
    assert(false);
fail:
    json_string_free_internal(key);
    json_free(key);
    json_value_free(value);
    json_object_free_internal(object);
    return false;
}

static bool parse_array(struct jsonArray *array) {
    struct jsonValue *value = NULL;
    assert('[' == peek());
    next_char();
    skip_spaces();
    if (']' == peek()) {
        return true;
    }
    while (1) {
        value = json_malloc(sizeof(struct jsonValue));
        if (!value) {
            goto fail;
        }
        if (!parse_value(value)) {
            goto fail;
        }
        if (!json_array_append(array, value)) {
            goto fail;
        }
        value = NULL;
        skip_spaces();
        if (']' == peek()) {
            assert(consume("]"));
            return true;
        }
        if (!consume(",")) {
            goto fail;
        }
    }
    assert(false);
fail:
    json_value_free(value);
    return false;
}

static bool parse_value_object(struct jsonValue *value) {
    value->kind = JVK_OBJ;
    json_object_init(&value->v.object);
    return parse_object(&value->v.object);
}

static bool parse_value_array(struct jsonValue *value) {
    value->kind = JVK_ARR;
    json_array_init(&value->v.array);
    return parse_array(&value->v.array);
}

static bool parse_value_true(struct jsonValue *value) {
    if (!consume("true")) {
        return false;
    }
    value->kind = JVK_BOOL;
    value->v.boolean = true;
    return true;
}

static bool parse_value_false(struct jsonValue *value) {
    if (!consume("false")) {
        return false;
    }
    value->kind = JVK_BOOL;
    value->v.boolean = false;
    return true;
}

static bool parse_value_null(struct jsonValue *value) {
    if (!consume("null")) {
        return false;
    }
    value->kind = JVK_NULL;
    return true;
}

static bool parse_value_string(struct jsonValue *value) {
    json_string_init(&value->v.string);
    value->kind = JVK_STR;
    return parse_string(&value->v.string);
}

static bool parse_value_number(struct jsonValue *value) {
    value->kind = JVK_NUM;
    char *end;
    value->v.number = strtod(json_it, &end);
    if (value->v.number == HUGE_VAL) {
        value->v.number = INFINITY;
    } else if (value->v.number == -HUGE_VAL) {
        value->v.number = -INFINITY;
    }
    json_it = end;
    return true;
}

static bool parse_value(struct jsonValue *value) {
    skip_spaces();
    switch (peek()) {
    case '{':
        return parse_value_object(value);
    case '[':
        return parse_value_array(value);
    case 't':
        return parse_value_true(value);
    case 'f':
        return parse_value_false(value);
    case 'n':
        return parse_value_null(value);
    case '"':
        return parse_value_string(value);
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return parse_value_number(value);
    default:
        errorf("unknown symbol");
        return false;
    }
}

