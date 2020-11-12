#include "colors.h"
#include "compliance.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include <json_internal.h>

static bool read_file(const char *filename, char **bytes, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *bytes = malloc(*size + 1);
    assert(*bytes);
    size_t n = fread(*bytes, 1, *size, f);
    assert(n == *size);
    fclose(f);
    (*bytes)[*size] = '\0';
    return true;
}

static bool test_case(char *filename, char *how_it_should_pretty_print) {
    char *bytes;
    size_t size;
    assert(read_file(filename, &bytes, &size));
    struct jsonValue *json = json_parse_mem(bytes, size, true);
    free(bytes);
    char *pp_bytes;
    size_t pp_size;
    if (read_file(how_it_should_pretty_print, &pp_bytes, &pp_size)) {
        if (!json) {
            printf(RED "FAILED TO PARSE" RESET " '%s' (%s)\n", filename, json_strerror());
            return false;
        }
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
            return false;
        }
        free(buffer);
        free(pp_bytes);
    } else if (json) {
        printf(RED "PARSER DIDN'T FAIL" RESET " '%s'\n", filename);
        return false;
    }
    json_value_free(json);
    printf(GREEN "TEST PASSED" RESET " '%s'\n", filename);
    return true;
}

#define PATH_MAX_LENGTH 1023

bool test_compliance(char *json_test_suite_dir, char *pretty_dir) {
    DIR *test_cases = opendir(json_test_suite_dir);
    assert(test_cases);
    bool result = true;
    char filename[PATH_MAX_LENGTH + 1];
    char how_it_should_pretty_print[PATH_MAX_LENGTH + 1];
    for (struct dirent *entry = readdir(test_cases); entry; entry = readdir(test_cases)) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        filename[0] = '\0';
        strncat(filename, json_test_suite_dir, PATH_MAX_LENGTH);
        strncat(filename, "/", PATH_MAX_LENGTH);
        strncat(filename, entry->d_name, PATH_MAX_LENGTH);
        how_it_should_pretty_print[0] = '\0';
        strncat(how_it_should_pretty_print, pretty_dir, PATH_MAX_LENGTH);
        strncat(how_it_should_pretty_print, "/", PATH_MAX_LENGTH);
        strncat(how_it_should_pretty_print, entry->d_name, PATH_MAX_LENGTH);
        test_case(filename, how_it_should_pretty_print);
    }
    closedir(test_cases);
    return result;
}

