#include "file_io.h"
#include "error.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

jfs_error_t jfs_fio_path_init(jfs_fio_path_t *path_init, const char *path_str) {
    size_t path_str_len = strlen(path_str);

    if (path_str_len > 1 && path_str[path_str_len - 1] == '/') {
        path_str_len -= 1;
    }
    FAIL_IF(path_str_len > PATH_MAX, JFS_ERR_FIO_PATH_LEN);

    char *new_str = malloc(path_str_len + 1);
    FAIL_IF(new_str == NULL, JFS_ERR_SYS);
    strlcpy(new_str, path_str, path_str_len + 1);

    path_init->len = path_str_len;
    path_init->str = new_str;

    return JFS_OK;
}

void jfs_fio_path_free(jfs_fio_path_t *path_free) {
    free(path_free->str);
    memset(path_free, 0, sizeof(*path_free));
}

void jfs_fio_path_transfer(jfs_fio_path_t *path_init, jfs_fio_path_t *path_free) {
    *path_init = *path_free;
    memset(path_free, 0, sizeof(*path_free));
}

JFS_ERR jfs_fio_name_init(jfs_fio_name_t *name_init, const char *name_str) {
    size_t name_str_len = strlen(name_str);
    FAIL_IF(name_str_len > NAME_MAX, JFS_ERR_FIO_NAME_LEN);

    char *new_str = malloc(name_str_len + 1);
    FAIL_IF(new_str == NULL, JFS_ERR_SYS);
    strlcpy(new_str, name_str, name_str_len + 1);

    name_init->len = name_str_len;
    name_init->str = new_str;

    return JFS_OK;
}

void jfs_fio_name_free(jfs_fio_name_t *name_free) {
    free(name_free->str);
    memset(name_free, 0, sizeof(*name_free));
}

void jfs_fio_name_transfer(jfs_fio_name_t *name_init, jfs_fio_name_t *name_free) {
    *name_init = *name_free;
    memset(name_free, 0, sizeof(*name_free));
}

void jfs_fio_path_buf_clear(jfs_fio_path_buf_t *buf) {
    buf->len = 0;
    memset(buf->data, 0, sizeof(buf->data));
}

void jfs_fio_path_buf_copy(jfs_fio_path_buf_t *buf, const jfs_fio_path_t *path) {
    jfs_fio_path_buf_clear(buf);
    strlcpy(buf->data, path->str, sizeof(buf->data));
    buf->len = path->len;
}

JFS_ERR jfs_fio_path_buf_compose(jfs_fio_path_buf_t *buf, const jfs_fio_path_t *path, const jfs_fio_name_t *name) {
    const size_t new_len = path->len + name->len + 1;
    FAIL_IF( new_len > PATH_MAX, JFS_ERR_FIO_PATH_OVERFLOW);
    jfs_fio_path_buf_clear(buf);
    snprintf(buf->data, sizeof(buf->data), "%s/%s", path->str, name->str);
    buf->len = new_len;

    return JFS_OK;
}
