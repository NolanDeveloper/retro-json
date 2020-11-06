#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <json.h>

#include "tests.h"

static struct {
    bool (*run)(void);
    const char *name;
} tests[] = {
    { test_array, "ARRAY" },
    { test_object, "OBJECT" },
    { test_number, "NUMBER" },
    { test_utf, "UTF" },
    { test_string, "STRING" },
    { test_parser, "PARSER" },
    { test_pretty_printer, "PRETTY PRINTER" },
};

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    if (!json_init()) {
        fprintf(stderr, "json_init failed\n");
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < sizeof(tests) / sizeof(*tests); ++i) {
        bool ok = tests[i].run();
        if (ok) {
            printf(GREEN "%s UNIT TEST PASSED\n" RESET, tests[i].name);
        } else {
            printf(RED "%s UNIT TEST FAILED\n" RESET, tests[i].name);
            return EXIT_FAILURE;
        }
    }
    json_exit();
    return EXIT_SUCCESS;
}
