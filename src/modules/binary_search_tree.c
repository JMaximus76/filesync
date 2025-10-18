#include "binary_search_tree.h"
#include <assert.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>

#define BST_RED   0
#define BST_BLACK 1

typedef struct bst_location bst_location_t;


struct bst_location {
    jfs_bst_node_t *node;
    jfs_bst_node_t *parent;
    int             parent_cmp;
};

static void bst_find(const jfs_bst_t *tree, const void *key, bst_location_t *location_init);
static void bst_insert(jfs_bst_t *tree, jfs_bst_node_t *node, const bst_location_t *location);
static void bst_delete(jfs_bst_t *tree, jfs_bst_node_t *node);

static void bst_fixup_cached_insert(jfs_bst_t *tree, const bst_location_t *location);
static void bst_fixup_cached_delete(jfs_bst_t *tree, const jfs_bst_node_t *node);
static void bst_fixup_insert(jfs_bst_t *tree, jfs_bst_node_t *node);
static void bst_fixup_delete(jfs_bst_t *tree, jfs_bst_node_t *node);

static void            bst_rotate_left(jfs_bst_t *tree, jfs_bst_node_t *x);
static void            bst_rotate_right(jfs_bst_t *tree, jfs_bst_node_t *x);
static void            bst_transplant(jfs_bst_t *tree, jfs_bst_node_t *old, jfs_bst_node_t *new);
static jfs_bst_node_t *bst_local_minimum(const jfs_bst_t *tree, jfs_bst_node_t *start);
static jfs_bst_node_t *bst_local_maximum(const jfs_bst_t *tree, jfs_bst_node_t *start);

static jfs_bst_node_t *bst_parent(const jfs_bst_node_t *node);
static void            bst_set_parent(jfs_bst_node_t *node, jfs_bst_node_t *parent);
static uint64_t        bst_color(const jfs_bst_node_t *node);
static void            bst_set_color(jfs_bst_node_t *node, uint64_t color);
static void            bst_set_parent_color(jfs_bst_node_t *node, jfs_bst_node_t *parent, uint64_t color);

