/*
 * author: Nolan <sullen.goose@gmail.com>
 * Copy if you can.
 */

#define NDEBUG

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define ARRAY_INITIAL_CAPACITY 4
#define OBJECT_INITIAL_CAPACITY 8

enum LexemeKind {
    JLK_L_BRACE, JLK_R_BRACE, JLK_L_SQUARE_BRACKET, JLK_R_SQUARE_BRACKET,
    JLK_COMMA, JLK_COLON, JLK_TRUE, JLK_FALSE, JLK_NULL, JLK_STRING, JLK_NUMBER
};

struct jsonString {
    char * string;
    size_t length;
};

struct LexemeData {
    enum LexemeKind kind;
    union {
        struct jsonString * unescaped;
        double number;
    } extra;
};

struct ObjectNode {
    struct ObjectNode * left;
    struct ObjectNode * right;
    struct ObjectNode * parent;
    enum Color {
        BLACK, RED
    } color;
    struct jsonString * key;
    struct jsonValue value;
};

struct jsonObject {
    struct ObjectNode * root;
    struct ObjectNode __nil;
    struct ObjectNode * nil;
    size_t size;
};

#define NODE_SIZE 64

struct ArrayNode {
    struct ArrayNode * next;
    struct jsonValue values[NODE_SIZE];
};

struct jsonArray {
    struct ArrayNode * first;
    struct ArrayNode * last;
    struct jsonValue * position;
    struct jsonValue * node_end;
    size_t size;
};

#define MAX_PAGES 4096
#define PAGE_SIZE (256 * 1024)
#define BIG_SIZE (4 * 1024)

struct {
    char * pages[MAX_PAGES];
    size_t counts[MAX_PAGES];
    size_t current_page;
    char * position;
    int andvance_allocation;
} allocator;

static int utf8_bytes_left[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  5,  5, -1, -1,
};

