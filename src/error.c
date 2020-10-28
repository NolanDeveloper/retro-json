#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <threads.h>

#include "json.h"
#include "json_internal.h"

const char * error_out_of_memory = "out of memory";
__thread const char * error;

void set_error(const char * e) {
    if (error && error != error_out_of_memory) {
        json_free((char *) error);
    }
    error = e;
}

static char *vasprintf(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int n = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (n < 0) {
        return NULL;
    }
    char *text = json_malloc(n + 1);
    if (!text) {
        return NULL;
    }
    va_copy(args_copy, args);
    n = snprintf(text, n + 1, fmt, args);
    va_end(args_copy);
    if (n < 0) {
        json_free(text);
        return NULL;
    }
    return text;
}

static char *asprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *result = vasprintf(fmt, args);
    va_end(args);
    return result;
}

extern void errorf(const char *fmt, ...) {
    unsigned line = 0;
    unsigned column = 0;
    const char *p = json_begin;
    while (*p && p != json_it) {
        if ('\n' == *p) {
            ++line;
            column = 0;
        }
        ++column;
        ++p;
    }
    va_list args;
    va_start(args, fmt);
    char *message = vasprintf(fmt, args);
    va_end(args);
    if (!message) {
        set_error(NULL);
        error = NULL;
        return;
    }
    set_error(asprintf("[%u:%u] %s", line, column, message));
    json_free(message);
}

