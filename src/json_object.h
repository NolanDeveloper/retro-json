struct TreeNode {
    struct TreeNode * left;
    struct TreeNode * right;
    struct TreeNode * parent;
    enum Color {
        BLACK, RED
    } color;
    struct jsonString * key;
    struct jsonValue * value;
};

struct jsonObject {
    struct TreeNode * root;
    struct TreeNode __nil;
    struct TreeNode * nil;
    size_t size;
};

void json_object_init(struct jsonObject * object);
void json_object_free_internal(struct jsonObject * object);
int json_object_add(struct jsonObject * object, struct jsonString * key, struct jsonValue * value);
