#include "error.h"
#include <errno.h>

jfs_error_t jfs_err_lstat(int err) {
    switch (err) {
        case ENOMEM:
            return JFS_ERR_RESOURCE;
        case EACCES:
            return JFS_ERR_ACCESS;
        case ENAMETOOLONG:
            return JFS_ERR_PATH_LEN;
        case ENOENT:
        case ENOTDIR:
            return JFS_ERR_INVAL_PATH;
        default:
            return JFS_ERR_MAP;
    }
}

jfs_error_t jfs_err_opendir(int err) {
    switch (err) {
        case EMFILE:
        case ENFILE:
        case ENOMEM:
            return JFS_ERR_RESOURCE;
        case ENOENT:
        case ENOTDIR:
            return JFS_ERR_INVAL_PATH;
        case EACCES:
            return JFS_ERR_ACCESS;
        default:
            return JFS_ERR_MAP;
    }
}

