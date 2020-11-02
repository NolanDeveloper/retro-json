#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include <libgen.h>

#include <json.h>
#include <json_internal.h>

#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define BLUE   "\x1b[34m"
#define RESET  "\x1b[0m"

static const char *read_file(const char *filename) {
    char *buffer = NULL;
    long length;
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length + 1);
    if (!buffer) {
        perror("malloc");
        return NULL;
    }
    fread(buffer, 1, length, f);
    fclose(f);
    buffer[length] = '\0';
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n\t%s y_test1.json n_test2.json i_test3.json\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (!json_init()) {
        printf("json_init failed\n");
        return EXIT_FAILURE;
    }
    for (int i = 1; argv[i]; ++i) {
        char *filename = argv[i];
        char type = basename(filename)[0];
        if (!strchr("yni", type)) {
            printf("file name doesn't start with 'y', 'n', or 'i': %s\n", filename);
            continue;
        }
        const char *json_str = read_file(filename);
        if (!json_str) {
            continue;
        }
        struct jsonValue *json = json_parse(json_str, true);
        switch (type) {
        case 'y':
            if (json) {
                printf(GREEN "TEST PASSED" RESET " '%s'\n", filename);
            } else {
                printf(RED "FAILED TO PARSE" RESET " '%s' (%s)\n", filename, json_strerror());
            }
            break;
        case 'n':
            if (json) {
                printf(RED "PARSER DIDN'T FAIL" RESET " '%s'\n", filename);
            } else {
                printf(GREEN "TEST PASSED" RESET " '%s' (%s)\n", filename, json_strerror());
            }
            break;
        case 'i':
            if (json) {
                printf(BLUE "TEST PASSED" RESET " " GREEN "+" RESET " '%s'\n", filename);
            } else {
                printf(BLUE "TEST PASSED" RESET " " RED "-" RESET " '%s' (%s)\n", filename, json_strerror());
            }
            break;
        }
        if (json) {
            json_value_free(json);
        }
        free((char *) json_str);
#ifndef NDEBUG
        if (!dbg_is_memory_clear()) {
            printf(RED "MEMORY LEAK" RESET " '%s'\n", filename);
            dbg_print_blocks();
            exit(EXIT_FAILURE);
        }
#endif
    }
    json_exit();
    return EXIT_SUCCESS;
}
