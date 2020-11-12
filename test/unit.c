#include "colors.h"
#include "unit.h"
#include "unit_tests.h"

static struct {
    bool (*run)(void);
    const char *name;
} tests[] = {
    { test_utf, "UTF" },
};

extern bool test_unit(void) {
    bool result = true;
    for (size_t i = 0; i < sizeof(tests) / sizeof(*tests); ++i) {
        bool ok = tests[i].run();
        if (ok) {
            printf(GREEN "%s UNIT TEST PASSED\n" RESET, tests[i].name);
        } else {
            printf(RED "%s UNIT TEST FAILED\n" RESET, tests[i].name);
        }
        result = result && ok;
    }
    return result;
}
