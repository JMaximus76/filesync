#include "error.h"
#include <errno.h>


static jfs_error_t err_map_lstat(int c_error);
static jfs_error_t err_map_opendir(int c_error);


jfs_error_t jfs_err_lstat(const char *path_str, struct stat *stat_init) {
    if (lstat(path_str, stat_init) != 0) {
        RETURN_ERR(err_map_lstat(errno));
    }

    return JFS_OK;
}

jfs_error_t jfs_err_opendir(DIR **dir, const char *path_str) {
    *dir = opendir(path_str);
    if (*dir == NULL) {
        RETURN_ERR(err_map_opendir(errno));
    }

    return JFS_OK;
}

const char *jfs_err_to_string(jfs_error_t err) {
    switch (err) {
#define X(name)              \
    case name: return #name;
        JFS_ERROR_LIST;
#undef X
        default: return "failed to map jfs_error_t to string";
    }
}

static jfs_error_t err_map_lstat(int c_error) {
    jfs_error_t err;

    switch (c_error) {
        case ENOMEM:       err = JFS_ERR_RESOURCE; break;
        case EACCES:       err = JFS_ERR_ACCESS; break;
        case ENAMETOOLONG: err = JFS_ERR_PATH_LEN; break;
        case ENOENT:
        case ENOTDIR:      err = JFS_ERR_INVAL_PATH; break;
        default:           err = JFS_ERR_MAP; break;
    }

    RETURN_ERR(err);
}

static jfs_error_t err_map_opendir(int c_error) {
    jfs_error_t err;

    switch (c_error) {
        case EMFILE:
        case ENFILE:
        case ENOMEM:  err = JFS_ERR_RESOURCE; break;
        case ENOENT:
        case ENOTDIR: err = JFS_ERR_INVAL_PATH; break;
        case EACCES:  err = JFS_ERR_ACCESS; break;
        default:      err = JFS_ERR_MAP; break;
    }

    return err;
}
