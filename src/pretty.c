#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

static thread_local char *out_begin;
static thread_local size_t out_size;
static thread_local size_t position;
static thread_local unsigned indent;

static thread_local const char *tab = "\t";
static thread_local bool ascii_only = true;

static void print_json_value(struct jsonValue *value);

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
    jprintf("\"");
    if (!ascii_only && !strchr(string->data, '\\') && !strchr(string->data, '\"')) {
        jprintf("%s", string->data);
    } else if (ascii_only) {
        for (char c, *p = string->data; (c = *p); ++p) {
            int n = u8len(c);
            if (n > 1) {
                char16_t u16[2];
                u32tou16le(u8tou32(p), u16);
                jprintf("\\u%04x", u16[0]);
                if (u16[1]) {
                    jprintf("\\u%04x", u16[1]);
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
                jprintf("%c", c);
                break;
            }
        } 
    } else {
        for (char c, *p = string->data; (c = *p); ++p) {
            if (c == '\"' || c == '\\') {
                jprintf("\\%c", c);
            } else {
                jprintf("%c", c);
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

static void print_json_value(struct jsonValue *value) {
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

extern size_t json_pretty_print(char *out, size_t size, struct jsonValue *value) {
    out_begin = out;
    out_size = size;
    position = 0;
    indent = 0;
    print_json_value(value);
    if (size) {
        out[size - 1] = '\0';
    }
    out_begin = NULL;
    return position;
}
