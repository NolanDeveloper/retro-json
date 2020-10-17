#include <stddef.h>
#include <string.h>
#include <stdio.h>

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
    size_t n;
    n = 0;
    n += print_str(out ? out + n : NULL, size - n, "\"");
    n += print_str(out ? out + n : NULL, size - n, string->data);
    n += print_str(out ? out + n : NULL, size - n, "\"");
    return n;
}

/* does not put null at the end */
static size_t print_json_number(char * out, size_t size, double number) {
    static char buffer[10 * 1024];
    size_t t;
    long n;
    if ((long)number == number) {
        n = sprintf(buffer, "%ld", (long) number);
    } else {
        n = sprintf(buffer, "%f", number); /*!< @todo we are at c89 ooh this sucks so much GOD PLEASE KILL ME :-( */
    }
    t = n < 0 ? 0 : (size_t) n;
    if (!out) return t;
    if (t > size) t = size;
    memcpy(out, buffer, t);
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
    struct jsonArrayNode * node;
    size_t n;
    n = 0;
    if (!array->size) {
        n += print_str(out ? out + n : NULL, size - n, "[ ]"); 
        return n;
    }
    n += print_str(out ? out + n : NULL, size - n, "[\n");
    node = array->first;
    if (node) {
        n += print_indent(out ? out + n : NULL, size - n, indent + TAB_SIZE);
        n += print_json_value(out ? out + n : NULL, size - n, node->value, indent + TAB_SIZE);
        node = node->next;
    }
    while (node) {
        n += print_str(out ? out + n : NULL, size - n, ",\n");
        n += print_indent(out ? out + n : NULL, size - n, indent + TAB_SIZE);
        n += print_json_value(out ? out + n : NULL, size - n, node->value, indent + TAB_SIZE);
        node = node->next;
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
