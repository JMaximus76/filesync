#ifndef JFS_MEMORY_BLOCK_H
#define JFS_MEMORY_BLOCK_H

#include "error.h"
#include <stddef.h>
#include <stdint.h>

typedef struct jfs_mb_conf jfs_mb_conf_t;
typedef struct jfs_mb      jfs_mb_t;

struct jfs_mb_conf {
    void  *memory;
    size_t obj_size;
    size_t capacity;
};

struct jfs_mb {
    uint8_t *base_ptr;
    size_t   obj_size;
    size_t   capacity;
};

void  jfs_mb_init(jfs_mb_t *mb_init, const jfs_mb_conf_t *conf, jfs_err_t *err);
void *jfs_mb_index(jfs_mb_t *mb, size_t index) WUR;


#endif
