#ifndef JFS_ERROR_H
#define JFS_ERROR_H

#include <stdio.h>

#define TEMP_LOG(err_var) \
    printf("ERROR (%d): %s\n    %s\n", (err_var), __FILE__, __func__)


#define RETURN_ERR(err_var) \
    do {                    \
        TEMP_LOG(err_var);  \
        return (err_var);   \
    } while (0)


#define CHECK_ERR(err_expr)                    \
    do {                                       \
        jfs_error_t __ERR_RESULT = (err_expr); \
        if (__ERR_RESULT != JFS_OK) {          \
            RETURN_ERR(__ERR_RESULT);          \
        }                                      \
    } while (0)

#define FAIL_IF(cond_expr, err_var) \
    do {                            \
        if ((cond_expr)) {          \
            RETURN_ERR(err_var);    \
        }                           \
    } while (0)


#define GOTO_ERR(err_var, label_name) \
    do {                              \
        if ((err_var) != JFS_OK) {    \
            goto label_name;          \
        }                             \
    } while (0)


#define GOTO_ERR_SET(err_set, err_expr, label_name) \
    do {                                            \
        (err_set) = (err_expr);                     \
        if ((err_set) != JFS_OK) {                  \
            goto label_name;                        \
        }                                           \
    } while (0)


#define JFS_ERR jfs_error_t __attribute__((warn_unused_result))


#define JFS_ERROR_LIST \
    X(JFS_OK) \
    X(JFS_ERR_MAP) \
    X(JFS_ERR_RESOURCE) \
    X(JFS_ERR_ARG) \
    X(JFS_ERR_FATAL) \
    X(JFS_ERR_EMPTY_STACK) \
    X(JFS_ERR_NOT_DIR) \
    X(JFS_ERR_ACCESS) \
    X(JFS_ERR_PATH_LEN) \
    X(JFS_ERR_NAME_LEN) \
    X(JFS_ERR_INVAL_PATH) \
    X(JFS_ERR_INVAL_NAME) \
    X(JFS_ERR_BAD_FD) \
    X(JFS_FIO_ERR_PATH_OVERFLOW) \
    X(JFS_FW_ERR_SKIP) \
    X(JFS_FW_ERR_STATE) \

typedef enum {
    #define X(name) name,
        JFS_ERROR_LIST
    #undef X
} jfs_error_t;


JFS_ERR jfs_err_lstat(int err);
JFS_ERR jfs_err_opendir(int err);
const char* jfs_err_to_string(jfs_error_t err);

#endif

