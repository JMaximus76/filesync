#ifndef JFS_FILE_IO_H
#define JFS_FILE_IO_H

#include "error.h"
#include <limits.h>
#include <sys/types.h>

#define PATH_BUF (PATH_MAX + 1)
#define NAME_BUF (NAME_MAX + 1)

void    jfs_fio_normalize_path(char *path_buf);
JFS_ERR jfs_fio_cat_path(char *path, const char *name);
JFS_ERR jfs_fio_change_path(char *path_buf, size_t path_len, const char *file_name);
JFS_ERR jfs_fio_is_dir(char *path);
JFS_ERR jfs_fio_is_file(char *path);

#endif
