#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "json.h"

int main(int argc, char ** argv) {
    int file;
    char * memory, * buffer;
    struct stat info;
    struct jsonValue * value;
    int result;
    size_t size;
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
    size = json_pretty_print(NULL, 0, value);
    buffer = malloc(size);
    json_pretty_print(buffer, size, value);
    puts(buffer);
finish:
    fflush(stdout);
    json_value_free(value);
    munmap(memory, info.st_size);
    close(file);
    return result;
}
