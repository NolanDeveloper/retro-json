#include <stdlib.h>

#include "json_internal.h"

#include "unit.h"
#include "compliance.h"
#include "stress.h"

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    if (!json_init()) {
        fprintf(stderr, "json_init failed\n");
        return EXIT_FAILURE;
    }
    bool result = test_compliance("./test/JSONTestSuite", "./test/pretty") 
        && test_unit() 
        && test_stress();
    json_exit();
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