static int is_trailing_utf8_byte[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

#ifdef NDEBUG

static size_t seq_alloc_bytes_left() {
    return allocator.position
        ? allocator.pages[allocator.current_page] + PAGE_SIZE - allocator.position
        : 0;
}

static size_t seq_alloc_shrink_pages() {
    size_t i;
    size_t o;
    o = 0;
    for (i = 0; i < MAX_PAGES; ++i) {
        if (!allocator.pages[i]) continue;
        allocator.pages[o] = allocator.pages[i];
        allocator.counts[o] = allocator.counts[o];
        ++o;
    }
    return o;
}

static int seq_alloc_new_page() {
    char * new_page;
    int i;
    new_page = json_malloc(PAGE_SIZE);
    if (!new_page) return 0;
    for (i = 0; i < MAX_PAGES; ++i) {
        if (!allocator.pages[i]) break;
    }
    if (i == MAX_PAGES) {
        i = seq_alloc_shrink_pages();
        if (i == MAX_PAGES) return 0;
    }
    allocator.current_page = i;
    allocator.counts[i] = 0;
    allocator.position = allocator.pages[i] = new_page;
    return 1;
}

static void * seq_alloc_allocate(size_t bytes) {
    void * memory;
    assert(bytes + 1 < BIG_SIZE);
    assert(!allocator.andvance_allocation);
    if (seq_alloc_bytes_left() < bytes) {
        if (!seq_alloc_new_page()) return NULL;
    }
    ++allocator.counts[allocator.current_page];
    *allocator.position++ = allocator.current_page;
    memory = allocator.position;
    allocator.position += bytes;
    return memory;
}

static void seq_alloc_free(void * memory) {
    unsigned char i;
    i = *((unsigned char *) memory - 1);
    if (--allocator.counts[i]) return;
    json_free(allocator.pages[i]);
    allocator.pages[i] = NULL;
}

static void * seq_alloc_begin_andvance_allocation() {
    if (seq_alloc_bytes_left() < BIG_SIZE + 1) {
        if (!seq_alloc_new_page()) return NULL;
    }
    ++allocator.counts[allocator.current_page];
    *allocator.position++ = allocator.current_page;
    allocator.andvance_allocation = 1;
    return allocator.position;
}

static void seq_alloc_reject_advance_allocation() {
    assert(allocator.andvance_allocation);
    --allocator.counts[allocator.current_page];
    allocator.andvance_allocation = 0;
}

static void seq_alloc_perform_andvance_allocation(void * end) {
    assert(allocator.andvance_allocation);
    assert((void *) allocator.position < end);
    assert((char *) end - allocator.pages[allocator.current_page] < PAGE_SIZE);
    allocator.andvance_allocation = 0;
    allocator.position = end;
}

#else

/* It's easier to debug malloc than custom allocator. */
#define seq_alloc_allocate malloc
#define seq_alloc_free free
#define seq_alloc_begin_andvance_allocation() malloc(16*1024)
#define seq_alloc_reject_advance_allocation()
#define seq_alloc_perform_andvance_allocation(x)

#endif

const char * json_string(struct jsonString * string) {
    return string->string;
}

extern size_t json_object_size(struct jsonObject * object) {
    return object->size;
}

static struct jsonObject * json_object_create() {
    struct jsonObject * object;
    object = seq_alloc_allocate(sizeof(struct jsonObject));
    if (!object) return NULL;
    object->nil = &object->__nil;
    object->nil->parent = object->nil;
    object->nil->color = BLACK;
    object->root = object->nil;
    object->size = 0;
    return object;
}

#define ROTATE(left, right) \
    struct ObjectNode * y; \
    assert(tree->root->parent == tree->nil); \
    assert(x->right != tree->nil); \
    y = x->right; \
    x->right = y->left; \
    if (y->left != tree->nil) y->left->parent = x; \
    y->parent = x->parent; \
    if (x->parent == tree->nil) { \
        tree->root = y; \
    } else if (x == x->parent->left) { \
        x->parent->left = y; \
    } else { \
        x->parent->right = y; \
    } \
    y->left = x; \
    x->parent = y;

static void object_node_left_rotate(struct jsonObject * tree, struct ObjectNode * x) {
    ROTATE(left, right)
}

static void object_node_right_rotate(struct jsonObject * tree, struct ObjectNode * x) {
    ROTATE(right, left)
}

#define FIXUP(left, right) \
    y = z->parent->parent->right; \
    if (y->color == RED) { \
        z->parent->color = BLACK; \
        y->color = BLACK; \
        z->parent->parent->color = RED; \
        z = z->parent->parent; \
    } else { \
        if (z == z->parent->right) { \
            z = z->parent; \
            object_node_##left##_rotate(object, z); \
        } \
        z->parent->color = BLACK; \
        z->parent->parent->color = RED; \
        object_node_##right##_rotate(object, z->parent->parent); \
    }

static void json_object_fixup(struct jsonObject * object, struct ObjectNode * z) {
    struct ObjectNode * y;
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            FIXUP(left, right)
        } else {
            FIXUP(right, left)
        }
    }
    object->root->color = BLACK;
}

static int json_object_add(struct jsonObject * object, struct jsonString * key,
        struct jsonValue value) {
    struct ObjectNode * x;
    struct ObjectNode * y;
    struct ObjectNode * z;
    z = seq_alloc_allocate(sizeof(struct ObjectNode));
    if (!z) return 0;
    z->key = key;
    z->value = value;
    y = object->nil;
    x = object->root;
    while (x != object->nil) {
        y = x;
        if (strcmp(z->key->string, x->key->string) < 0) {
            x = x->left;
        } else x = x->right;
    }
    z->parent = y;
    if (y == object->nil) object->root = z;
    else if (strcmp(z->key->string, y->key->string) < 0) {
        y->left = z;
    } else y->right = z;
    z->left = object->nil;
    z->right = object->nil;
    z->color = RED;
    json_object_fixup(object, z);
    ++object->size;
    return 1;
}

extern struct jsonValue json_object_get_value(struct jsonObject * object,
        const char * key) {
    struct ObjectNode * x;
    struct jsonValue zero;
    int c;
    if (object->root != object->nil) { zero.kind = 0; return zero; }
    x = object->root;
    c = strcmp(key, x->key->string);
    while (x != object->nil && c) {
        x = c < 0 ? x->left : x->right;
    }
    return x->value;
}

