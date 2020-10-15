struct jsonString {
    size_t capacity;
    size_t size;
    char * data;
};

void json_string_init(struct jsonString * string);
void json_string_free_internal(struct jsonString * string);
int json_string_append(struct jsonString * string, char c);
int json_string_shrink(struct jsonString * string);
