#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "allocator.h"
#include "parser.h"
#include "json_string.h"
#include "json_object.h"

extern size_t json_object_size(struct jsonObject * object) {
    return object->size;
}

extern struct jsonValue * json_object_at(struct jsonObject * object, const char * key) {
    struct TreeNode * x;
    int c;
    if (object->root != object->nil) return NULL;
    x = object->root;
    c = strcmp(key, x->key->data);
    while (x != object->nil && c) {
        x = c < 0 ? x->left : x->right;
    }
    return x->value;
}

extern void json_object_for_each(struct jsonObject * object,
        void (*action)(const char *, struct jsonValue *, void *), void * user_data) {
    struct TreeNode * current;
    struct TreeNode * predecessor;
    current = object->root;
    while (current != object->nil) {
        if (current->left == object->nil) {
            predecessor = current->right;
            action(current->key->data, current->value, user_data);
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
                action(current->key->data, current->value, user_data);
                current = predecessor;
            }
        }
    }
}

extern void json_object_init(struct jsonObject * object) {
    object->nil = &object->__nil;
    object->nil->parent = object->nil;
    object->nil->color = BLACK;
    object->root = object->nil;
    object->size = 0;
}

#define ROTATE(left, right) \
    struct TreeNode * y; \
    assert(object->root->parent == object->nil); \
    assert(x->right != object->nil); \
    y = x->right; \
    x->right = y->left; \
    if (y->left != object->nil) y->left->parent = x; \
    y->parent = x->parent; \
    if (x->parent == object->nil) { \
        object->root = y; \
    } else if (x == x->parent->left) { \
        x->parent->left = y; \
    } else { \
        x->parent->right = y; \
    } \
    y->left = x; \
    x->parent = y;

static void node_left_rotate(struct jsonObject * object, struct TreeNode * x) {
    ROTATE(left, right)
}

static void node_right_rotate(struct jsonObject * object, struct TreeNode * x) {
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
            node_##left##_rotate(object, z); \
        } \
        z->parent->color = BLACK; \
        z->parent->parent->color = RED; \
        node_##right##_rotate(object, z->parent->parent); \
    }

static void tree_fixup(struct jsonObject * object, struct TreeNode * z) {
    struct TreeNode * y;
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            FIXUP(left, right)
        } else {
            FIXUP(right, left)
        }
    }
    object->root->color = BLACK;
}

static struct TreeNode * create_node(struct allocAllocator * allocator,
        struct jsonString * key, struct jsonValue * value) {
    struct TreeNode * node;
    node = alloc_malloc(allocator, sizeof(struct TreeNode));
    if (!node) return NULL;
    node->key = key;
    node->value = value;
    return node;
}

extern int json_object_add(struct jsonObject * object, struct allocAllocator * allocator,
        struct jsonString * key, struct jsonValue * value) {
    struct TreeNode * x, * y, * z;
    z = create_node(allocator, key, value);
    if (!z) return 0;
    y = object->nil;
    x = object->root;
    while (x != object->nil) {
        y = x;
        if (strcmp(z->key->data, x->key->data) < 0) {
            x = x->left;
        } else x = x->right;
    }
    z->parent = y;
    if (y == object->nil) object->root = z;
    else if (strcmp(z->key->data, y->key->data) < 0) {
        y->left = z;
    } else y->right = z;
    z->left = object->nil;
    z->right = object->nil;
    z->color = RED;
    tree_fixup(object, z);
    ++object->size;
    return 1;
}

