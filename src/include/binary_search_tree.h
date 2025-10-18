#ifndef JFS_BINARY_SEARCH_TREE_H
#define JFS_BINARY_SEARCH_TREE_H

#include "free_list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define bst_container_of(ptr, type, member) ((type *) ((uint8_t *) (ptr) - offsetof(type, member)))

typedef struct jfs_bst      jfs_bst_t;
typedef struct jfs_bst_node jfs_bst_node_t;
typedef struct jfs_bst_fns  jfs_bst_fns_t;
typedef struct jfs_bst_conf jfs_bst_conf_t;

typedef int (*jfs_bst_cmp_fn)(const void *key, const jfs_bst_node_t *node);
typedef jfs_bst_node_t *(*jfs_bst_pack_fn)(void *container, const void *value);
typedef void (*jfs_bst_unpack_fn)(void *container, void *value_out);
typedef void (*jfs_bst_attach_fn)(jfs_bst_node_t *node, void *container);
typedef bool (*jfs_bst_detach_fn)(jfs_bst_node_t *node, void **container_out);

typedef enum {
    JFS_BST_SMALLEST,
    JFS_BST_LARGEST
} jfs_bst_cached_t;

struct jfs_bst_node {
    uintptr_t       parent_color;
    jfs_bst_node_t *right;
    jfs_bst_node_t *left;
};

struct jfs_bst_fns {
    jfs_bst_cmp_fn    cmp;
    jfs_bst_pack_fn   pack;
    jfs_bst_unpack_fn unpack;
    jfs_bst_attach_fn attach;
    jfs_bst_detach_fn detach;
};

struct jfs_bst_conf {
    jfs_fl_conf_t fl_conf;
    jfs_bst_fns_t fns;
};

struct jfs_bst {
    jfs_bst_node_t *root;
    jfs_bst_node_t *nil;
    jfs_bst_node_t  nil_storage;
    jfs_bst_node_t *smallest;
    jfs_bst_node_t *largest;
    jfs_bst_fns_t   fns;
    jfs_fl_t        free_list;
};

void jfs_bst_init(jfs_bst_t *tree_init, const jfs_bst_conf_t *conf, jfs_err_t *err);
void jfs_bst_puts(jfs_bst_t *tree, const void *value, const void *key, jfs_err_t *err);
void jfs_bst_gets(jfs_bst_t *tree, void *value_out, const void *key, jfs_err_t *err);
void jfs_bst_cached_gets(jfs_bst_t *tree, void *value_out, jfs_bst_cached_t type, jfs_err_t *err);

#endif
