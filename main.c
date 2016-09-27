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
        if (!object->buckets[i].key) continue;
        if (not_first) printf(",\n");
        print_offset(offset + 1);
        printf("\"%s\" : ", object->buckets[i].key);
        print_json_value(object->buckets[i].value, offset + 1);
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

int main() { 
    const char * json = "{\"menu\": {\r\n    \"header\": \"SVG Viewer\",\r\n"
        "\"items\": [\r\n        {\"id\": \"Open\"},\r\n        {\"id\":"
        "\"OpenNew\", \"label\": \"Open New\"},\r\n        null,\r\n"
        "{\"id\": \"ZoomIn\", \"label\": \"Zoom In\"},\r\n        {\"id\":"
        "\"ZoomOut\", \"label\": \"Zoom Out\"},\r\n        {\"id\":"
        "\"OriginalView\", \"label\": \"Original View\"},\r\n        null,\r\n"
        "{\"id\": \"Quality\"},\r\n        {\"id\": \"Pause\"},\r\n"
        "{\"id\": \"Mute\"},\r\n        null,\r\n        {\"id\": \"Find\","
        "\"label\": \"Find...\"},\r\n        {\"id\": \"FindAgain\", \"label\":"
        "\"Find Again\"},\r\n        {\"id\": \"Copy\"},\r\n        {\"id\":"
        "\"CopyAgain\", \"label\": \"Copy Again\"},\r\n        {\"id\":"
        "\"CopySVG\", \"label\": \"Copy SVG\"},\r\n        {\"id\":"
        "\"ViewSVG\", \"label\": \"View SVG\"},\r\n        {\"id\":"
        "\"ViewSource\", \"label\": \"View Source\"},\r\n        {\"id\":"
        "\"SaveAs\", \"label\": \"Save As\"},\r\n        null,\r\n"
        "{\"id\": \"Help\"},\r\n        {\"id\": \"About\", \"label\":"
        "\"About Adobe CVG Viewer...\"}\r\n    ]\r\n}}";
    struct jsonValue value;
    if (!json_parse_value(json, &value)) return 1;
    print_json_value(value, 0);
    puts("");
    json_value_free(value);
    return 0;
}
