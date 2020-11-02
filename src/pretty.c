#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#include "json_internal.h"

static thread_local char *out_begin;
static thread_local size_t out_size;
static thread_local size_t position;
static thread_local unsigned indent;

static thread_local const char *tab = "\t";
static thread_local bool ascii_only = false;

extern void pretty_print_begin(char *out, size_t size) {
    out_begin = out;
    out_size = size;
    position = 0;
    indent = 0;
}

extern size_t pretty_print_end(void) {
    if (position < out_size) {
        out_begin[position] = '\0';
    } else {
        ++position;
    }
    out_begin = NULL;
    return position;
}

static void jprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t size = position < out_size ? out_size - position : 0;
    int n = vsnprintf(out_begin + position, size, fmt, args);
    va_end(args);
    position += n;
}

static void print_indent(void) {
    for (unsigned i = 0; i < indent; ++i) {
        jprintf("%s", tab);
    }
}

static void print_json_string(struct jsonString *string) {
    assert(string);
    jprintf("\"");
    const char * must_be_escaped = 
        "\"\\\x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
        "\x10\x11\x12\x13\x14\x15\x16\x17"
        "\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";
    if (!ascii_only && !strpbrk(string->data, must_be_escaped)) {
        jprintf("%s", string->data);
    } else {
        for (char c, *p = string->data; (c = *p); ++p) {
            int n;
            if (ascii_only && (n = c8len(c)) > 1) {
                char16_t c16[2];
                c32toc16be(c8toc32(p), c16);
                jprintf("\\u%04x", c16[0]);
                if (c16[1]) {
                    jprintf("\\u%04x", c16[1]);
                }
                p += n - 1;
                continue;
            }
            switch (c) {
            case '\"':
            case '\\':
            case '/':
                jprintf("\\%c", c);
                break;
            case '\b':
                jprintf("\\b");
                break;
            case '\f':
                jprintf("\\f");
                break;
            case '\n':
                jprintf("\\n");
                break;
            case '\r':
                jprintf("\\r");
                break;
            case '\t':
                jprintf("\\t");
                break;
            default:
                if ((unsigned char) c <= 0x1F) {
                    jprintf("%c", c);
                }
                jprintf("%c", c);
                break;
            }
        } 
    }
    jprintf("\"");
}

static void print_json_number(double number) {
    if (isnan(number)) {
        jprintf("null");
        return;
    } 
    if (isinf(number) == 1) {
        number = DBL_MAX;
    } else if (isinf(number) == -1) {
        number = DBL_MIN;
    }
    if ((long) number == number) {
        jprintf("%ld", (long) number);
    } else {
        jprintf("%f", number);
    }
}

static void print_json_object(struct jsonObject *object) {
    if (!object->size) {
        jprintf("{ }");
        return;
    }
    jprintf("{\n");
    ++indent;
    bool latch = false;
    for (struct jsonObjectEntry *entry = object->first; entry; entry = entry->next) {
        if (latch) {
            jprintf(",\n");
        }
        latch = true;
        print_indent();
        print_json_string(entry->key);
        jprintf(": ");
        print_json_value(entry->value);
    }
    --indent;
    jprintf("\n");
    print_indent();
    jprintf("}");
}

static void print_json_array(struct jsonArray *array) {
    if (!array->size) {
        jprintf("[ ]");
        return;
    }
    ++indent;
    jprintf("[\n");
    for (size_t i = 0; i < array->size; ++i) {
        if (i) {
            jprintf(",\n");
        }
        print_indent();
        print_json_value(array->values[i]);
    }
    --indent;
    jprintf("\n");
    print_indent();
    jprintf("]");
}

extern void print_json_value(struct jsonValue *value) {
    switch (value->kind) {
    case JVK_STR:  
        print_json_string(&value->v.string);
        break;
    case JVK_NUM:  
        print_json_number(value->v.number);
        break;
    case JVK_OBJ:  
        print_json_object(&value->v.object);
        break;
    case JVK_ARR:  
        print_json_array(&value->v.array);
        break;
    case JVK_BOOL: 
        jprintf(value->v.boolean ? "true" : "false");
        break;
    case JVK_NULL: 
    default:       
        jprintf("null");
        break;
    }
}
