#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h> 

#include <json.h>

#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define BLUE   "\x1b[34m"
#define RESET  "\x1b[0m"

static char *vasprintf(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int n = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (n < 0) {
        return NULL;
    }
    char *text = malloc(n + 1);
    if (!text) {
        return NULL;
    }
    n = vsnprintf(text, n + 1, fmt, args);
    if (n < 0) {
        free(text);
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

static const char *read_file(const char *filename) {
    char *buffer = NULL;
    long length;
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length + 1);
    if (!buffer) {
        perror("malloc");
        return NULL;
    }
    fread(buffer, 1, length, f);
    fclose(f);
    buffer[length] = '\0';
    return buffer;
}

static const char *extension(const char *filename) {
    const char *ext = filename;
    while (true) {
        const char *next = strchr(ext, '.');
        if (!next) {
            break;
        }
        ext = next + 1;
    }
    return ext == filename ? "" : ext;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage:\n\ttest path/to/JSONTestSuite\n");
        return EXIT_FAILURE;
    }
    DIR *d = opendir(argv[1]);
    if (!d) {
        perror("opendir");
        return EXIT_FAILURE;
    }
    struct dirent *file;
    while ((file = readdir(d))) {
        if (!strcmp(file->d_name, ".")) {
            continue;
        }
        if (!strcmp(file->d_name, "..")) {
            continue;
        }
        const char *filename = asprintf("%s/%s", argv[1], file->d_name);
        if (strcmp(extension(file->d_name), "json")) {
            printf("skip unknown file: %s\n", filename);
            free((char *) filename);
            continue;
        }
        char type = file->d_name[0];
        if (!strchr("yni", type)) {
            printf("skip unknown file: %s\n", filename);
            free((char *) filename);
            continue;
        }
        const char *json_str = read_file(filename);
        if (!json_str) {
            free((char *) filename);
            continue;
        }
        struct jsonValue *json = json_parse(json_str);
        switch (type) {
        case 'y':
            if (json) {
                printf(GREEN "TEST PASSED" RESET " '%s'\n", filename);
            } else {
                printf(RED "FAILED TO PARSE" RESET " '%s'\n", filename);
            }
            break;
        case 'n':
            if (json) {
                printf(RED "PARSER DIDN'T FAIL" RESET " '%s'\n", filename);
            } else {
                printf(GREEN "TEST PASSED" RESET " '%s'\n", filename);
            }
            break;
        case 'i':
            if (json) {
                printf(BLUE "TEST PASSED" RESET " " GREEN "+" RESET " '%s'\n", filename);
            } else {
                printf(BLUE "TEST PASSED" RESET " " RED "-" RESET " '%s'\n", filename);
            }
            break;
        }
        if (json) {
            json_value_free(json);
        }
        free((char *) json_str);
        free((char *) filename);
    }
    closedir(d);
    return EXIT_SUCCESS;
}
