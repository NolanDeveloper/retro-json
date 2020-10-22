#ifndef NDEBUG

void * dbg_malloc(size_t size, const char * file, int line);
void * dbg_calloc(size_t size, const char * file, int line);
void * dbg_realloc(void * ptr, size_t size, const char * file, int line);
void dbg_free(void * ptr, const char * file, int line);
int dbg_is_memory_clear(void);
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

void json_string_init(struct jsonString * string);
int json_string_init_str(struct jsonString * string, const char * str);
void json_string_free_internal(struct jsonString * string);
int json_string_append(struct jsonString * string, char c);
int json_string_shrink(struct jsonString * string);
unsigned json_string_hash(const char * str);

void json_array_init(struct jsonArray * array);
void json_array_free_internal(struct jsonArray * array);
int json_array_append(struct jsonArray * array, struct jsonValue * value);
size_t json_array_size(struct jsonArray * array);

void json_object_init(struct jsonObject * object);
void json_object_free_internal(struct jsonObject * object);
int json_object_add(struct jsonObject * object, struct jsonString * key, struct jsonValue * value);
struct jsonValue * json_object_at(struct jsonObject * object, const char * key);

void json_value_free_internal(struct jsonValue * value);

/*! Marker for trailing bytes */
#define TR (-1)

/*! Marker for illegal bytes */
#define IL (-2)

extern int utf8_bytes_left[256]; /*!< utf8_bytes_left[code unit] = how long this code point is in utf8 - 1 */

