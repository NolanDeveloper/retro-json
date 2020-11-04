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

char *file_bytes;
size_t file_size;

static void read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    file_bytes = malloc(file_size + 1);
    if (!file_bytes) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    fread(file_bytes, 1, file_size, f);
    fclose(f);
    file_bytes[file_size] = '\0';
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
    bool ok = true;
    for (int i = 1; ok && argv[i]; ++i) {
        char *filename = argv[i];
        char type = basename(filename)[0];
        if (!strchr("yni", type)) {
            printf("file name doesn't start with 'y', 'n', or 'i': %s\n", filename);
            continue;
        }
        read_file(filename);
        struct jsonValue *json = json_parse_mem(file_bytes, file_size, true);
        switch (type) {
        case 'y':
            if (json) {
                printf(GREEN "TEST PASSED" RESET " '%s'\n", filename);
            } else {
                printf(RED "FAILED TO PARSE" RESET " '%s' (%s)\n", filename, json_strerror());
                ok = false;
            }
            break;
        case 'n':
            if (json) {
                printf(RED "PARSER DIDN'T FAIL" RESET " '%s'\n", filename);
                ok = false;
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
        free(file_bytes);
        file_bytes = NULL;
        file_size = 0;
#ifndef RELEASE
        if (!dbg_is_memory_clear()) {
            printf(RED "MEMORY LEAK" RESET " '%s'\n", filename);
            dbg_print_blocks();
            exit(EXIT_FAILURE);
        }
#endif
    }
    json_exit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
