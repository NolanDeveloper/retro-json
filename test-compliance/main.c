#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <json_internal.h>

#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define BLUE   "\x1b[34m"
#define RESET  "\x1b[0m"

static void read_file(const char *filename, char **bytes, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *bytes = malloc(*size + 1);
    if (!bytes) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    fread(*bytes, 1, *size, f);
    fclose(f);
    (*bytes)[*size] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("\t%s filename.json [how_it_should_pretty_print.json]\n", argv[0]);
        printf("\t%s JSONTestSuite/y_test1.json pretty/y_test1.json\n", argv[0]);
        printf("\t%s JSONTestSuite/i_test2.json pretty/y_test2.json\n", argv[0]);
        printf("\t%s JSONTestSuite/n_test3.json\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (!json_init()) {
        printf("json_init failed\n");
        return EXIT_FAILURE;
    }
    char *filename = argv[1];
    char *bytes;
    size_t size;
    read_file(filename, &bytes, &size);
    struct jsonValue *json = json_parse_mem(bytes, size, true);
    if (3 <= argc) {
        if (!json) {
            printf(RED "FAILED TO PARSE" RESET " '%s' (%s)\n", filename, json_strerror());
            return EXIT_FAILURE;
        }
        const char *how_it_should_pretty_print = argv[2];
        char *pp_bytes;
        size_t pp_size;
        read_file(how_it_should_pretty_print, &pp_bytes, &pp_size);
        size_t actual_pretty_print_size = json_pretty_print(NULL, 0, json);
        char *buffer = malloc(actual_pretty_print_size + 1);
        json_pretty_print(buffer, actual_pretty_print_size + 1, json);
        if (pp_size != actual_pretty_print_size || memcmp(buffer, pp_bytes, pp_size)) {
            printf("===============================\n");
            printf("ACTUAL:\n%s\n", buffer);
            printf("===============================\n");
            printf("EXPECTED:\n%s\n", pp_bytes);
            printf(RED "FAILED" RESET ": PRETTY PRINT IS DIFFERENT FROM EXPECTED '%s' '%s' (%s)\n", 
                    filename, how_it_should_pretty_print, json_strerror());
            return EXIT_FAILURE;
        }
        free(buffer);
        free(pp_bytes);
    } else if (json) {
        printf(RED "PARSER DIDN'T FAIL" RESET " '%s'\n", filename);
        return EXIT_FAILURE;
    }
    json_value_free(json);
    if (!dbg_is_memory_clear()) {
        printf(RED "MEMORY LEAK" RESET " '%s'\n", filename);
        dbg_print_blocks();
        return EXIT_FAILURE;
    }
    json_exit();
    printf(GREEN "TEST PASSED" RESET " '%s'\n", filename);
    return EXIT_SUCCESS;
}
