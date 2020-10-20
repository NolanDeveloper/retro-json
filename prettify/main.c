#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "json.h"

#define KB(n)   (1024 * (n))
#define MB(n) KB(1024 * (n))
#define MAX_SUPPORTED_FILE_SIZE MB(100)

#define TAB_SIZE 4

#ifndef NDEBUG
#define BUFFER_SIZE 100
#else
#define BUFFER_SIZE KB(20)
#endif

static char buffer[BUFFER_SIZE];
static size_t would_write;

int main(int argc, char ** argv) {
    int file;
    char * memory;
    struct stat info;
    struct jsonValue * value;
    int result;
    file   = -1;
    memory = NULL;
    value  = NULL;
    result = EXIT_SUCCESS;
    memset(&info, 0, sizeof(struct stat));
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ./file.json\n", argv[0]);
        result = EXIT_FAILURE;
        goto finish;
    }
    file = open(argv[1], O_RDONLY);
    if (file < 0) {
        perror("open");
        result = EXIT_FAILURE;
        goto finish;
    }
    if (fstat(file, &info) < 0) {
        perror("fstat");
        result = EXIT_FAILURE;
        goto finish;
    };
    if (info.st_size > MAX_SUPPORTED_FILE_SIZE) {
        fprintf(stderr, "Input file is too big.\n");
        result = EXIT_FAILURE;
        goto finish;
    }
    memory = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, file, 0);
    if (MAP_FAILED == memory) {
        result = EXIT_FAILURE;
        goto finish;
    }
    value = json_parse(memory);
    if (!value) {
        fprintf(stderr, "Failed to parse json.\n");
        result = EXIT_FAILURE;
        goto finish;
    }
    setvbuf(stdout, NULL, _IOFBF, 0);
    would_write = json_pretty_print(NULL, 0, value);
    printf("would write: %ld\n", would_write);
    json_pretty_print(buffer, sizeof(buffer) / 2, value);
    printf("has written: %ld\n", strlen(buffer) + 1);
    puts(buffer);
    assert(strlen(buffer) <= sizeof(buffer) / 2 - 1);
#if 0
    puts("second half:"); /*!< @todo check second half is empty */
    puts(buffer + sizeof(buffer) / 2);
#endif 
finish:
    fflush(stdout);
    json_value_free(value);
    munmap(memory, info.st_size);
    close(file);
    return result;
}
