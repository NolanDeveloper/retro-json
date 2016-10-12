/*
 * author: Nolan <sullen.goose@gmail.com>
 * license: license.terms
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "parser.h"

#define TAB_SIZE 4

int offset;
int not_first;

void print_json_value(struct jsonValue * value);

void print_json_pair(const char * key, struct jsonValue * value, void * user_data) {
    (void) user_data;
    if (not_first) printf(",\n");
    printf("%*.s\"%s\": ", offset, "", key);
    print_json_value(value);
    not_first = 1;
}

void print_json_object(struct jsonObject * object) {
    printf("{\n");
    not_first = 0;
    offset += TAB_SIZE;
    json_object_for_each(object, print_json_pair, NULL);
    puts("");
    offset -= TAB_SIZE;
    printf("%*.s}", offset, "");
}

void print_json_value_in_array(size_t i, struct jsonValue * value, void * user_data) {
    struct jsonArray * array = user_data;
    printf("%*.s", offset, "");
    print_json_value(value);
    if (i != json_array_size(array) - 1) printf(",\n");
}

void print_json_array(struct jsonArray * array) {
    printf("[\n");
    offset += TAB_SIZE;
    json_array_for_each(array, print_json_value_in_array, array);
    offset -= TAB_SIZE;
    puts("");
    printf("%*.s]", offset, "");
}

void print_json_value(struct jsonValue * value) {
    switch (json_value_kind(value)) {
    case JVK_STR:  printf("\"%s\"", json_string_data(json_value_string(value))); break;
    case JVK_NUM:  printf("%f", json_value_number(value));               break;
    case JVK_OBJ:  print_json_object(json_value_object(value));  break;
    case JVK_ARR:  print_json_array(json_value_array(value));    break;
    case JVK_BOOL: printf(json_value_bool(value) ? "true" : "false");    break;
    case JVK_NULL: printf("null");                                       break;
    default: break;
    }
}

int main(int argc, char ** argv) {
    int file;
    char * memory;
    struct stat info;
    struct jsonDocument * document;
    if (argc != 2) return -1;
    file = open(argv[1], O_RDONLY);
    if (-1 == file) return 1;
    if (-1 == fstat(file, &info)) {
        close(file);
        return 1;
    };
    memory = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, file, 0);
    if (MAP_FAILED == memory) {
        close(file);
        return 2;
    }
    document = json_parse(memory);
    if (!document) {
        munmap(memory, info.st_size);
        close(file);
        return 3;
    }
#ifndef NDEBUG
    print_json_value(json_document_value(document));
    puts("");
#endif
    json_document_free(document);
    munmap(memory, info.st_size);
    close(file);
    return 0;
}
