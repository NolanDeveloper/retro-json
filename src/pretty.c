#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include "json.h"

#define TAB_SIZE 4

static size_t print_json_value(char * out, size_t size, struct jsonValue * value, unsigned indent);

static size_t print_str(char * out, size_t size, const char * string) {
    size_t len;
    len = strlen(string);
    if (!out) return len;
    if (size < len) len = size;
    memcpy(out, string, len);
    return len;
}

static size_t print_indent(char * out, size_t size, unsigned indent) {
    unsigned t;
    if (!out) return indent;
    if (size < indent) indent = size;
    t = indent;
    while (t) {
        *out++ = ' ';
        --t;
    }
    return indent;
}

/* does not put null at the end */
static size_t print_json_string(char * out, size_t size, struct jsonString * string) {
    char buf[3];
    size_t n;
    char * p;
    n = 0;
    p = string->data;
    n += print_str(out ? out + n : NULL, size - n, "\"");
    while (*p) {
        if (*p == '\"' || *p == '\\') {
            buf[0] = '\\';
            buf[1] = *p;
            buf[2] = '\0';
        } else {
            buf[0] = *p;
            buf[1] = '\0';
        }
        n += print_str(out ? out + n : NULL, size - n, buf);
        ++p;
    }
    n += print_str(out ? out + n : NULL, size - n, "\"");
    return n;
}

/* does not put null at the end */
static size_t print_json_number(char * out, size_t size, double number) {
    size_t t;
    long n;
    if (isnan(number)) {
        n = snprintf(out, out ? size : 0, "null");
    } else {
        if (isinf(number) == 1) {
            number = DBL_MAX;
        } else if (isinf(number) == -1) {
            number = DBL_MIN;
        }
        if ((long)number == number) {
            n = snprintf(out, out ? size : 0, "%ld", (long) number);
        } else {
            n = snprintf(out, out ? size : 0, "%f", number);
        }
    }
    t = n < 0 ? 0 : (size_t) n;
    if (!out) return t;
    return t;
}

/* does not put null at the end */
static size_t print_json_object(char * out, size_t size, struct jsonObject * object, unsigned indent) {
    struct jsonObjectEntry * entry;
    size_t n;
    n = 0;
    if (!object->size) {
        n += print_str(out ? out + n : NULL, size - n, "{ }"); 
        return n;
    }
    n += print_str(out ? out + n : NULL, size - n, "{\n"); 
    entry = object->first;
    if (entry) {
        n += print_indent(out ? out + n : NULL, size - n, indent + TAB_SIZE);
        n += print_json_string(out ? out + n : NULL, size - n, entry->key);
        n += print_str(out ? out + n : NULL, size - n, ": ");
        n += print_json_value(out ? out + n : NULL, size - n, entry->value, indent + TAB_SIZE);
        entry = entry->next;
    }
    while (entry) {
        n += print_str(out ? out + n : NULL, size - n, ",\n");
        n += print_indent(out ? out + n : NULL, size - n, indent + TAB_SIZE);
        n += print_json_string(out ? out + n : NULL, size - n, entry->key);
        n += print_str(out ? out + n : NULL, size - n, ": ");
        n += print_json_value(out ? out + n : NULL, size - n, entry->value, indent + TAB_SIZE);
        entry = entry->next;
    }
    n += print_str(out ? out + n : NULL, size - n, "\n"); 
    n += print_indent(out ? out + n : NULL, size - n, indent);
    n += print_str(out ? out + n : NULL, size - n, "}"); 
    return n;
}

/* does not put null at the end */
static size_t print_json_array(char * out, size_t size, struct jsonArray * array, unsigned indent) {
    size_t n, i;
    n = 0;
    if (!array->size) {
        n += print_str(out ? out + n : NULL, size - n, "[ ]"); 
        return n;
    }
    n += print_str(out ? out + n : NULL, size - n, "[\n");
    for (i = 0; i < array->size; ++i) {
        if (i) n += print_str(out ? out + n : NULL, size - n, ",\n");
        n += print_indent(out ? out + n : NULL, size - n, indent + TAB_SIZE);
        n += print_json_value(out ? out + n : NULL, size - n, array->values[i], indent + TAB_SIZE);
    }
    n += print_str(out ? out + n : NULL, size - n, "\n"); 
    n += print_indent(out ? out + n : NULL, size - n, indent);
    n += print_str(out ? out + n : NULL, size - n, "]");
    return n;
}

/* does not put null at the end */
static size_t print_json_value(char * out, size_t size, struct jsonValue * value, unsigned indent) {
    switch (value->kind) {
    case JVK_STR:  return print_json_string(out, size, &value->v.string);
    case JVK_NUM:  return print_json_number(out, size, value->v.number);
    case JVK_OBJ:  return print_json_object(out, size, &value->v.object, indent);
    case JVK_ARR:  return print_json_array(out, size, &value->v.array, indent);
    case JVK_BOOL: return print_str(out, size, value->v.boolean ? "true" : "false");
    case JVK_NULL: 
    default:       return print_str(out, size, "null");
    }
}

/* puts null at the end */
/* returns number of bytes writen or that would have been written if out is NULL */
extern size_t json_pretty_print(char * out, size_t size, struct jsonValue * value) {
    size_t n;
    if (!value) {
        if (out && size) *out = '\0';
        return 1;
    }
    n = print_json_value(out, size - 1, value, 0);
    if (out) out[n] = '\0';
    return n + 1;
}
