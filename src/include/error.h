#ifndef JFS_ERROR_H
#define JFS_ERROR_H


#define RETURN_ERR(err_var) \
    do {                    \
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


typedef enum {
    JFS_OK = 0,
    JFS_ERR_MAP,
    JFS_ERR_RESOURCE,
    JFS_ERR_ARG,
    JFS_ERR_FATAL,
    JFS_ERR_EMPTY_STACK,
    JFS_ERR_NOT_DIR,
    JFS_ERR_ACCESS,
    JFS_ERR_PATH_LEN,
    JFS_ERR_NAME_LEN,
    JFS_ERR_INVAL_PATH,
    JFS_ERR_INVAL_NAME,
    JFS_ERR_PATH_OVERFLOW,
    JFS_ERR_BAD_FD,

    JFS_FW_ERR_SKIP,
    JFS_FW_ERR_STATE,
} jfs_error_t;



JFS_ERR jfs_err_lstat(int err);
JFS_ERR jfs_err_opendir(int err);

#endif