extern void json_object_for_each(struct jsonObject * object,
        void (*action)(const char *, struct jsonValue, void *), void * user_data) {
    struct ObjectNode * current;
    struct ObjectNode * predecessor;
    current = object->root;
    while (current != object->nil) {
        if (current->left == object->nil) {
            predecessor = current->right;
            action(current->key->string, current->value, user_data);
            current = predecessor;
        } else {
            predecessor = current->left;
            while (predecessor->right != object->nil && predecessor->right != current) {
                predecessor = predecessor->right;
            }
            if (predecessor->right == object->nil) {
                predecessor->right = current;
                current = current->left;
            } else {
                predecessor->right = object->nil;
                predecessor = current->right;
                action(current->key->string, current->value, user_data);
                current = predecessor;
            }
        }
    }
}

static void json_string_free(struct jsonString * string) {
    if (string->length < BIG_SIZE) {
        seq_alloc_free(string->string);
    } else {
        json_free(string->string);
    }
    seq_alloc_free(string);
}

static void node_free(struct ObjectNode * node) {
    if (node->left->parent != node->left) node_free(node->left);
    if (node->right->parent != node->right) node_free(node->right);
    json_string_free(node->key);
    json_value_free(node->value);
    seq_alloc_free(node);
}

static void json_object_free(struct jsonObject * object) {
    if (object->root != object->nil) {
        node_free(object->root);
    }
    seq_alloc_free(object);
}

/* Returns length of utf8 code point representing 'hex' unicode character. */
static int get_utf8_code_point_length(int32_t hex) {
    if (hex <= 0x007f)
        return 1;
    else if (0x0080 <= hex && hex <= 0x07ff)
        return 2;
    return 3;
}

/* Writes code point representing 'hex' unicode character into 'out' and returns
   pointer to first byte after written code point. */
static size_t put_utf8_code_point(int32_t hex, char * out) {
    if (hex <= 0x007f) {
        *out++ = hex;
        return 1;
    } else if (0x0080 <= hex && hex <= 0x07ff) {
        *out++ = 0xc0 | hex >> 6;
        *out++ = 0x80 | (hex & 0x3f);
        return 2;
    } else {
        *out++ = 0xe0 | hex >> 12;
        *out++ = 0x80 | ((hex >> 6) & 0x3f);
        *out++ = 0x80 | (hex & 0x3f);
        return 3;
    }
}

#define E(c) case (c): *out = (c); break;

#define F(c, d) case (c): *out = (d); break;

/* 'escape' must point to char after backslash '\' symbol. The function calculates
   length of code point produced by this escape sequence and puts it into
   location pointed by 'code_pont_length'. If escape sequence was valid returns
   number of bytes read otherwise returns 0. */
static size_t measure_escape_sequence(const char * escape, size_t * code_point_length) {
    uint32_t hex;
    char * end;
    switch (*escape) {
    case '\\': case '\"': case '/': case 'b':
    case 'f': case 'n': case 'r': case 't':
        *code_point_length = 1;
        return 1;
    case 'u':
        ++escape;
        hex = strtoul(escape, &end, 16);
        if (end - escape != 4) return 0;
        *code_point_length = get_utf8_code_point_length(hex);
        return 5;
    default: return 0;
    }
}

/* Checks that 'code_point' points to valid code point. If it is valid returns
   its lengths in bytes otherwise returns 0. */
static size_t check_code_point(const char * code_point) {
    const char * begin;
    int bytes_left;
    begin = code_point;
    bytes_left = utf8_bytes_left[*(const unsigned char *)code_point];
    if (-1 == bytes_left) return 0;
    ++code_point;
    while (bytes_left--) {
        if (!is_trailing_utf8_byte[*(unsigned char *)code_point]) return 0;
        ++code_point;
    }
    return code_point - begin;
}

/* Validates and calculates length of json string after unescaping escape sequences
   including terminating '\0'. This length is written into location pointed by
   'unescaped_length'. 'json' must point to first char inside of quotes(").
   Returns number of bytes read if string was valid and 0 otherwise. */
