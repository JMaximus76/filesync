#ifndef JFS_FILE_IO_H
#define JFS_FILE_IO_H

#include "error.h"
#include <limits.h>
#include <sys/types.h>

typedef struct jfs_fio_path_buf jfs_fio_path_buf_t;
typedef struct jfs_fio_path     jfs_fio_path_t;
typedef struct jfs_fio_name     jfs_fio_name_t;

struct jfs_fio_path_buf {
    size_t len;
    char   data[PATH_MAX + 1];
};

struct jfs_fio_path {
    size_t len;
    char  *str;
};

struct jfs_fio_name {
    size_t len;
    char  *str;
};

JFS_ERR jfs_fio_path_init(jfs_fio_path_t *path_init, const char *path_str);
void    jfs_fio_path_free(jfs_fio_path_t *path_free);
void    jfs_fio_path_transfer(jfs_fio_path_t *path_init, jfs_fio_path_t *path_free);

JFS_ERR jfs_fio_name_init(jfs_fio_name_t *name_init, const char *name_str);
void    jfs_fio_name_free(jfs_fio_name_t *name_free);
void    jfs_fio_name_transfer(jfs_fio_name_t *name_init, jfs_fio_name_t *name_free);

void    jfs_fio_path_buf_clear(jfs_fio_path_buf_t *buf);
void    jfs_fio_path_buf_copy(jfs_fio_path_buf_t *buf, const jfs_fio_path_t *path);
JFS_ERR jfs_fio_path_buf_compose(jfs_fio_path_buf_t *buf, const jfs_fio_path_t *path, const jfs_fio_name_t *name);

#endif
