#include "error.h"
#include <errno.h>


const char* jfs_err_to_string(jfs_error_t err) {
    switch (err) {
        #define X(name) case name: return #name;
            JFS_ERROR_LIST;
        #undef X
        default: return "failed to map jfs_error_t to string";
    }
}

jfs_error_t jfs_err_lstat(int c_error) {
    jfs_error_t err;

    switch (c_error) {
        case ENOMEM:
            err = JFS_ERR_RESOURCE;
            break;
        case EACCES:
            err = JFS_ERR_ACCESS;
            break;
        case ENAMETOOLONG:
            err = JFS_ERR_PATH_LEN;
            break;
        case ENOENT:
        case ENOTDIR:
            err = JFS_ERR_INVAL_PATH;
            break;
        default:
            err = JFS_ERR_MAP;
            break;
    }

    RETURN_ERR(err);
}

jfs_error_t jfs_err_opendir(int c_error) {
    jfs_error_t err;

    switch (c_error) {
        case EMFILE:
        case ENFILE:
        case ENOMEM:
            err = JFS_ERR_RESOURCE;
            break;
        case ENOENT:
        case ENOTDIR:
            err = JFS_ERR_INVAL_PATH;
            break;
        case EACCES:
            err = JFS_ERR_ACCESS;
            break;
        default:
            err = JFS_ERR_MAP;
            break;
    }

    RETURN_ERR(err);
}

