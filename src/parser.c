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
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

static struct jsonValue *parse_value(void);

thread_local const char *json_begin; //!< holds start of json string during json_parse recursive calls 
thread_local const char *json_it; 

static thread_local size_t depth;
static thread_local size_t max_depth = 1000;

extern struct jsonValue *json_parse(const char *json) {
    if (!json) {
        errorf("json == NULL");
        return NULL;
    }
    json_begin = json_it = json;
    struct jsonValue *value = parse_value();
    json_begin = json_it = NULL;
    return value;
}

static void skip_spaces(void) {
    while (isspace(*json_it)) {
        ++json_it;
    }
}

static bool consume_optionally(const char *str) {
    size_t n = strlen(str);
    if (strncmp(str, json_it, n)) {
        return false;
    }
    json_it += n;
    return true;
}

static bool consume(const char *str) {
    bool result = consume_optionally(str);
    if (!result) {
        errorf("'%s' was expected", str);
    }
    return result;
}

/*! Appends UTF-32 char to UTF-8 code unit sequence in jsonString. */
static bool append_unicode_code_point(struct jsonString *string, char32_t c32) {
    char c8[4];
    int n = 0;
    if (!c32toc8(c32, &n, c8)) {
        errorf("illegal UTF-8 sequence");
        return false;
    }
    for (int i = 0; i < n; ++i) {
        if (!json_string_append(string, (char) c8[i])) {
            return false;
        }
    }
    return true;
}

static bool parse_hex4(char32_t *out) {
    char buf[5];
    strncpy(buf, json_it, 4);
    buf[4] = '\0';
    char *end;
    *out = strtoul(buf, &end, 16);
    if (end != &buf[4]) {
        errorf("bad Unicode escape sequence");
        return false;
    }
    json_it += 4;
    return true;
}

static bool parse_string(struct jsonString *string) {
    if (!consume("\"")) {
        return false;
    }
    char *end_or_slash = strpbrk(json_it, "\"\\");
    if (!end_or_slash) {
        errorf("unterminated string");
        return false;
    }
    if (*end_or_slash == '\"') {
        size_t n = end_or_slash - json_it;
        json_string_init_mem(string, json_it, n);
        json_it = end_or_slash + 1;
        return true;
    }
    while (1) {
        switch (*json_it) {
        case '"':
            if (!json_string_append(string, '\0')) {
                return false;
            }
            if (!json_string_shrink(string)) {
                return false;
            }
            ++json_it;
            return true;
        case '\\':
            ++json_it;
            switch (*json_it) {
            case '\\': 
            case '"': 
            case '/': 
                if (!json_string_append(string, *json_it)) {
                    return false;
                }
                ++json_it;
                break;
            case 'b': 
                if (!json_string_append(string, '\b')) {
                    return false;
                }
                ++json_it;
                break;
            case 'f': 
                if (!json_string_append(string, '\f')) {
                    return false;
                }
                ++json_it;
                break;
            case 'n': 
                if (!json_string_append(string, '\n')) {
                    return false;
                }
                ++json_it;
                break;
            case 'r':
                if (!json_string_append(string, '\r')) {
                    return false;
                }
                ++json_it;
                break;
            case 't':
                if (!json_string_append(string, '\t')) {
                    return false;
                }
                ++json_it;
                break;
            case 'u': {
                ++json_it;
                char32_t p = 0;
                if (!parse_hex4(&p)) {
                    return false;
                }
                enum c16Type type = c16type((char16_t) p);
                const char *t = json_it;
                if (type == UTF16_SURROGATE_HIGH && consume_optionally("\\u")) {
                    char32_t next = 0;
                    if (!parse_hex4(&next)) {
                        return false;
                    } 
                    if (c16type(next) != UTF16_SURROGATE_LOW) {
                        json_it = t;
                    } else {
                        p = c16pairtoc32(p, next);
                    }
                }
                if (!append_unicode_code_point(string, p)) {
                    return false;
                }
                break;
            }
            default:
                errorf("unknown escape sequence");
                return false;
            }
            break;
        default: 
            json_string_append(string, *json_it);
            ++json_it;
            break;
        }
    }
}

static bool parse_object(struct jsonObject *object) {
    struct jsonString *key = NULL;
    struct jsonValue *value = NULL;
    assert('{' == *json_it);
    ++json_it;
    skip_spaces();
    if (consume_optionally("}")) {
        return true;
    }
    while (1) {
        key = json_string_create();
        if (!key) {
            goto fail;
        }
        skip_spaces();
        if (!parse_string(key)) {
            goto fail;
        }
        skip_spaces();
        if (!consume(":")) {
            goto fail;
        }
        value = parse_value();
        if (!value) {
            goto fail;
        }
        if (!json_object_add(object, key, value)) {
            goto fail;
        }
        key = NULL;
        value = NULL;
        skip_spaces();
        if (consume_optionally("}")) {
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
    assert('[' == *json_it);
    ++json_it;
    skip_spaces();
    if (consume_optionally("]")) {
        return true;
    }
    while (true) {
        value = parse_value();
        if (!value) {
            goto fail;
        }
        if (!json_array_append(array, value)) {
            goto fail;
        }
        value = NULL;
        skip_spaces();
        if (consume_optionally("]")) {
            return true;
        }
        if (!consume(",")) {
            goto fail;
        }
    }
    assert(false);
fail:
    json_value_free(value);
    json_array_free_internal(array);
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
    bool result = parse_string(&value->v.string);
    if (!result) {
        json_string_free_internal(&value->v.string);
    }
    return result;
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

static struct jsonValue *parse_value(void) {
    ++depth;
    if (depth > max_depth) {
        --depth;
        return NULL;
    }
    struct jsonValue *value = json_malloc(sizeof(struct jsonValue));
    if (!value) {
        return NULL;
    }
    skip_spaces();
    bool result;
    switch (*json_it) {
    case '{':
        result = parse_value_object(value);
        break;
    case '[':
        result = parse_value_array(value);
        break;
    case 't':
        result = parse_value_true(value);
        break;
    case 'f':
        result = parse_value_false(value);
        break;
    case 'n':
        result = parse_value_null(value);
        break;
    case '"':
        result = parse_value_string(value);
        break;
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
        result = parse_value_number(value);
        break;
    default:
        errorf("json value was expected");
        result = false;
        break;
    }
    --depth;
    if (!result) {
        json_free(value);
        value = NULL;
    }
    return value;
}

