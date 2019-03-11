/*  MIT License
 *
 *  Copyright (c) 2019 Gregory Meyer
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation
 *  files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy,
 *  modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including
 *  the next paragraph) shall be included in all copies or substantial
 *  portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#include <avlbst.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/** AVL Tree node. */
struct AvlNode {
    AvlNode *left;
    AvlNode *right;
    void *key;
    void *value;
    /* If not one of (-1, 0, 1), needs to be rebalanced. */
    signed char balance_factor;
};

/**
 *  Initializes an empty AvlMap.
 *
 *  @param self Must not be NULL. Must not be initialized.
 *  @param compare Must not be NULL. Will be used to compare keys as if
 *                 by compare(lhs, rhs, compare_arg). Return values
 *                 should have the same meaning as strcmp and should
 *                 form a total order over the set of keys.
 *  @param compare_arg Passed to compare when invoked.
 *  @param deleter Must not be NULL. Will be used to free key-value
 *                 pairs when they are no longer usable by the tree as
 *                 if by deleter(key, value, deleter_arg).
 *  @param deleter_arg Passed to deleter when invoked.
 */
void AvlMap_init(AvlMap *self, AvlComparator compare, void *compare_arg,
                 AvlDeleter deleter, void *deleter_arg) {
    assert(self);
    assert(compare);
    assert(deleter);

    self->root = NULL;
    self->len = 0;
    self->compare = compare;
    self->compare_arg = compare_arg;
    self->deleter = deleter;
    self->deleter_arg = deleter_arg;
}

static AvlNode* alloc_node(void *key, void *value);

/**
 *  Inserts a (key, value) pair into an AvlMap, taking ownership of
 *  them.
 *
 *  @param self Must not be NULL. Must be initialized.
 *  @param key If a key in this AvlMap compares equal, ownership will
 *             remain with the caller and will not be transferred to
 *             the AvlMap.
 *  @returns The previous value associated with key, if it exists.
 *           Ownership is transferred back to the caller in this case.
 */
void* AvlMap_insert(AvlMap *self, void *key, void *value) {
    assert(self);

    if (!self->root) {
        self->root = alloc_node(key, value);
        ++self->len;

        return NULL;
    } else {
        AvlNode *current = self->root;

        while (1) {
            const int compare = self->compare(key, current->key, self->compare_arg);

            if (compare == 0) { /* key == current */
                AvlNode *const previous = current->value;
                current->value = value;

                return previous;
            } else if (compare < 0) { /* key < current */
                if (current->left) {
                    current = current->left;
                } else {
                    current->left = alloc_node(key, value);
                    ++self->len;

                    return NULL;
                }
            } else { /* compare > 0, key > current */
                if (current->right) {
                    current = current->right;
                } else {
                    current->right = alloc_node(key, value);
                    ++self->len;

                    return NULL;
                }
            }
        }
    }
}

static AvlNode* alloc_node(void *key, void *value) {
    AvlNode *const node = (AvlNode*) calloc(1, sizeof(AvlNode));

    if (!node) { /* highly unlikely, but sure */
        fprintf(stderr, "libavlbst: alloc_node: calloc() returned NULL");
        abort();
    }

    node->key = key;
    node->value = value;

    return node;
}

/**
 *  Destroys an AvlMap, removing all members.
 *
 *  Equivalent to AvlMap_clear. Runs in O(1) stack frames and O(n) time
 *  complexity.
 *
 *  @param self Must not be NULL. Must be initialized.
 */
void AvlMap_destroy(AvlMap *self) {
    assert(self);

    AvlMap_clear(self);
}

/**
 *  Clears the map, removing all members.
 *
 *  Runs in O(1) stack frames and O(n) time complexity.
 *
 *  @param self Must not be NULL. Must be initialized.
 */
void AvlMap_clear(AvlMap *self) {
    AvlNode *current;

    assert(self);

    if (!self->root) {
        return;
    }

    /* Morris in-order tree traversal */
    current = self->root;
    while (current) {
        if (!current->left) {
            AvlNode *const next = current->right;

            self->deleter(current->key, current->value, self->deleter_arg);
            free(current);

            current = next;
        } else {
            /* rightmost node of tree with root current->left */
            AvlNode *predecessor = current->left;

            while (predecessor->right && predecessor->right != current) {
                predecessor = predecessor->right;
            }

            if (!predecessor->right) {
                predecessor->right = current;
                current = current->left;
            } else { /* predecessor->right == current */
                AvlNode *const next = current->right;

                self->deleter(current->key, current->value, self->deleter_arg);
                free(current);

                predecessor->right = NULL;
                current = next;
            }
        }
    }

    self->len = 0;
}