struct ArrayNode {
    struct ArrayNode * next;
    struct jsonValue * value;
};

struct jsonArray {
    struct ArrayNode * first;
    struct ArrayNode * last;
    size_t size;
};

void json_array_init(struct jsonArray * array);
int json_array_add(struct jsonArray * array, struct allocAllocator * allocator,
        struct jsonValue * value);
