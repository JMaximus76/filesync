#ifndef JFS_MEMORY_LAYOUT_GENERATOR_H
#define JFS_MEMORY_LAYOUT_GENERATOR_H

#include "error.h"
#include <stddef.h>

typedef struct jfs_mlg_info      jfs_mlg_info_t;
typedef struct jfs_mlg_component jfs_mlg_component_t;
typedef struct jfs_mlg_layout    jfs_mlg_layout_t;

struct jfs_mlg_info {
    const jfs_mlg_component_t *components;
    size_t                     component_count;
    size_t                     header_size;
    size_t                     header_align;
};

struct jfs_mlg_component {
    size_t obj_size;
    size_t obj_align;
    size_t obj_count;
};

struct jfs_mlg_layout {
    size_t total_bytes;
    size_t master_align;
};

void  jfs_mlg_generate(jfs_mlg_layout_t *layout, size_t offsets[], const jfs_mlg_info_t *info, jfs_err_t *err);
void *jfs_mlg_allocate_block(jfs_mlg_layout_t *layout, jfs_err_t *err) WUR;
void *jfs_mlg_component_pointer(void *base_ptr, size_t offsets[], size_t component_index) WUR;

#endif
