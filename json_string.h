struct jsonString {
    size_t capacity;
    size_t size;
    char * data;
};

struct jsonString * json_string_create(struct allocAllocator * allocator);
void json_string_init(struct jsonString * string);
int json_string_append(struct jsonString * string,
        struct allocAllocator * allocator, char c);
int json_string_shrink(struct jsonString * string, struct allocAllocator * allocator);
