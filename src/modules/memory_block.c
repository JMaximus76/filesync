#include "memory_block.h"
#include <assert.h>

void jfs_mb_init(jfs_mb_t *mb_init, const jfs_mb_conf_t *conf, jfs_err_t *err) {
    VOID_FAIL_IF(conf->memory == NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(conf->capacity == 0, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(conf->obj_size == 0, JFS_ERR_BAD_CONF);

    mb_init->base_ptr = conf->memory;
    mb_init->obj_size = conf->obj_size;
    mb_init->capacity = conf->capacity;
}

void *jfs_mb_index(jfs_mb_t *mb, size_t index) {
    return mb->base_ptr + (index * mb->obj_size);
}
