#include "file_io.h"

#include <string.h>
#include <stdio.h>
#include <limits.h>


jfs_error_t jfs_fio_cat_path(char *path_buf, const char *file_name) {
    size_t path_len = strlen(path_buf);
    size_t file_len = strlen(file_name);

    FAIL_IF(file_len == 0, JFS_ERR_INVAL_NAME);
    FAIL_IF(path_len == 0, JFS_ERR_INVAL_PATH);

    if (path_len > 1 && path_buf[path_len - 1] == '/') {
        path_buf[path_len - 1] = '\0';
        path_len -= 1;
    }

    FAIL_IF(file_len > NAME_MAX, JFS_ERR_NAME_LEN);
    FAIL_IF(path_len > PATH_MAX, JFS_ERR_PATH_LEN);
    FAIL_IF(path_len + file_len + 1 > PATH_MAX, JFS_ERR_PATH_OVERFLOW);

    snprintf(path_buf + path_len, PATH_MAX - path_len, "/%s", file_name);
    return JFS_OK;
}