void jfs_bst_init(jfs_bst_t *tree_init, const jfs_bst_conf_t *conf, jfs_err_t *err) {
    assert(conf != NULL);

    jfs_fl_init(&tree_init->free_list, &conf->fl_conf, err);
    VOID_CHECK_ERR;

    tree_init->nil = &tree_init->nil_storage;
    tree_init->nil_storage.left = tree_init->nil;
    tree_init->nil_storage.right = tree_init->nil;
    bst_set_parent_color(tree_init->nil, tree_init->nil, BST_BLACK);

    tree_init->smallest = tree_init->nil;
    tree_init->largest = tree_init->nil;
    tree_init->root = tree_init->nil;

    tree_init->fns = conf->fns;
    VOID_FAIL_IF(tree_init->fns.cmp != NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(tree_init->fns.pack != NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(tree_init->fns.unpack != NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(tree_init->fns.attach != NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(tree_init->fns.detach != NULL, JFS_ERR_BAD_CONF);
}

void jfs_bst_puts(jfs_bst_t *tree, const void *value, const void *key, jfs_err_t *err) {
    VOID_FAIL_IF(tree->free_list.count == 0, JFS_ERR_FULL);
    void *const           container = jfs_fl_alloc(&tree->free_list);
    jfs_bst_node_t *const node = tree->fns.pack(container, value);

    bst_location_t location = {0};
    bst_find(tree, key, &location);

    if (location.node != tree->nil) { // key is a dupe
        tree->fns.attach(location.node, container);
    } else { // key is not a dupe
        bst_insert(tree, node, &location);
    }
}

void jfs_bst_gets(jfs_bst_t *tree, void *value_out, const void *key, jfs_err_t *err) {
    VOID_FAIL_IF(tree->root == tree->nil, JFS_ERR_EMPTY);

    bst_location_t location = {0};
    bst_find(tree, key, &location);

    VOID_FAIL_IF(location.node == tree->nil, JFS_ERR_BST_BAD_KEY);

    void      *container = NULL;
    const bool do_delete = tree->fns.detach(location.node, &container);
    tree->fns.unpack(container, value_out);
    if (do_delete) {
        bst_delete(tree, location.node);
    }
}

void jfs_bst_cached_gets(jfs_bst_t *tree, void *value_out, jfs_bst_cached_t type, jfs_err_t *err) {
    VOID_FAIL_IF(tree->root == tree->nil, JFS_ERR_EMPTY);

    jfs_bst_node_t *node = NULL;
    assert(type == JFS_BST_LARGEST || type == JFS_BST_SMALLEST);
    if (type == JFS_BST_SMALLEST) {
        node = tree->smallest;
    } else {
        node = tree->largest;
    }

    assert(node != tree->nil);

    void      *container = NULL;
    const bool do_delete = tree->fns.detach(node, &container);
    tree->fns.unpack(container, value_out);
    if (do_delete) {
        bst_delete(tree, node);
    }
}

static void bst_find(const jfs_bst_t *tree, const void *key, bst_location_t *location_init) {
    assert(tree != NULL);
    assert(key != NULL);

    jfs_bst_node_t *node = tree->root;
    jfs_bst_node_t *parent = tree->nil;

    while (node != tree->nil) {
        const int cmp_result = tree->fns.cmp(key, node);
        if (cmp_result == -1) {
            parent = node;
            node = node->left;
        } else if (cmp_result == 1) {
            parent = node;
            node = node->right;
        } else {
            break;
        }
    }

    if (parent != tree->nil) {
        location_init->parent_cmp = tree->fns.cmp(key, parent);
    } else {
        location_init->parent_cmp = 0;
    }

    location_init->node = node;
    location_init->parent = parent;
}

static void bst_insert(jfs_bst_t *tree, jfs_bst_node_t *node, const bst_location_t *location) {
    assert(location->node == tree->nil); // we don't handle dupes here

    node->left = tree->nil;
    node->right = tree->nil;
    bst_set_parent_color(node, location->parent, BST_RED);

    if (location->parent == tree->nil) {
        assert(tree->root == tree->nil);
        tree->root = node;
    } else {
        assert(location->parent_cmp != 0);
        if (location->parent_cmp == -1) {
            location->parent->left = node;
        } else {
            location->parent->right = node;
        }
    }

    bst_fixup_cached_insert(tree, location);
    bst_fixup_insert(tree, node);
}

static void bst_delete(jfs_bst_t *tree, jfs_bst_node_t *node) {
    assert(node != tree->nil); // must have something to delete

    bst_fixup_cached_delete(tree, node);

    jfs_bst_node_t *replacement = tree->nil;
    uint64_t        deleted_color = bst_color(node);

    if (node->left == tree->nil) {
        replacement = node->right;
        bst_transplant(tree, node, node->right);
    } else if (node->right == tree->nil) {
        replacement = node->left;
        bst_transplant(tree, node, node->left);
    } else {
        jfs_bst_node_t *next_largest = bst_local_minimum(tree, node->right);
        deleted_color = bst_color(next_largest);
        replacement = next_largest->right;

        if (next_largest != node->right) {
            bst_transplant(tree, next_largest, next_largest->right);
            next_largest->right = node->right;
            bst_set_parent(next_largest->right, next_largest);
        } else {
            bst_set_parent(replacement, next_largest);
        }

        bst_transplant(tree, node, next_largest);
        next_largest->left = node->left;
        bst_set_parent(next_largest->left, next_largest);
        bst_set_color(next_largest, bst_color(node));
    }

    if (deleted_color == BST_BLACK) {
        bst_fixup_delete(tree, replacement);
    }
}

static void bst_fixup_cached_insert(jfs_bst_t *tree, const bst_location_t *location) {
    // we don't use location->node because this is assumed to be nil
    if (location->parent_cmp == -1 && location->parent == tree->smallest) {
        tree->smallest = location->parent->left;
    } else if (location->parent_cmp == 1 && location->parent == tree->largest) {
        tree->largest = location->parent->right;
    }
}

static void bst_fixup_cached_delete(jfs_bst_t *tree, const jfs_bst_node_t *node) {
    if (tree->smallest == tree->largest) { // only one node in tree
        assert(node == tree->root);
        tree->smallest = tree->nil;
        tree->largest = tree->nil;
    } else if (node == tree->smallest) {
        assert(tree->smallest->left == tree->nil);

        if (tree->smallest->right != tree->nil) {
            tree->smallest = bst_local_minimum(tree, tree->smallest->right);
        } else {
            tree->smallest = bst_parent(tree->smallest);
        }
    } else if (node == tree->largest) {
        assert(tree->largest->right == tree->nil);

        if (tree->largest->left != tree->nil) {
            tree->largest = bst_local_maximum(tree, tree->largest->left);
        } else {
            tree->largest = bst_parent(tree->largest);
        }
    }
}

static void bst_fixup_insert(jfs_bst_t *tree, jfs_bst_node_t *node) {
    assert(node != tree->nil);
    assert(tree->root != tree->nil);    // after an insert the tree should never be empty
    assert(bst_color(node) == BST_RED); // all new nodes should be red

    while (bst_color(bst_parent(node)) == BST_RED) {
        jfs_bst_node_t       *parent = bst_parent(node);
        jfs_bst_node_t *const grandparent = bst_parent(parent);
        assert(grandparent != tree->nil);

        if (parent == grandparent->left) {
            jfs_bst_node_t *const uncle = grandparent->right;

            if (bst_color(uncle) == BST_RED) {
                bst_set_color(parent, BST_BLACK);
                bst_set_color(uncle, BST_BLACK);
                bst_set_color(grandparent, BST_RED);
                node = grandparent;
                // loop continues
            } else {
                if (node == parent->right) {
                    node = parent;
                    bst_rotate_left(tree, parent);
                    parent = bst_parent(node);
                }

                bst_set_color(parent, BST_BLACK);
                bst_set_color(grandparent, BST_RED);
                bst_rotate_right(tree, grandparent);
                break; // tree is fixed
            }
        } else {
            jfs_bst_node_t *const uncle = grandparent->left;

            if (bst_color(uncle) == BST_RED) {
                bst_set_color(parent, BST_BLACK);
                bst_set_color(uncle, BST_BLACK);
                bst_set_color(grandparent, BST_RED);
                node = grandparent;
                // loop continues
            } else {
                if (node == parent->left) {
                    node = parent;
                    bst_rotate_right(tree, parent);
                    parent = bst_parent(node);
                }

                bst_set_color(parent, BST_BLACK);
                bst_set_color(grandparent, BST_RED);
                bst_rotate_left(tree, grandparent);
                break; // tree is fixed
            }
        }
    }

    bst_set_color(tree->root, BST_BLACK);
}

static void bst_fixup_delete(jfs_bst_t *tree, jfs_bst_node_t *node) {
    while (node != tree->root && bst_color(node) == BST_BLACK) {
        jfs_bst_node_t *const parent = bst_parent(node);
        if (node == parent->left) {
            jfs_bst_node_t *sibling = parent->right;
            if (bst_color(sibling) == BST_RED) {
                bst_set_color(sibling, BST_BLACK);
                bst_set_color(parent, BST_RED);
                bst_rotate_left(tree, parent);
                sibling = parent->right;
            }

            if (bst_color(sibling->left) == BST_BLACK && bst_color(sibling->right) == BST_BLACK) {
                bst_set_color(sibling, BST_RED);
                node = parent;
            } else {
                if (bst_color(sibling->right) == BST_BLACK) {
                    bst_set_color(sibling->left, BST_BLACK);
                    bst_set_color(sibling, BST_RED);
                    bst_rotate_right(tree, sibling);
                    sibling = parent->right;
                }

                bst_set_color(sibling, bst_color(parent));
                bst_set_color(parent, BST_BLACK);
                bst_set_color(sibling->right, BST_BLACK);
                bst_rotate_left(tree, parent);
                node = tree->root;
            }
        } else {
            jfs_bst_node_t *sibling = parent->left;
            if (bst_color(sibling) == BST_RED) {
                bst_set_color(sibling, BST_BLACK);
                bst_set_color(parent, BST_RED);
                bst_rotate_right(tree, parent);
                sibling = parent->left;
            }

            if (bst_color(sibling->left) == BST_BLACK && bst_color(sibling->right) == BST_BLACK) {
                bst_set_color(sibling, BST_RED);
                node = parent;
            } else {
                if (bst_color(sibling->left) == BST_BLACK) {
                    bst_set_color(sibling->right, BST_BLACK);
                    bst_set_color(sibling, BST_RED);
                    bst_rotate_left(tree, sibling);
                    sibling = parent->left;
                }

                bst_set_color(sibling, bst_color(parent));
                bst_set_color(parent, BST_BLACK);
                bst_set_color(sibling->left, BST_BLACK);
                bst_rotate_right(tree, parent);
                node = tree->root;
            }
        }
    }

    bst_set_color(node, BST_BLACK);
}

static void bst_rotate_left(jfs_bst_t *tree, jfs_bst_node_t *x) {
    assert(x != tree->nil);
    assert(x->right != tree->nil);

    jfs_bst_node_t *const y = x->right;
    jfs_bst_node_t *const x_parent = bst_parent(x);

    x->right = y->left;
    if (y->left != tree->nil) bst_set_parent(y->left, x);

    bst_set_parent(y, x_parent);
    if (x_parent == tree->nil) {
        tree->root = y;
    } else if (x == x_parent->left) {
        x_parent->left = y;
    } else {
        x_parent->right = y;
    }

    y->left = x;
    bst_set_parent(x, y);
}

static void bst_rotate_right(jfs_bst_t *tree, jfs_bst_node_t *x) {
    assert(x != tree->nil);
    assert(x->left != tree->nil);

    jfs_bst_node_t *const y = x->left;
    jfs_bst_node_t *const x_parent = bst_parent(x);

    x->left = y->right;
    if (y->right != tree->nil) bst_set_parent(y->right, x);

    bst_set_parent(y, x_parent);
    if (x_parent == tree->nil) {
        tree->root = y;
    } else if (x == x_parent->left) {
        x_parent->left = y;
    } else {
        x_parent->right = y;
    }

    y->right = x;
    bst_set_parent(x, y);
}

static void bst_transplant(jfs_bst_t *tree, jfs_bst_node_t *old, jfs_bst_node_t *new) { // NOLINT
    assert(tree->root != tree->nil);
    assert(old != new);

    jfs_bst_node_t *const old_parent = bst_parent(old);

    if (old_parent == tree->nil) {
        tree->root = new;
    } else if (old_parent->left == old) {
        old_parent->left = new;
    } else {
        old_parent->right = new;
    }

    bst_set_parent(new, old_parent);
}

static jfs_bst_node_t *bst_local_minimum(const jfs_bst_t *tree, jfs_bst_node_t *start) {
    while (start->left != tree->nil) {
        start = start->left;
    }
    return start;
}

static jfs_bst_node_t *bst_local_maximum(const jfs_bst_t *tree, jfs_bst_node_t *start) {
    while (start->right != tree->nil) {
        start = start->right;
    }
    return start;
}

static jfs_bst_node_t *bst_parent(const jfs_bst_node_t *node) {
    assert(node != NULL);
    return (jfs_bst_node_t *) (node->parent_color & ~((uintptr_t) 1)); // NOLINT
}

static void bst_set_parent(jfs_bst_node_t *node, jfs_bst_node_t *parent) {
    assert(node != NULL);
    assert(parent != NULL);
    node->parent_color = (uintptr_t) parent | (node->parent_color & 1);
}

static uint64_t bst_color(const jfs_bst_node_t *node) {
    assert(node != NULL);
    return (uint64_t) (node->parent_color & 1);
}

static void bst_set_color(jfs_bst_node_t *node, uint64_t color) {
    assert(node != NULL);
    assert(color == BST_RED || color == BST_BLACK);
    node->parent_color = (node->parent_color & ~((uintptr_t) 1)) | color;
}

static void bst_set_parent_color(jfs_bst_node_t *node, jfs_bst_node_t *parent, uint64_t color) {
    assert(node != NULL);
    assert(parent != NULL);
    assert(color == BST_RED || color == BST_BLACK);
    node->parent_color = (uintptr_t) parent | color;
}
