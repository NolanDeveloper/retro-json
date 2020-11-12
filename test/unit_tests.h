#ifndef UNIT_TESTS_H
#define UNIT_TESTS_H

#include <stdbool.h>
#include <stdio.h>

#include "colors.h"

bool test_utf(void);

#define ASSERT(expr)\
    do {\
        if (!(expr)) {\
            printf(RED "ERROR" RESET " assertion failed at %s:%d (%s)\n", __FILE__, __LINE__, #expr);\
            return false;\
        }\
    } while (false)

#endif
