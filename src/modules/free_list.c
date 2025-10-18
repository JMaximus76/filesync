#include "free_list.h"
#include "error.h"
#include <assert.h>
#include <stdlib.h>

#define MIN_OBJ_SIZE sizeof(jfs_fl_obj_t)

static jfs_fl_obj_t *fl_link_memory(uint8_t *memory, size_t padded_obj_size, size_t obj_count);

struct jfs_fl_obj {
    jfs_fl_obj_t *next;
};

void jfs_fl_init(jfs_fl_t *fl_init, const jfs_fl_conf_t *conf, jfs_err_t *err) {
    VOID_FAIL_IF(conf->memory == NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(conf->padded_obj_size < MIN_OBJ_SIZE, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(conf->obj_count == 0, JFS_ERR_BAD_CONF);

    jfs_fl_obj_t *const free_list = fl_link_memory(conf->memory, conf->padded_obj_size, conf->obj_count);

    fl_init->list = free_list;
    fl_init->count = conf->obj_count;
}

void *jfs_fl_alloc(jfs_fl_t *fl) {
    if (fl->count == 0) return NULL;
    jfs_fl_obj_t *obj = fl->list;
    fl->list = obj->next;
    fl->count -= 1;
    return obj;
}

void jfs_fl_free(jfs_fl_t *fl, void *ptr_move) {
    if (ptr_move == NULL) return;
    jfs_fl_obj_t *obj = ptr_move;
    obj->next = fl->list;
    fl->list = obj;
    fl->count += 1;
}

static jfs_fl_obj_t *fl_link_memory(uint8_t *memory, size_t padded_obj_size, size_t obj_count) { // NOLINT
    jfs_fl_obj_t *list = NULL;

    for (size_t i = 0; i < obj_count; i++) {
        jfs_fl_obj_t *obj = (jfs_fl_obj_t *) (memory + (i * padded_obj_size));
        obj->next = list;
        list = obj;
    }

    return list;
}
