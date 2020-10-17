struct jsonString {
    size_t capacity;
    size_t size;
    char * data;
    unsigned hash;
};

void json_string_init(struct jsonString * string);
void json_string_free_internal(struct jsonString * string);
int json_string_append(struct jsonString * string, char c);
int json_string_shrink(struct jsonString * string);
unsigned json_string_hash(const char * str);

struct jsonArrayNode {
    struct jsonArrayNode * next;
    struct jsonValue * value;
};

struct jsonArray {
    struct jsonArrayNode * first;
    struct jsonArrayNode * last;
    size_t size;
};

typedef void (*jsonArrayVisitor)(size_t index, struct jsonValue * value, void * user_data);

void json_array_init(struct jsonArray * array);
void json_array_free_internal(struct jsonArray * array);
int json_array_add(struct jsonArray * array, struct jsonValue * value);
size_t json_array_size(struct jsonArray * array);
void json_array_for_each(struct jsonArray * array, jsonArrayVisitor action, void * user_data);

struct jsonObjectEntry;
struct jsonValue;

struct jsonObject {
    /*! Hash table */
    size_t capacity;
    size_t size;
    struct jsonObjectEntry * entries;
    /*! List in which entries were added */
    struct jsonObjectEntry * first;
    struct jsonObjectEntry * last;
};

void json_object_init(struct jsonObject * object);
void json_object_free_internal(struct jsonObject * object);
int json_object_add(struct jsonObject * object, struct jsonString * key, struct jsonValue * value);
struct jsonValue * json_object_at(struct jsonObject * object, const char * key);

enum jsonValueKind { JVK_STR, JVK_NUM, JVK_OBJ, JVK_ARR, JVK_BOOL, JVK_NULL };

struct jsonValue {
    enum jsonValueKind kind; 
    union {
        double number;
        struct jsonString string;
        struct jsonObject object;
        struct jsonArray array;
        int boolean;
    } v;
};

struct jsonObjectEntry {
    struct jsonString * key;
    struct jsonValue * value;
    struct jsonObjectEntry * next;
    struct jsonObjectEntry * prev;
};

void json_value_free(struct jsonValue * value);
void json_value_free_internal(struct jsonValue * value);

struct jsonValue * json_parse(const char * json);

size_t json_pretty_print(char * out, size_t size, struct jsonValue * value);
