#ifndef NDEBUG

void * dbg_malloc(size_t size, const char * file, int line);
void * dbg_calloc(size_t size, const char * file, int line);
void * dbg_realloc(void * ptr, size_t size, const char * file, int line);
void dbg_free(void * ptr, const char * file, int line);
void dbg_print_blocks(void);

#define json_malloc(size)        dbg_malloc(size, __FILE__, __LINE__)
#define json_calloc(size)        dbg_calloc(size, __FILE__, __LINE__)
#define json_realloc(ptr, size)  dbg_realloc(ptr, size, __FILE__, __LINE__)
#define json_free(ptr)           dbg_free(ptr, __FILE__, __LINE__)

#else

#define json_malloc        malloc
#define json_calloc(size)  calloc(size, 1)
#define json_realloc       realloc
#define json_free          free

#endif

