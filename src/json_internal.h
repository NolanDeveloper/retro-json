#ifndef NDEBUG

void *dbg_malloc(size_t size, const char *file, int line);
void *dbg_calloc(size_t size, const char *file, int line);
void *dbg_realloc(void *ptr, size_t size, const char *file, int line);
void dbg_free(void *ptr, const char *file, int line);
bool dbg_is_memory_clear(void);
void dbg_print_blocks(void);

#define json_malloc(size)        dbg_malloc(size, __FILE__, __LINE__)
#define json_calloc(size)        dbg_calloc(size, __FILE__, __LINE__)
#define json_realloc(ptr, size)  dbg_realloc(ptr, size, __FILE__, __LINE__)
#define json_free(ptr)           dbg_free(ptr, __FILE__, __LINE__)

#else

#define json_malloc  json_malloc_
#define json_calloc  json_calloc_
#define json_realloc json_realloc_
#define json_free    json_free_

#endif

void *json_malloc_(size_t size);
void *json_calloc_(size_t size);
void *json_realloc_(void *ptr, size_t size);
void json_free_(void *ptr);

void json_string_init(struct jsonString *string);
bool json_string_init_str(struct jsonString *string, const char *str);
bool json_string_init_mem(struct jsonString *string, const char *mem, size_t n);
void json_string_free_internal(struct jsonString *string);
bool json_string_append(struct jsonString *string, char c);
bool json_string_shrink(struct jsonString *string);
unsigned json_string_hash(const char *str);

void json_array_init(struct jsonArray *array);
void json_array_free_internal(struct jsonArray *array);
bool json_array_reserve(struct jsonArray *array, size_t new_capacity);
bool json_array_double(struct jsonArray *array, size_t min_capacity);
bool json_array_append(struct jsonArray *array, struct jsonValue *value);
size_t json_array_size(struct jsonArray *array);

void json_object_init(struct jsonObject *object);
void json_object_free_internal(struct jsonObject *object);
bool json_object_reserve(struct jsonObject *object, size_t size);
bool json_object_add(struct jsonObject *object, struct jsonString *key, struct jsonValue *value);
struct jsonValue *json_object_next(struct jsonObject *object, const char *key, struct jsonValue *prev);
struct jsonValue *json_object_at(struct jsonObject *object, const char *key);

void json_value_free_internal(struct jsonValue *value);

enum c16Type {
    UTF16_NOT_SURROGATE,
    UTF16_SURROGATE_HIGH,
    UTF16_SURROGATE_LOW
};

extern int c16len(char16_t c16);
extern enum c16Type c16type(char16_t c16);
extern char32_t c16pairtoc32(char16_t high, char16_t low);
extern int c8len(char c);
extern bool c32toc8(char32_t c32, int *n, char *c8);
extern char32_t c8toc32(const char *c8);
extern void c32toc16be(char32_t c32, char16_t out[2]);

extern const char *error_out_of_memory;

extern thread_local const char *error;

void set_error(const char *e);

extern thread_local const char *json_begin; //!< holds start of json string during json_parse recursive calls 
extern thread_local const char *json_it; 

extern void errorf(const char *fmt, ...);

