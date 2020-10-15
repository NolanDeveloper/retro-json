#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "src/json_parser.h"

#define KB(n)   (1024 * (n))
#define MB(n) KB(1024 * (n))
#define MAX_SUPPORTED_FILE_SIZE MB(100)

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
    case JVK_NUM:  printf("%f", json_value_number(value));                       break;
    case JVK_OBJ:  print_json_object(json_value_object(value));                  break;
    case JVK_ARR:  print_json_array(json_value_array(value));                    break;
    case JVK_BOOL: printf(json_value_bool(value) ? "true" : "false");            break;
    case JVK_NULL: printf("null");                                               break;
    default: break;
    }
}

int main(int argc, char ** argv) {
    int file;
    char * memory;
    struct stat info;
    struct jsonValue * value;
    int result;
    file   = -1;
    memory = NULL;
    value  = NULL;
    result = EXIT_SUCCESS;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ./file.json", argv[0]);
        result = EXIT_FAILURE;
        goto finish;
    }
    file = open(argv[1], O_RDONLY);
    if (file < 0) {
        perror("open");
        result = EXIT_FAILURE;
        goto finish;
    }
    if (fstat(file, &info) < 0) {
        perror("fstat");
        result = EXIT_FAILURE;
        goto finish;
    };
    if (info.st_size > MAX_SUPPORTED_FILE_SIZE) {
        fprintf(stderr, "Input file is too big.");
        result = EXIT_FAILURE;
        goto finish;
    }
    memory = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, file, 0);
    if (MAP_FAILED == memory) {
        result = EXIT_FAILURE;
        goto finish;
    }
    value = json_parse(memory);
    if (!value) {
        fprintf(stderr, "Failed to parse json.");
        result = EXIT_FAILURE;
        goto finish;
    }
    setvbuf(stdout, NULL, _IOFBF, 0);
    print_json_value(value);
    puts("");
finish:
#ifndef NDEBUG
    dbg_print_blocks();
#endif
    fflush(stdout);
    json_value_free(value);
    munmap(memory, info.st_size);
    close(file);
    return result;
}
