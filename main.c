// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

void * (*json_malloc)(size_t) = malloc;
void (*json_free)(void *) = free;

void print_offset(int offset) { int i; for (i = 0; i < offset; ++i) printf("    "); }

void print_json_value(struct jsonValue value, int offset);

void print_json_object(struct jsonObject * object, int offset) {
    size_t i;
    int not_first = 0;
    printf("{\n");
    for (i = 0; i < object->capacity; ++i) {
        if (!object->keys[i]) continue;
        if (not_first) printf(",\n");
        print_offset(offset + 1);
        printf("\"%s\" : ", object->keys[i]);
        print_json_value(object->values[i], offset + 1);
        not_first = 1;
    }
    puts("");
    print_offset(offset);
    printf("}");
}

void print_json_array(struct jsonArray * array, int offset) {
    size_t i;
    printf("[\n");
    for (i = 0; i < array->size; ++i) {
        print_offset(offset + 1);
        print_json_value(array->values[i], offset + 1);
        if (i != array->size - 1) printf(",\n");
    }
    puts("");
    print_offset(offset);
    printf("]");
}

void print_json_value(struct jsonValue value, int offset) {
    switch (value.kind) {
    case JVK_STR: printf("\"%s\"", value.str); break;
    case JVK_NUM: printf("%lf", value.num); break;
    case JVK_OBJ: print_json_object(value.obj, offset); break;
    case JVK_ARR: print_json_array(value.arr, offset); break;
    case JVK_BOOL: printf(value.bul ? "true" : "false"); break;
    case JVK_NULL: printf("null"); break;
    }
}

#define BUFFER_SIZE (32 * 1024)

int from_stdin() {
    char * buffer = malloc(BUFFER_SIZE);
    struct jsonValue value;
    size_t bytes_read = fread(buffer, BUFFER_SIZE, sizeof(char), stdin);
    if (bytes_read == BUFFER_SIZE) return 1;
    if (!json_parse_value(buffer, &value)) return 1;
    print_json_value(value, 0);
    puts("");
    json_value_free(value);
    free(buffer);
    return 0;
}

int from_string() {
    const char * json = "[ { \"name\" : \"101 (MM4)\", \"id\" : 1 } ]";
    struct jsonValue value;
    if (!json_parse_value(json, &value)) return 1;
    print_json_value(value, 0);
    json_value_free(value);
    puts("");
    return 0;
}

int main() {
    return from_stdin();
}
