#ifndef NDEBUG

void * dbg_malloc(size_t size, const char * file, int line);
void * dbg_calloc(size_t size, const char * file, int line);
void * dbg_realloc(void * ptr, size_t size, const char * file, int line);
void dbg_free(void * ptr, const char * file, int line);
bool dbg_is_memory_clear(void);
void dbg_print_blocks(void);

#define json_malloc(size)        dbg_malloc(size, __FILE__, __LINE__)
#define json_calloc(size)        dbg_calloc(size, __FILE__, __LINE__)
#define json_realloc(ptr, size)  dbg_realloc(ptr, size, __FILE__, __LINE__)
#define json_free(ptr)           dbg_free(ptr, __FILE__, __LINE__)

#else

#define json_malloc        json_malloc_
#define json_calloc(size)  json_calloc_(size, 1)
#define json_realloc       json_realloc_
#define json_free          json_free_

#endif

void * json_malloc_(size_t size);
void * json_calloc_(size_t size);
void * json_realloc_(void * ptr, size_t size);
void json_free_(void * ptr);

void json_string_init(struct jsonString * string);
bool json_string_init_str(struct jsonString * string, const char * str);
void json_string_free_internal(struct jsonString * string);
bool json_string_append(struct jsonString * string, char c);
bool json_string_shrink(struct jsonString * string);
unsigned json_string_hash(const char * str);

void json_array_init(struct jsonArray * array);
void json_array_free_internal(struct jsonArray * array);
bool json_array_append(struct jsonArray * array, struct jsonValue * value);
size_t json_array_size(struct jsonArray * array);

void json_object_init(struct jsonObject * object);
void json_object_free_internal(struct jsonObject * object);
bool json_object_add(struct jsonObject * object, struct jsonString * key, struct jsonValue * value);
struct jsonValue * json_object_at(struct jsonObject * object, const char * key);

void json_value_free_internal(struct jsonValue * value);

/*! Marker for trailing bytes */
#define TR (-1)

/*! Marker for illegal bytes */
#define IL (-2)

extern int utf8_bytes_left[256]; /*!< utf8_bytes_left[code unit] = how long this code point is in utf8 - 1 */

extern const char * error_out_of_memory;

extern __thread const char * error;

void set_error(const char * e);

extern thread_local const char *json_begin; //!< holds start of json string during json_parse recursive calls 
extern thread_local const char *json_end; 
extern thread_local const char *json_it; 

extern void errorf(const char *fmt, ...);