static size_t measure_string_lexeme(const char * json, size_t * unescaped_length) {
    const char * begin = json;
    size_t code_point_length;
    size_t bytes_read;
    size_t ul;
    ul = 0;
    while (1) {
        switch (*json) {
        case '"':
            ++json;
            *unescaped_length = ul + 1;
            return json - begin;
        case '\\':
            ++json;
            if (!(bytes_read = measure_escape_sequence(json, &code_point_length))) {
                return 0;
            }
            json += bytes_read;
            ul += code_point_length;
            break;
        default:
            if (!(bytes_read = check_code_point(json))) return 0;
            json += bytes_read;
            ul += bytes_read;
            break;
        }
    }
}

/* Returns non zero value if 'prefix' is prefix of 'str' and 0 otherwise. */
static int is_prefix(const char * prefix, const char * str) {
    while (*prefix && *str && *prefix++ == *str++);
    return *prefix;
}

#define READ_CODE_POINT(inclen, fail_lable, on_success) { \
    switch (*json) { \
    case '"': \
        *out++ = '\0'; \
        on_success; \
        data->extra.unescaped = unescaped; \
        ++json; \
        return json - begin; \
    case '\\': \
        ++json; \
        switch (*json++) { \
        case '\\': *out++ = '\\'; inclen; break; \
        case '"': *out++ = '"'; inclen; break; \
        case '/': *out++ = '/'; inclen; break; \
        case 'b': *out++ = '\b'; inclen; break; \
        case 'f': *out++ = '\f'; inclen; break; \
        case 'n': *out++ = '\n'; inclen; break; \
        case 'r': *out++ = '\r'; inclen; break; \
        case 't': *out++ = '\t'; inclen; break; \
        case 'u': \
            hex = strtoul(json, &end, 16); \
            json += 4; \
            if (end != json) goto fail_lable; \
            bytes = put_utf8_code_point(hex, out); \
            out += bytes; \
            while (bytes--) inclen; \
            break; \
        default: goto fail_lable; \
        } \
        break; \
    default: \
        left = utf8_bytes_left[*(const unsigned char *)json]; \
        if (-1 == left) goto fail_lable; \
        *out++ = *json++; \
        inclen; \
        for (i = 0; i < left; ++i) { \
            if (!is_trailing_utf8_byte[*(unsigned char *)json]) goto fail_lable; \
            *out++ = *json++; \
            inclen; \
        } \
        break; \
    } \
}

static size_t read_string_lexeme(const char * json, struct LexemeData * data) {
    const char * begin;
    char * end;
    size_t bytes;
    int i;
    int left;
    struct jsonString * unescaped;
    int mallocated;
    char * long_unescaped;
    size_t rest_length;
    char * out;
    uint32_t hex;
    begin = json;
    mallocated = 0;
    unescaped = seq_alloc_allocate(sizeof(struct jsonString));
    if (!unescaped) return 0;
    unescaped->string = seq_alloc_begin_andvance_allocation();
    unescaped->length = 1;
    if (!unescaped->string) return 0;
    out = unescaped->string;
    while (unescaped->length < BIG_SIZE) {
        READ_CODE_POINT(++unescaped->length, fail1,
                seq_alloc_perform_andvance_allocation(out));
    }
    /* Rest of code will be executed only for really long lines */
    if (!measure_string_lexeme(json, &rest_length)) goto fail1;
    long_unescaped = json_malloc(unescaped->length + rest_length);
    if (!long_unescaped) goto fail;
    memcpy(long_unescaped, unescaped->string, unescaped->length);
    seq_alloc_reject_advance_allocation();
    unescaped->string = long_unescaped;
    unescaped->length += rest_length;
    out = unescaped->string;
    while (1) READ_CODE_POINT((void) 0, fail, (void) 0)
fail:
    json_free(long_unescaped);
fail1:
    json_string_free(data->extra.unescaped);
    return 0;
}

#define SINGLE_CHAR_LEXEME(c, k) \
    case (c): \
        ++json; \
        data->kind = (k); \
        return json - begin;

#define CONCRETE_WORD_LEXEME(c, w, k) \
    case (c):  \
        if (is_prefix((w), json)) return 0; \
        json += sizeof(w) - 1; \
        data->kind = (k); \
        return json - begin;

#define SINGLE_CHAR_SEQUENCE(c, d) \
    case (c): *out++ = (d); ++length; ++json; break; \

