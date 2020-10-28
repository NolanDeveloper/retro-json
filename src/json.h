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
struct jsonValue;

struct jsonObject {
    /*! Hash table */
    size_t capacity;
    size_t size;
    struct jsonObjectEntry *entries;
    /*! List of entries in the order they were added */
    struct jsonObjectEntry *first;
    struct jsonObjectEntry *last;
};

enum jsonValueKind { JVK_STR, JVK_NUM, JVK_OBJ, JVK_ARR, JVK_BOOL, JVK_NULL };

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

struct jsonObjectEntry {
    struct jsonString *key;
    struct jsonValue *value;
    struct jsonObjectEntry *next;
    struct jsonObjectEntry *prev;
};

struct jsonValue * json_value_create_number(double number);
struct jsonValue * json_value_create_string(const char * string);
struct jsonValue * json_value_create_object(void);
struct jsonValue * json_value_create_array(void);
struct jsonValue * json_value_create_boolean(int boolean);
struct jsonValue * json_value_create_null(void);

bool json_value_object_add(struct jsonValue * object, const char * key, struct jsonValue * value);
bool json_value_array_append(struct jsonValue * array, struct jsonValue * value);
/*! @todo int json_value_array_remove */
struct jsonValue * json_value_object_lookup(struct jsonValue * object, const char * key);
size_t json_value_array_size(struct jsonValue * array);
struct jsonValue * json_value_array_at(struct jsonValue * array, size_t index);
void json_value_free(struct jsonValue * value);

struct jsonValue * json_parse(const char * json);

/*! @brief Pretty prints json value.
 *
 * Acts like snprint. Passing size = 0 allows to precalculate out buffer size.
 * Then you may allocate buffer of required size and pass to the function
 * again.
 *
 * If size != 0 output guaranteed have '\0' at the end.
 *
 * @param out Output buffer or NULL.
 * @param size Size of out buffer. The function doesn't write more than that.
 * @param value Json value to print.
 * @returns Number of bytes (excluding terminating '\0') that would be written
 * if output buffer was of infinite size.
 */
size_t json_pretty_print(char * out, size_t size, struct jsonValue * value);



