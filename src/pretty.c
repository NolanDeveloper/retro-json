#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include "json.h"

#define TAB "\t"

static thread_local char *out_begin;
static thread_local size_t out_size;
static thread_local size_t position;
static thread_local unsigned indent;

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
        jprintf(TAB);
    }
}

static void print_json_string(struct jsonString *string) {
    jprintf("\"");
    for (char c, *p = string->data; (c = *p); ++p) {
        if (c == '\"' || c == '\\') {
            jprintf("\\%c", c);
        } else {
            jprintf("%c", c);
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
    bool latch = false;
    for (struct jsonObjectEntry *entry = object->first; entry; entry = entry->next) {
        if (latch) {
            jprintf(",\n");
        }
        latch = true;
        print_indent();
        print_json_string(entry->key);
        jprintf(": ");
        ++indent;
        print_json_value(entry->value);
        --indent;
    }
    jprintf("\n");
    print_indent();
    jprintf("}");
}

static void print_json_array(struct jsonArray *array) {
    if (!array->size) {
        jprintf("[ ]");
        return;
    }
    jprintf("[\n");
    for (size_t i = 0; i < array->size; ++i) {
        if (i) {
            jprintf(",\n");
        }
        print_indent();
        ++indent;
        print_json_value(array->values[i]);
        --indent;
    }
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
