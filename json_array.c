#include <stddef.h>
#include <assert.h>

#include "allocator.h"
#include "parser.h"
#include "json_array.h"


extern size_t json_array_size(struct jsonArray * array) {
    return array->size;
}

extern void json_array_for_each(struct jsonArray * array,
        void (*action)(size_t, struct jsonValue *, void *), void * user_data) {
    struct ArrayNode * node;
    size_t i;
    i = 0;
    node = array->first;
    while (node) {
        action(i++, node->value, user_data);
        node = node->next;
    }
}

extern void json_array_init(struct jsonArray * array) {
    array->first = NULL;
    array->last = NULL;
    array->size = 0;
}

static struct ArrayNode * array_node_create(struct jsonValue * value,
        struct allocAllocator * allocator) {
    struct ArrayNode * node;
    node = alloc_malloc(allocator, sizeof(struct ArrayNode));
    if (!node) return NULL;
    node->value = value;
    node->next = NULL;
    return node;
}

extern int json_array_add(struct jsonArray * array,
        struct allocAllocator * allocator, struct jsonValue * value) {
    struct ArrayNode * new_node;
    new_node = array_node_create(value, allocator);
    if (!new_node) return 0;
    if (array->last) {
        assert(array->first);
        array->last->next = new_node;
        array->last = new_node;
    } else {
        assert(!array->first);
        array->first = array->last = new_node;
    }
    ++array->size;
    return 1;
}
