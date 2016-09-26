// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

void * (*json_malloc)(size_t) = malloc;
void (*json_free)(void *) = free;

void print_json_value(struct jsonValue value);

void print_json_object(struct jsonObject * object) {
    size_t i;
    int not_first = 0;
    printf("{ ");
    for (i = 0; i < object->capacity; ++i) {
        if (!object->buckets[i].name) continue;
        if (not_first) printf(", ");
        printf("\"%s\" : ", object->buckets[i].name);
        print_json_value(object->buckets[i].value);
        not_first = 1;
    }
    printf(" }");
}

void print_json_array(struct jsonArray * array) {
    size_t i;
    printf("[ ");
    for (i = 0; i < array->size; ++i) {
        print_json_value(array->values[i]);
        if (i != array->size - 1) printf(", ");
    }
    printf(" ]");
}

void print_json_value(struct jsonValue value) {
    switch (value.kind) {
    case JVK_STR: printf("\"%s\"", value.str); break;
    case JVK_NUM: printf("%lf", value.num); break;
    case JVK_OBJ: print_json_object(value.obj); break;
    case JVK_ARR: print_json_array(value.arr); break;
    case JVK_BOOL: printf(value.bul ? "true" : "false"); break;
    case JVK_NULL: printf("null"); break;
    }
}

int main() {
    const char * json = "{\"one\":1,\"two\":2}";
    struct jsonValue value;
    size_t bytes_read = json_parse_value(json, &value);
    if (!bytes_read) return 1;
    print_json_value(value);
    puts("");
    return 0;
}
