#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

#define REPEATS 10000
#define JSON_SIZE 30
#define JSON_OBJARR_KEYS 10
#define JSON_STR_SIZE 20 /* number of unicode code points */

/*! At most single character can take up to 12 bytes in the form of hex encoded
 * UTF-16 surrogate pairs: \uXXXX\uXXXX. We use excessive buffer size just in
 * case we are wrong in this assumption. In the end only some characters will
 * be encoded in this longest form. So we should be fine anyway. */
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
        *p++ = c; /*! @todo add unicode and escape sequences */
    }
    *p++ = '\0';
    return cstr_buffer;
}

static struct jsonValue * generate_str(void) {
    return json_value_create_string(generate_cstr());
}

static struct jsonValue * generate_num(void) {
    /*! this expression should produce both integral and fractional numbers */
    /*! in fact I expect quarter of the generated numbers to be integral */
    return json_value_create_number((rand() - RAND_MAX / 2) / 4.); 
}

static struct jsonValue * generate_bool(void) {
    return json_value_create_boolean(rand() % 2);
}

static struct jsonValue * generate_null(void) {
    return json_value_create_null();
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
    k = n / JSON_OBJARR_KEYS;
    if (rand() % 2) {
        value = json_value_create_object();
        if (!value) return NULL;
        for (i = 0; i < JSON_OBJARR_KEYS; ++i) {
            t = k + (i < n - k * JSON_OBJARR_KEYS ? 1 : 0);
            if (t == 0) break;
            subvalue = generate_json(t);
            key = generate_key(i);
            if (!json_value_object_add(value, key, subvalue)) {
                printf("json_value_object_add failed\n");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        value = json_value_create_array();
        if (!value) return NULL;
        for (i = 0; i < JSON_OBJARR_KEYS; ++i) {
            t = k + (i < n - k * JSON_OBJARR_KEYS ? 1 : 0);
            if (t == 0) break;
            if (!json_value_array_append(value, generate_json(t))) {
                printf("json_value_array_add failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return value;
}

static int are_equal(struct jsonValue * left, struct jsonValue * right);

static int are_equal_objects(struct jsonValue * left, struct jsonValue * right) {
    struct jsonObjectEntry * entry;
    struct jsonValue * lookup;
    if (left->v.object.size != right->v.object.size) {
        return 0;
    }
    entry = left->v.object.first;
    while (entry) {
        lookup = json_value_object_lookup(right, entry->key->data);
        if (!lookup || !are_equal(entry->value, lookup)) {
            return 0;
        }
        entry = entry->next;
    }
    return 1;
}

static int are_equal_arrays(struct jsonValue * left, struct jsonValue * right) {
    struct jsonValue * left_element, * right_element;
    size_t i, n;
    n = json_value_array_size(left);
    if (n != json_value_array_size(right)) return 0;
    for (i = 0; i < n; ++i) {
        left_element = json_value_array_at(left, i);
        right_element = json_value_array_at(right, i);
        if (!are_equal(left_element, right_element)) {
            return 0;
        }
    }
    return 1;
}

static int are_equal(struct jsonValue * left, struct jsonValue * right) {
    if (!left && !right) {
        return 1;
    }
    if (!left || !right) {
        return 0;
    }
    if (left->kind != right->kind) {
        return 0;
    }
    switch (left->kind) {
    case JVK_STR: 
        return !strcmp(left->v.string.data, right->v.string.data);
    case JVK_NUM: 
        return left->v.number == right->v.number;
    case JVK_OBJ: 
        return are_equal_objects(left, right);
    case JVK_ARR: 
        return are_equal_arrays(left, right);
    case JVK_BOOL: 
        return left->v.boolean == right->v.boolean;
    case JVK_NULL: 
        return 1;
    }
    exit(EXIT_FAILURE);
}

static char json_str_buffer[100 * 1024];
static char json_str_parsed_buffer[100 * 1024];

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

int main(int argc, char * argv[]) {
    struct jsonValue * json, * json_parsed;
    int i;
    (void) argc;
    (void) argv;
    srand(42);
    for (i = 0; i < REPEATS; ++i) {
        json = generate_json(JSON_SIZE);
        json_pretty_print(json_str_buffer, sizeof(json_str_buffer), json);
        json_parsed = json_parse(json_str_buffer);
        if (!are_equal(json, json_parsed)) {
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
        if (!dbg_is_memory_clear()) {
            printf(RED "STRESS TEST FAILED\n" RESET);
            printf("Attempt #%d\n", i);
            printf("There was a memory leak.\n");
            dbg_print_blocks();
            return EXIT_FAILURE;
        }
    }
    printf(GREEN "STRESS TEST PASSED\n" RESET);
    return EXIT_SUCCESS;
}

