/* Value is located next to TreeNode in memory */
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
int json_object_add(struct jsonObject * object, struct allocAllocator * allocator,
        struct jsonString * key, struct jsonValue * value);

