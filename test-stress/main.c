#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

#define REPEATS 1000
#define JSON_SIZE 30
#define JSON_OBJARR_SIZE 3
#define JSON_STR_SIZE 20 /* number of unicode code points */

/*! At most single character can take up to 12 bytes in the form of hex encoded UTF-16 surrogate pairs: \uXXXX\uXXXX.
 * We use excessive buffer size just in case we are wrong in this assumption. In the end only some characters will be
 * encoded in this longest form. So we should be fine anyway. */
static char cstr_buffer[JSON_STR_SIZE * 13];

static char * generate_cstr(void) {
    char c;
    char * p;
    int i;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    p = cstr_buffer;
    for (i = 0; i < JSON_STR_SIZE; ++i) {
        /*c = rand() % (127 - 20) + 20; */
        /*if (c == '"' || c == '\\') *p++ = '\\'; */
        c = alphabet[rand() % (sizeof(alphabet) - 1)];
        *p++ = c;
    }
    *p++ = '\0';
    return cstr_buffer;
}

static struct jsonValue * generate_str(void) {
    return json_create_string(generate_cstr());
}

static struct jsonValue * generate_num(void) {
    /*! this expression should produce both integral and fractional numbers */
    /*! in fact I expect quarter of the generated numbers to be integral */
    return json_create_number((rand() - RAND_MAX / 2) / 4.);
}

static struct jsonValue * generate_bool(void) {
    return json_create_boolean(rand() % 2);
}

static struct jsonValue * generate_null(void) {
    return json_create_null();
}

static char key_buffer[sizeof(cstr_buffer) + 100];

static char * generate_key(unsigned i) {
    sprintf(key_buffer, "%d: %s", i, generate_cstr());
    return key_buffer;
}

static struct jsonValue * generate_json(size_t n) {
    char * key;
    struct jsonValue * value, * subvalue;
    size_t k, t;
    unsigned i;
    if (1 == n) {
        switch (rand() % 4) {
        case 0: return generate_str();
        case 1: return generate_num();
        case 2: return generate_bool();
        case 3: return generate_null();
        }
    }
    k = n / JSON_OBJARR_SIZE;
    if (rand() % 2) {
        value = json_create_object(JSON_OBJARR_SIZE);
        if (!value) return NULL;
        for (i = 0; i < JSON_OBJARR_SIZE; ++i) {
            t = k + (i < n - k * JSON_OBJARR_SIZE ? 1 : 0);
            if (t == 0) break;
            subvalue = generate_json(t);
            key = generate_key(i);
            if (!json_object_add(value, key, subvalue)) {
                printf("json_value_object_add failed\n");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        value = json_create_array(JSON_OBJARR_SIZE);
        if (!value) return NULL;
        for (i = 0; i < JSON_OBJARR_SIZE; ++i) {
            t = k + (i < n - k * JSON_OBJARR_SIZE ? 1 : 0);
            if (t == 0) break;
            if (!json_array_append(value, generate_json(t))) {
                printf("json_value_array_add failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return value;
}

static char json_str_buffer[100 * 1024];
static char json_str_parsed_buffer[100 * 1024];

#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define RESET "\x1B[0m"

int main(int argc, char *argv[]) {
    struct jsonValue *json, *json_parsed;
    int i;
    (void) argc;
    (void) argv;
    json_init();
    srand(42);
    for (i = 0; i < REPEATS; ++i) {
        json = generate_json(JSON_SIZE);
        json_pretty_print(json_str_buffer, sizeof(json_str_buffer), json);
        json_parsed = json_parse(json_str_buffer, true);
        //! @todo Show path to the first different node
        if (!json_are_equal(json, json_parsed, NULL, NULL)) {
            json_pretty_print(json_str_parsed_buffer, sizeof(json_str_parsed_buffer), json_parsed);
            printf(RED "STRESS TEST FAILED\n" RESET);
            printf("Attempt #%d\n", i);
            printf("========================================\n");
            printf("%s\n", json_str_buffer);
            printf("========================================\n");
            printf("%s\n", json_str_parsed_buffer);
            printf("========================================\n");
            return EXIT_FAILURE;
        }
        json_value_free(json_parsed);
        json_value_free(json);
#ifndef RELEASE
        if (!dbg_is_memory_clear()) {
            printf(RED "STRESS TEST FAILED\n" RESET);
            printf("Attempt #%d\n", i);
            printf("There was a memory leak.\n");
            dbg_print_blocks();
            return EXIT_FAILURE;
        }
#endif
    }
    json_exit();
    printf(GREEN "STRESS TEST PASSED\n" RESET);
    return EXIT_SUCCESS;
}

