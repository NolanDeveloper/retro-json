/*
 * author: Nolan <sullen.goose@gmail.com>
 * Copy if you can.
 */

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

struct int_pair {
    int first;
    int second;
};

void print_offset(int offset) { int i; for (i = 0; i < offset; ++i) printf("    "); }

void print_json_value(struct jsonValue * value, int offset);

void print_json_pair(const char * key, struct jsonValue * value, void * user_data) {
    struct int_pair * p = user_data;
    if (p->first) printf(",\n");
    print_offset(p->second + 1);
    printf("\"%s\" : ", key);
    print_json_value(value, p->second + 1);
    p->first = 1;
}

void print_json_object(struct jsonObject * object, int offset) {
    struct int_pair p;
    p.first = 0;
    p.second = offset;
    printf("{\n");
    json_object_for_each(object, print_json_pair, &p);
    puts("");
    print_offset(offset);
    printf("}");
}

void print_json_value_in_array(size_t i, struct jsonValue * value, void * user_data) {
    void * (*p)[2] = user_data;
    int * offset = (*p)[0];
    struct jsonArray * array = (*p)[1];
    print_offset(*offset + 1);
    print_json_value(value, *offset + 1);
    if (i != json_array_size(array) - 1) printf(",\n");
}

void print_json_array(struct jsonArray * array, int offset) {
    void * p[2];
    p[0] = &offset;
    p[1] = array;
    printf("[\n");
    json_array_for_each(array, print_json_value_in_array, &p);
    puts("");
    print_offset(offset);
    printf("]");
}

void print_json_value(struct jsonValue * value, int offset) {
    switch (json_value_kind(value)) {
    case JVK_STR:  printf("\"%s\"", json_string_data(json_value_string(value))); break;
    case JVK_NUM:  printf("%f", json_value_number(value));               break;
    case JVK_OBJ:  print_json_object(json_value_object(value), offset);  break;
    case JVK_ARR:  print_json_array(json_value_array(value), offset);    break;
    case JVK_BOOL: printf(json_value_bool(value) ? "true" : "false");    break;
    case JVK_NULL: printf("null");                                       break;
    default: break;
    }
}

#define BUFFER_SIZE (4 * 1024 * 1024)

int from_string() {
    const char * json;
    struct jsonDocument * document;
    json = "[ { \"name\" : \"101 (MM4)\", \"id\" : 1 } ]";
    document = json_parse(json);
    if (!document) return 0;
    print_json_value(json_document_value(document), 0);
    puts("");
    json_document_free(document);
    return 0;
}

int from_stdin() {
    char * buffer;
    struct jsonDocument * document;
    size_t bytes_read;
    int ret = 0;
    buffer = malloc(BUFFER_SIZE);
    bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, stdin);
    if (bytes_read == BUFFER_SIZE) { ret = 1; goto end; }
    document = json_parse(buffer);
    if (!document) { ret = 1; goto end; }
    /*print_json_value(json_document_value(document), 0);*/
    /*puts("");*/
    json_document_free(document);
end:
    free(buffer);
    return ret;
}

int main() {
    return from_stdin();
}
