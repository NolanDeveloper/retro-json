// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

struct jsonObject;
struct jsonArray;

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

struct jsonPair {
    char * key;
    struct jsonValue value;
};

struct jsonObject {
    struct jsonPair * buckets;
    size_t size;
    size_t capacity;
};

struct jsonArray {
    struct jsonValue * values;
    size_t size;
    size_t capacity;
};

size_t json_parse_value(const char * json, struct jsonValue * value);
void json_value_free(struct jsonValue value);
struct jsonValue json_object_value_at(struct jsonObject * object, const char * key);

extern void * (*json_malloc)(size_t);
extern void (*json_free)(void *);
