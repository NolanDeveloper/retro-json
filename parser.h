/*
 * author: Nolan <sullen.goose@gmail.com>
 * Copy if you can.
 */

struct jsonValue;

struct jsonObject {
    char ** keys;
    struct jsonValue * values;
    size_t size;
    size_t capacity;
};

struct jsonArray {
    struct jsonValue * values;
    size_t size;
    size_t capacity;
};

enum jsonValueKind { JVK_STR = 1, JVK_NUM, JVK_OBJ, JVK_ARR, JVK_BOOL, JVK_NULL };

struct jsonValue {
    enum jsonValueKind kind;
    union {
        char * string;
        double number;
        struct jsonObject * object;
        struct jsonArray * array;
        int boolean;
    } value;
};

/* Parses json value from string pointed by 'json' into variable pointed by 'value'.
   Returns number of bytes read. */
size_t json_parse_value(const char * json, struct jsonValue * value);

/* Free memory held by by value. */
void json_value_free(struct jsonValue value);

/* If 'object' has attribute 'key' retuns its value otherwise returns jsonValue with
   kind set to 0. */
struct jsonValue json_object_get_value(struct jsonObject * object, const char * key);

/* Traverses 'object' and calls 'action' for each attribute with key, value of
   attribute and 'user_data'. */
void json_object_for_each(struct jsonObject * object,
        void (*action)(const char *, struct jsonValue, void *), void * user_data);

/* Traverses 'array' and calls 'action' for each value with index of the value,
   value itself and 'user_data'. */
void json_array_for_each(struct jsonArray * array,
        void (*action)(size_t, struct jsonValue, void *), void * user_data);

/* Library user must define these variables. */
extern void * (*json_malloc)(size_t);
extern void (*json_free)(void *);
