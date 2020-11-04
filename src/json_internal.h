#ifndef JSON_INTERNAL_H
#define JSON_INTERNAL_H

#include <json.h>

#include <threads.h>
#include <uchar.h>

#ifndef RELEASE

void *dbg_malloc(size_t size, const char *file, int line);
void *dbg_calloc(size_t size, const char *file, int line);
void *dbg_realloc(void *ptr, size_t size, const char *file, int line);
void dbg_free(void *ptr, const char *file, int line);
void dbg_mem_detach(void *ptr, const char *file, int line);
bool dbg_is_memory_clear(void);
void dbg_print_blocks(void);

#define json_malloc(size)        dbg_malloc(size, __FILE__, __LINE__)
#define json_calloc(size)        dbg_calloc(size, __FILE__, __LINE__)
#define json_realloc(ptr, size)  dbg_realloc(ptr, size, __FILE__, __LINE__)
#define json_free(ptr)           dbg_free(ptr, __FILE__, __LINE__)
#define json_mem_detach(ptr)     dbg_mem_detach(ptr, __FILE__, __LINE__)

#else

#define json_malloc(size)        json_malloc_(size)
#define json_calloc(size)        json_calloc_(size)
#define json_realloc(ptr, size)  json_realloc_(ptr, size)
#define json_free(ptr)           json_free_(ptr)
#define json_mem_detach(ptr)

#endif

// Assertion that's computationally hard to evaluate.  This might be used to
// check complex invariants.
#ifndef RELEASE
#define assert_slow(expr) assert(expr)
#else
#define assert_slow(expr)
#endif

struct jsonString {
    size_t capacity;
    size_t size;
    char *data;
    unsigned hash;
};

struct jsonArray {
    size_t capacity;
    size_t size;
    struct jsonValue **values;
};

struct jsonObjectEntry;

struct jsonObject {
    size_t capacity;
    size_t size;
    size_t unique_size;
    struct jsonObjectEntry *entries;
};

struct jsonObjectEntry {
    unsigned long long id;
    struct jsonString *key;
    struct jsonValue *value;
};

struct jsonValue {
    enum jsonValueKind kind;
    union {
        double number;
        struct jsonString string;
        struct jsonObject object;
        struct jsonArray array;
        bool boolean;
    } v;
};

void *json_malloc_(size_t size);
void *json_calloc_(size_t size);
void *json_realloc_(void *ptr, size_t size);
void json_free_(void *ptr);

struct jsonString *string_create(void);
struct jsonString *string_create_str(const char *str);
void string_init(struct jsonString *string);
bool string_init_str(struct jsonString *string, const char *str);
bool string_init_mem(struct jsonString *string, const char *mem, size_t n);
void string_free_internal(struct jsonString *string);
bool string_append(struct jsonString *string, char c);
bool string_shrink(struct jsonString *string);
unsigned string_hash(const char *str);

void array_init(struct jsonArray *array);
void array_free_internal(struct jsonArray *array);
bool array_reserve(struct jsonArray *array, size_t new_capacity);
bool array_double(struct jsonArray *array, size_t min_capacity);
bool array_append(struct jsonArray *array, struct jsonValue *value);
size_t array_size(struct jsonArray *array);

extern const struct jsonString key_deleted;

void object_init(struct jsonObject *object);
void object_free_internal(struct jsonObject *object);
bool object_reserve(struct jsonObject *object, size_t size);
bool object_add(struct jsonObject *object, struct jsonString *key, struct jsonValue *value);
void object_get_entry(struct jsonObject *object, size_t i, struct jsonString **out_key, struct jsonValue **out_value);
struct jsonValue *object_next(struct jsonObject *object, const char *key, struct jsonValue *prev);
struct jsonValue *object_at(struct jsonObject *object, const char *key);

void value_free_internal(struct jsonValue *value);

void parser_begin(const char *json);
void parser_end(void);
struct jsonValue *parse_json_text(bool all);

void pretty_print_begin(char *out, size_t size);
size_t pretty_print_end(void);
void print_json_value(struct jsonValue *value);

enum c16Type {
    UTF16_NOT_SURROGATE,
    UTF16_SURROGATE_HIGH,
    UTF16_SURROGATE_LOW
};

int c16len(char16_t c16);
enum c16Type c16type(char16_t c16);
char32_t c16pairtoc32(char16_t high, char16_t low);
int c8len(char c);
bool c32toc8(char32_t c32, int *n, char *c8);
char32_t c8toc32(const char *c8);
void c32toc16be(char32_t c32, char16_t out[2]);

extern tss_t error_key;

bool error_init(void);
void error_exit(void);
void set_error(const char *e);

extern const char *error_out_of_memory;
extern thread_local const char *json_begin; //!< holds start of json string during json_parse recursive calls
extern thread_local const char *json_it;

void errorf(const char *fmt, ...);

#endif
