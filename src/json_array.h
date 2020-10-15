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
void json_array_free_internal(struct jsonArray * array);
int json_array_add(struct jsonArray * array, struct jsonValue * value);
