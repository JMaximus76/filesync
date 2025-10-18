#ifndef JFS_FREE_LIST_H
#define JFS_FREE_LIST_H

#include "error.h"
#include <stdint.h>

typedef struct jfs_fl_conf jfs_fl_conf_t;
typedef struct jfs_fl      jfs_fl_t;
typedef struct jfs_fl_obj  jfs_fl_obj_t;

struct jfs_fl_conf {
    size_t padded_obj_size;
    size_t obj_count;
    void  *memory;
};

struct jfs_fl {
    jfs_fl_obj_t *list;
    size_t        count;
};

void  jfs_fl_init(jfs_fl_t *fl_init, const jfs_fl_conf_t *conf, jfs_err_t *err);
void *jfs_fl_alloc(jfs_fl_t *fl) WUR;
void  jfs_fl_free(jfs_fl_t *fl, void *ptr_move);

#endif
