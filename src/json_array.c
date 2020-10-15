#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include "json_parser.h"
#include "json_array.h"

extern size_t json_array_size(struct jsonArray * array) {
    return array->size;
}

extern void json_array_for_each(struct jsonArray * array, jsonArrayVisitor action, void * user_data) {
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

extern void json_array_free_internal(struct jsonArray * array) {
    struct ArrayNode * node, * next;
    if (!array) return;
    node = array->first;
    while (node) {
        next = node->next;
        json_value_free(node->value);
        json_free(node);
        node = next;
    }
    array->first = NULL;
    array->last = NULL;
    array->size = 0;
}

static struct ArrayNode * array_node_create(struct jsonValue * value) {
    struct ArrayNode * node;
    node = json_malloc(sizeof(struct ArrayNode));
    if (!node) return NULL;
    node->value = value;
    node->next = NULL;
    return node;
}

extern int json_array_add(struct jsonArray * array, struct jsonValue * value) {
    struct ArrayNode * new_node;
    new_node = array_node_create(value);
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
