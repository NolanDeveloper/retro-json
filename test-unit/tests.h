#ifndef TESTS_H
#define TESTS_H

#include <stdbool.h>
#include <stdio.h>

bool test_array(void);
bool test_object(void);
bool test_number(void);
bool test_utf(void);
bool test_string(void);
bool test_parser(void);
bool test_pretty_printer(void);

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

#define ASSERT(expr)\
    do {\
        if (!(expr)) {\
            printf(RED "ERROR" RESET " assertion failed at %s:%d (%s)\n", __FILE__, __LINE__, #expr);\
            return false;\
        }\
    } while (false)

#endif
