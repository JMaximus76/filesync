#include "file_io.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

void jfs_fio_normalize_path(char *path_buf) {
    size_t len = strlen(path_buf);
    if (len > 1 && path_buf[len - 1] == '/') {
        path_buf[len - 1] = '\0';
    }
}

jfs_error_t jfs_fio_cat_path(char *path_buf, const char *file_name) {
    size_t file_len = strlen(file_name);
    FAIL_IF(file_len > NAME_MAX, JFS_ERR_NAME_LEN);

    jfs_fio_normalize_path(path_buf);
    size_t path_len = strlen(path_buf);
    FAIL_IF(path_len + file_len + 1 > PATH_MAX, JFS_FIO_ERR_PATH_OVERFLOW);

    snprintf(path_buf + path_len, PATH_BUF - path_len, "/%s", file_name);
    return JFS_OK;
}

jfs_error_t jfs_fio_change_path(char *path_buf, size_t path_len, const char *file_name) {
    FAIL_IF(path_len == 0, JFS_ERR_ARG);

    if (path_buf[path_len] == '\0') {
        if (path_buf[path_len - 1] == '/') {
            path_len -= 1;
        }
    } else if (path_buf[path_len] != '/') {
        RETURN_ERR(JFS_ERR_ARG);
    }

    size_t file_len = strlen(file_name);
    FAIL_IF(file_len > NAME_MAX, JFS_ERR_NAME_LEN);

    FAIL_IF(path_len + file_len + 1 > PATH_MAX, JFS_FIO_ERR_PATH_OVERFLOW);

    snprintf(&path_buf[path_len], PATH_BUF - path_len, "/%s", file_name);
    return JFS_OK;
}
