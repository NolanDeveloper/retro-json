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
	for (char *p = string->data, *end = string->data + string->size - 1; p != end; ++p) {
		char c = *p;
		int n;
		if (ascii_only && (n = c8len(c)) > 1) {
			char16_t c16[2];
			char32_t c32;
			assert(c8toc32(p, &c32));
			c32toc16be(c32, c16);
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
			if ((unsigned char) c < 0x20) {
				jprintf("\\u%04x", (unsigned char) c);
			} else {
				jprintf("%c", c);
			}
			break;
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
        number = -DBL_MAX;
    }
    if (number == (long long) number) {
        jprintf("%lld", (long long) number);
    } else {
        jprintf("%.16g", number);
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
    for (size_t i = 0; i < object->capacity; ++i) {
        struct jsonObjectEntry *entry = &object->entries[i];
        if (!entry->key || entry->key == &key_deleted) {
            continue;
        }
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
