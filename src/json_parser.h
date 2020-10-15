struct jsonString;
struct jsonObject;
struct jsonArray;
struct jsonValue;
struct jsonDocument;

enum jsonValueKind { JVK_STR, JVK_NUM, JVK_OBJ, JVK_ARR, JVK_BOOL, JVK_NULL };

#ifndef NDEBUG

void * dbg_malloc(size_t size, const char * file, int line);
void * dbg_realloc(void * ptr, size_t size, const char * file, int line);
void dbg_free(void * ptr, const char * file, int line);
void dbg_print_blocks(void);

#define json_malloc(size)        dbg_malloc(size, __FILE__, __LINE__)
#define json_realloc(ptr, size)  dbg_realloc(ptr, size, __FILE__, __LINE__)
#define json_free(ptr)           dbg_free(ptr, __FILE__, __LINE__)

#else

#define json_malloc  malloc
#define json_realloc realloc
#define json_free    free

#endif

/* String */
size_t json_string_length(struct jsonString * string);
const char * json_string_data(struct jsonString * string);

/* Object */
typedef void (*jsonObjectVisitor)(const char * key, struct jsonValue * value, void * user_data);

size_t json_object_size(struct jsonObject * object);
struct jsonValue * json_object_at(struct jsonObject * object, const char * key);
void json_object_for_each(struct jsonObject * object, jsonObjectVisitor action, void * user_data);

/* Array */
typedef void (*jsonArrayVisitor)(size_t index, struct jsonValue * value, void * user_data);

size_t json_array_size(struct jsonArray * array);
void json_array_for_each(struct jsonArray * array, jsonArrayVisitor action, void * user_data);

/* Value */
enum jsonValueKind json_value_kind(struct jsonValue * value);
struct jsonString * json_value_string(struct jsonValue * value);
double json_value_number(struct jsonValue * value);
struct jsonObject * json_value_object(struct jsonValue * value);
struct jsonArray * json_value_array(struct jsonValue * value);
int json_value_bool(struct jsonValue * value);
void json_value_free(struct jsonValue * value);
void json_value_free_internal(struct jsonValue * value);

struct jsonValue * json_parse(const char * json);

