// BSD 3-Clause License
//
// Copyright (c) 2018, Gregory Meyer
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "internal/node.h"

#include <assert.h>

TreeErrorE TreeNode_init(TreeNode *self, const void *key, void *value) {
    assert(self);

    self->left = NULL;
    self->right = NULL;
    self->parent = NULL;
    self->height = 1;
    self->key = key;
    self->value = value;

    return TREE_SUCCESS;
}

TreeErrorE TreeNode_destroy(TreeNode *self) {
    assert(self);

    if (self->left) {
        TreeNode_destroy(self->left);
        self->left = NULL;
    }

    if (self->right) {
        TreeNode_destroy(self->right);
        self->right = NULL;
    }

    self->parent = NULL;
    self->key = NULL;
    self->value = NULL;
    self->height = 1;

    return TREE_SUCCESS;
}

static ptrdiff_t get_height(TreeNode *self) {
    if (!self) {
        return 0;
    }

    return self->height;
}

static void update_height(TreeNode *self) {
    assert(self);

    const ptrdiff_t left_height = get_height(self->left);
    const ptrdiff_t right_height = get_height(self->right);

    if (left_height > right_height) {
        self->height = left_height + 1;
    } else {
        self->height = right_height + 1;
    }

    if (!self->parent) {
        return;
    }

    update_height(self->parent);
}

static TreeNode* rotate_right(TreeNode *self) {
    assert(self);
    assert(self->left);

    TreeNode *const left = self->left;

    if (left->right) {
        left->right->parent = self;
    }

    self->left = left->right;
    left->right = self;

    if (self->parent) {
        if (self->parent->left == self) {
            self->parent->left = left;
        } else {
            self->parent->right = left;
        }
    }

    left->parent = self->parent;
    self->parent = left;

    update_height(self);

    return left;
}

static TreeNode* rotate_left(TreeNode *self) {
    assert(self);
    assert(self->right);

    TreeNode *const right = self->right;

    if (right->left) {
        right->left->parent = self;
    }

    self->right = right->left;
    right->left = self;

    if (self->parent) {
        if (self->parent->left == self) {
            self->parent->left = right;
        } else {
            self->parent->right = right;
        }
    }

    right->parent = self->parent;
    self->parent = right;

    update_height(self);

    return right;
}

TreeErrorE TreeNode_lower_bound(TreeNode *self, const void *key,
                                TreeComparatorT comparator, TreeNode **bound) {
    assert(self);
    assert(key);
    assert(bound);

    const int compare = comparator(self->key, key);

    if (compare < 0) {
        if (!self->right) {
            return TREE_NO_SUCH_KEY;
        }

        return TreeNode_lower_bound(self->right, key, comparator, bound);
    } else if (compare > 0 && self->left) {
        if (TreeNode_lower_bound(self->left, key, comparator, bound) != TREE_SUCCESS) {
            *bound = self;
        }

        return TREE_SUCCESS;
    }

    *bound = self;

    return TREE_SUCCESS;
}

static TreeErrorE do_insert(TreeNode *self, TreeNode *to_insert, TreeComparatorT comparator) {
    assert(self);
    assert(to_insert);

    const int compare = comparator(self->key, to_insert->key);

    if (compare == 0) {
        return TREE_DUPLICATE_KEY;
    }

    if (compare > 0) {
        if (self->left) {
            return do_insert(self->left, to_insert, comparator);
        }

        self->left = to_insert;
    } else if (compare < 0) {
        if (self->right) {
            return do_insert(self->right, to_insert, comparator);
        }

        self->right = to_insert;
    }

    to_insert->parent = self;

    return TREE_SUCCESS;
}

static ptrdiff_t get_balance_factor(TreeNode *self) {
    assert(self);

    return get_height(self->left) - get_height(self->right);;
}

static void rebalance(TreeNode *self) {
    assert(self);

    const ptrdiff_t balance_factor = get_balance_factor(self);

    if (balance_factor > 1) {
        if (self->left && get_balance_factor(self->left) < 0) {
            rotate_left(self->left);
        }

        rotate_right(self);
    } else if (balance_factor < -1) {
        if (self->right && get_balance_factor(self->right) > 0) {
            rotate_right(self->right);
        }

        rotate_left(self);
    }

    if (self->parent) {
        rebalance(self->parent);
    }
}

TreeErrorE TreeNode_insert(TreeNode *self, TreeNode *to_insert, TreeComparatorT comparator) {
    assert(self);
    assert(to_insert);

    const TreeErrorE ret = do_insert(self, to_insert, comparator);

    if (ret != TREE_SUCCESS) {
        return ret;
    }

    update_height(to_insert);
    rebalance(to_insert);

    return TREE_SUCCESS;
}

static void remove_parent(TreeNode *self) {
    assert(self);

    if (self->parent && self->parent->left == self) {
        self->parent->left = NULL;
    } else if (self->parent) {
        self->parent->right = NULL;
    }

    self->parent = NULL;
}

static void replace_left(TreeNode *self) {
    assert(self);
    assert(self->left);
    assert(!self->right);

    if (self->parent && self->parent->left == self) {
        self->parent->left = self->left;
    } else if (self->parent) {
        self->parent->right = self->left;
    }

    self->left->parent = self->parent;
    self->left = NULL;
    self->parent = NULL;
}

static void replace_right(TreeNode *self) {
    assert(self);
    assert(self->right);
    assert(!self->left);

    if (self->parent && self->parent->left == self) {
        self->parent->left = self->right;
    } else if (self->parent) {
        self->parent->right = self->right;
    }

    self->right->parent = self->parent;
    self->right = NULL;
    self->parent = NULL;
}

static TreeNode* inorder_successor(TreeNode *self) {
    assert(self);
    assert(self->right);

    TreeNode *successor = self->right;

    for (; successor->left; successor = successor->left) { }

    return successor;
}

static void replace_successor(TreeNode *self) {
    assert(self);
    assert(self->left);
    assert(self->right);

    TreeNode *const successor = inorder_successor(self);

    remove_parent(successor);
}

TreeErrorE TreeNode_erase(TreeNode *self) {
    assert(self);

    if (!self->left && !self->right) {
        remove_parent(self);

        return TREE_SUCCESS;
    } else if (self->left && !self->right) {
        replace_left(self);
    } else if (!self->left && self->right) {
        replace_right(self);
    } else {
        replace_successor(self);
    }

    return TREE_SUCCESS;
}
