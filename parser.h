// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

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
        char * str;
        double num;
        struct jsonObject * obj;
        struct jsonArray * arr;
        int bul;
    };
};

size_t json_parse_value(const char * json, struct jsonValue * value);

void json_value_free(struct jsonValue value);

struct jsonValue json_object_get_value(struct jsonObject * object, const char * key);

extern void * (*json_malloc)(size_t);
extern void (*json_free)(void *);