/* Reads next lexeme pointed by 'json' and collects data about it. Lexeme can
   be preceded by whitespaces. Data about lexeme is written into location pointed
   by 'data'. Fields of the data that will be filled always: kind, begin. Ex field
   will be filled if kind is JLK_NUMBER or JLK_STRING it will be number and
   unescaped_length correspondingly. Returns number of bytes read if lexeme was
   successfully read and 0 otherwise. */
extern size_t next_lexeme(const char * json, struct LexemeData * data) {
    const char * begin;
    char * end;
    size_t t;
    begin = json;
    while (isspace(*json)) ++json;
    switch (*json) {
    case '\0': return 0;
    SINGLE_CHAR_LEXEME('{', JLK_L_BRACE)
    SINGLE_CHAR_LEXEME('}', JLK_R_BRACE)
    SINGLE_CHAR_LEXEME('[', JLK_L_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(']', JLK_R_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(',', JLK_COMMA)
    SINGLE_CHAR_LEXEME(':', JLK_COLON)
    CONCRETE_WORD_LEXEME('t', "true",  JLK_TRUE)
    CONCRETE_WORD_LEXEME('f', "false", JLK_FALSE)
    CONCRETE_WORD_LEXEME('n', "null",  JLK_NULL)
    case '"':
        ++json;
        t = read_string_lexeme(json, data);
        if (!t) return 0;
        data->kind = JLK_STRING;
        json += t;
        return json - begin;
    default:
        data->extra.number = strtod(json, &end);
        if (end == json) return 0;
        json = end;
        data->kind = JLK_NUMBER;
        return json - begin;
    }
}

/* Parses json object and stores pointer to it into '*object'. 'json' must
   point to next lexeme after '{'. Object will be created in heap so don't
   forget to free it using json_object_free function. */
static size_t json_parse_object(const char * json, struct jsonObject ** object) {
    const char * begin;
    struct jsonString * key;
    struct jsonValue value;
    struct LexemeData data;
    size_t bytes_read;
    begin = json;
    if (!(bytes_read = next_lexeme(json, &data))) return 0;
    json += bytes_read;
    *object = json_object_create();
    if (data.kind == JLK_R_BRACE) {
        return json - begin;
    }
    while (1) {
        if (JLK_STRING != data.kind) goto fail2;
        key = data.extra.unescaped;
        if (!(bytes_read = next_lexeme(json, &data)) || JLK_COLON != data.kind) goto fail1;
        json += bytes_read;
        if (!(bytes_read = json_parse_value(json, &value))) goto fail1;
        json += bytes_read;
        json_object_add(*object, key, value);
        if (!(bytes_read = next_lexeme(json, &data))) goto fail2;
        json += bytes_read;
        if (JLK_R_BRACE == data.kind) break;
        if (JLK_COMMA != data.kind) goto fail2;
        if (!(bytes_read = next_lexeme(json, &data))) goto fail2;
        json += bytes_read;
    }
    return json - begin;
fail1:
    json_string_free(key);
fail2:
    json_object_free(*object);
    *object = NULL;
    return 0;
}

extern size_t json_array_size(struct jsonArray * array) {
    return array->size;
}

#include <stdio.h>

static struct jsonArray * json_array_create() {
    struct jsonArray * array;
    array = seq_alloc_allocate(sizeof(struct jsonArray));
    if (!array) return 0;
    array->first = NULL;
    array->last = NULL;
    array->position = NULL;
    array->node_end = NULL;
    array->size = 0;
    return array;
}

static int json_array_ensure_free_space(struct jsonArray * array) {
    struct ArrayNode * new_node;
    if (array->position && array->position != array->node_end) return 1;
    new_node = seq_alloc_allocate(sizeof(struct ArrayNode));
    if (!new_node) return 0;
    new_node->next = NULL;
    array->position = new_node->values;
    array->node_end = new_node->values + NODE_SIZE;
    if (array->last) {
        array->last->next = new_node;
        array->last = new_node;
    } else {
        array->first = array->last = new_node;
    }
    return 1;
}

static int json_array_add(struct jsonArray * array, struct jsonValue value) {
    if (!json_array_ensure_free_space(array)) return 0;
    *array->position++ = value;
    array->size++;
    return 1;
}

static void json_array_free(struct jsonArray * array) {
    struct ArrayNode * p;
    struct ArrayNode * next;
    size_t i;
    size_t n;
    p = array->first;
    if (!p) {
        seq_alloc_free(array);
        return;
    }
    while (p->next) {
        for (i = 0; i < NODE_SIZE; ++i) {
            json_value_free(p->values[i]);
        }
        next = p->next;
        seq_alloc_free(p);
        p = next;
    }
    n = array->position - p->values;
    for (i = 0; i < n; ++i) {
        json_value_free(p->values[i]);
    }
    seq_alloc_free(p);
    seq_alloc_free(array);
}

/* Iterates over 'array' and for each value calls 'action' with index, value itself and
   'user_data'. */
extern void json_array_for_each(struct jsonArray * array,
        void (*action)(size_t, struct jsonValue, void *), void * user_data) {
    struct ArrayNode * p;
    size_t i;
    size_t j;
    size_t k;
    size_t n;
    size_t number_of_nodes;
    k = 0;
    if (!(p = array->first)) return;
    number_of_nodes = (array->size + NODE_SIZE - 1) / NODE_SIZE;
    for (i = 0; i < number_of_nodes; ++i, p = p->next) {
        n = i == (number_of_nodes - 1)
            ? array->position - p->values
            : NODE_SIZE;
        for (j = 0; j < n; ++j) {
            action(k++, p->values[j], user_data);
        }
    }
}

/* Parses json array. 'json' must point to next lexeme after '['. Don't forget to free. */
static size_t json_parse_array(const char * json, struct jsonArray ** array) {
    const char * begin;
    struct jsonValue value;
    struct LexemeData data;
    size_t bytes_read;
    begin = json;
    if (!(bytes_read = next_lexeme(json, &data))) return 0;
    *array = json_array_create();
    if (JLK_R_SQUARE_BRACKET == data.kind) {
        json += bytes_read;
        return json - begin;
    }
    while (1) {
        if (!(bytes_read = json_parse_value(json, &value))) goto fail;
        json += bytes_read;
        if (!(bytes_read = next_lexeme(json, &data))) goto fail;
        json += bytes_read;
        if (!json_array_add(*array, value)) goto fail;
        if (JLK_R_SQUARE_BRACKET == data.kind) break;
        if (JLK_COMMA != data.kind) goto fail;
    }
    return json - begin;
fail:
    json_array_free(*array);
    *array = NULL;
    return 0;
}

/* Parses json value into 'value'. Don't forget to call json_value_free.  */
extern size_t json_parse_value(const char * json, struct jsonValue * value) {
    const char * begin;
    struct LexemeData data;
    struct jsonValue ret;
    size_t bytes_read;
    begin = json;
    if (!(bytes_read = next_lexeme(json, &data))) return 0;
    json += bytes_read;
    switch (data.kind) {
    case JLK_L_BRACE:
        ret.kind = JVK_OBJ;
        if (!(bytes_read = json_parse_object(json, &ret.value.object))) return 0;
        json += bytes_read;
        break;
    case JLK_L_SQUARE_BRACKET:
        ret.kind = JVK_ARR;
        if (!(bytes_read = json_parse_array(json, &ret.value.array))) return 0;
        json += bytes_read;
        break;
    case JLK_TRUE:
        ret.kind = JVK_BOOL;
        ret.value.boolean = 1;
        break;
    case JLK_FALSE:
        ret.kind = JVK_BOOL;
        ret.value.boolean = 0;
        break;
    case JLK_NULL:
        ret.kind = JVK_NULL;
        break;
    case JLK_STRING:
        ret.kind = JVK_STR;
        ret.value.string = data.extra.unescaped;
        break;
    case JLK_NUMBER:
        ret.kind = JVK_NUM;
        ret.value.number = data.extra.number;
        break;
    default: return 0;
    }
    *value = ret;
    return json - begin;
}

extern void json_value_free(struct jsonValue value) {
    switch (value.kind) {
    case JVK_OBJ: json_object_free(value.value.object); break;
    case JVK_ARR: json_array_free(value.value.array);   break;
    case JVK_STR: json_string_free(value.value.string); break;
    default:;
    }
}
