#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "json_internal.h"

static thread_local size_t depth;
static thread_local size_t max_depth = 128;

thread_local unsigned long line;
thread_local unsigned long column;
thread_local const char *input_buffer;
thread_local size_t input_buffer_size;
thread_local size_t offset;

static struct jsonValue *parse_value(void);

extern void parser_begin(const char *buffer, size_t n) {
    assert(buffer);
    set_error(NULL);
    depth = 0;
    line = 1;
    column = 1;
    input_buffer = buffer;
    input_buffer_size = n;
    offset = 0;
}

extern void parser_end(void) {
    input_buffer = NULL;
}

static int peek(void) {
    if (input_buffer_size <= offset) {
        return EOF;
    }
    return input_buffer[offset];
}

static int next_char(void) {
    int c = peek();
    if (c == '\n') {
        ++line;
        column = 0;
    }
    ++column;
    ++offset;
    return c;
}

extern void skip_spaces(void) {
    int c;
    while (EOF != (c = next_char()) && c && strchr("\x20\x09\x0A\x0D", c)) {
    }
    --offset;
}

static bool consume_optionally(const char *str) {
    size_t n = strlen(str);
    size_t left = input_buffer_size - offset;
    if (left < n || strncmp(&input_buffer[offset], str, n)) {
        return false;
    }
    offset += n;
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
        if (!string_append(string, (char) c8[i])) {
            return false;
        }
    }
    return true;
}

static bool parse_hex4(char32_t *out) {
    char buf[5];
    for (int i = 0; i < 4; ++i) {
        if (EOF == (buf[i] = next_char())) {
            errorf("bad Unicode escape sequence");
            return false;
        }
    }
    buf[4] = '\0';
    char *end;
    *out = strtoul(buf, &end, 16);
    if (end != &buf[4]) {
        errorf("bad Unicode escape sequence");
        return false;
    }
    return true;
}

static bool parse_string(struct jsonString *string) {
    if (!consume("\"")) {
        return false;
    }
    while (1) {
        int c;
        switch (c = next_char()) {
        case EOF:
            errorf("unexpected end of input");
            return false;
        case '"':
            if (!string_append(string, '\0')) {
                return false;
            }
            if (!string_shrink(string)) {
                return false;
            }
            return true;
        case '\x00':
            errorf("unescaped null character");
            return false;
        case '\\':
            switch (c = next_char()) {
            case '\\':
            case '"':
            case '/':
                if (!string_append(string, c)) {
                    return false;
                }
                break;
            case 'b':
                if (!string_append(string, '\b')) {
                    return false;
                }
                break;
            case 'f':
                if (!string_append(string, '\f')) {
                    return false;
                }
                break;
            case 'n':
                if (!string_append(string, '\n')) {
                    return false;
                }
                break;
            case 'r':
                if (!string_append(string, '\r')) {
                    return false;
                }
                break;
            case 't':
                if (!string_append(string, '\t')) {
                    return false;
                }
                break;
            case 'u': {
                char32_t p = 0;
                if (!parse_hex4(&p)) {
                    return false;
                }
                enum c16Type type = c16type((char16_t) p);
                size_t prev_offset = offset;
                if (type == UTF16_SURROGATE_HIGH && consume_optionally("\\u")) {
                    char32_t next = 0;
                    if (!parse_hex4(&next)) {
                        return false;
                    }
                    if (c16type(next) != UTF16_SURROGATE_LOW) {
                        offset = prev_offset;
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
            if ((unsigned char) c <= 0x1F) {
                errorf("unescaped control character");
                return false;
            }
            string_append(string, c);
            break;
        }
    }
}

static bool parse_object(struct jsonObject *object) {
    struct jsonString *key = NULL;
    struct jsonValue *value = NULL;
    if (!consume("{")) {
        return false;
    }
    skip_spaces();
    if (consume_optionally("}")) {
        return true;
    }
    while (1) {
        key = string_create();
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
        if (!object_add(object, key, value)) {
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
    string_free_internal(key);
    json_free(key);
    json_value_free(value);
    object_free_internal(object);
    return false;
}

static bool parse_array(struct jsonArray *array) {
    struct jsonValue *value = NULL;
    if (!consume("[")) {
        return false;
    }
    skip_spaces();
    if (consume_optionally("]")) {
        return true;
    }
    while (true) {
        value = parse_value();
        if (!value) {
            goto fail;
        }
        if (!array_append(array, value)) {
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
    array_free_internal(array);
    return false;
}

static bool parse_value_object(struct jsonValue *value) {
    value->kind = JVK_OBJ;
    object_init(&value->v.object);
    return parse_object(&value->v.object);
}

static bool parse_value_array(struct jsonValue *value) {
    value->kind = JVK_ARR;
    array_init(&value->v.array);
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
    string_init(&value->v.string);
    value->kind = JVK_STR;
    bool result = parse_string(&value->v.string);
    if (!result) {
        string_free_internal(&value->v.string);
    }
    return result;
}

static thread_local char number_buffer[4096];
static thread_local size_t number_size;

static bool number_append(char c) {
    if (number_size >= sizeof(number_buffer) - 1) {
        errorf("number is too long");
        return false;
    }
    number_buffer[number_size++] = c;
    return true;
}

static bool digit(void) {
    int c = peek();
    if ('0' <= c && c <= '9') {
        if (!number_append(c)) {
            return false;
        }
        next_char();
        return true;
    }
    errorf("a digit was expected");
    return false;
}

static bool digits(void) {
    int c = peek();
    while ('0' <= c && c <= '9') {
        if (!number_append(c)) {
            return false;
        }
        next_char();
        c = peek();
    }
    return true;
}

static bool integer(void) {
    if ('-' == peek()) {
        if (!number_append('-')) {
            return false;
        }
        next_char();
    }
    int c = peek();
    if ('0' == c) {
        if (!number_append('0')) {
            return false;
        }
        next_char();
        return true;
    }
    return digit() && digits();
}

static bool fraction(void) {
    int c = peek();
    if ('.' == c) {
        if (!number_append('.')) {
            return false;
        }
        next_char();
        return digit() && digits();
    }
    return true;
}

static bool sign(void) {
    int c = peek();
    if ('+' == c || '-' == c) {
        if (!number_append(c)) {
            return false;
        }
        next_char();
    }
    return true;
}

static bool exponent(void) {
    int c = peek();
    if ('e' == c || 'E' == c) {
        if (!number_append(c)) {
            return false;
        }
        next_char();
        return sign() && digit() && digits();
    }
    return true;
}

static bool parse_value_number(struct jsonValue *value) {
    value->kind = JVK_NUM;
    number_size = 0;
    if (!integer() || !fraction() || !exponent()) {
        return false;
    }
    if (!number_append('\0')) {
        return false;
    }
    int match = 0;
    int n = sscanf(number_buffer, "%lf%n", &value->v.number, &match);
    if (1 != n) {
        errorf("failed to parse number");
        return false;
    }
    return true;
}

static struct jsonValue *parse_value(void) {
    ++depth;
    if (depth > max_depth) {
        errorf("recursion depth exceeded");
        return NULL;
    }
    struct jsonValue *value = json_malloc(sizeof(struct jsonValue));
    if (!value) {
        return NULL;
    }
    skip_spaces();
    bool result;
    int c = peek();
    switch (c) {
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

extern struct jsonValue *parse_json_text(bool all) {
    struct jsonValue *value = parse_value();
    skip_spaces();
    if (all && value && EOF != next_char()) {
        errorf("trailing bytes");
        json_value_free(value);
        value = NULL;
    }
    return value;
}
