#ifndef JFS_ERROR_H
#define JFS_ERROR_H

#include <dirent.h>
#include <netdb.h>
#include <sys/stat.h>

// TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP
#include <stdio.h>
#define JFS_LOG(err_var) // printf("\033[1;31m%s\033[0m: \033[35m%s\033[0m at \033[32m%s\033[0m\n", jfs_err_to_string((err_var)), __func__, __FILE__)
#define JFS_RES(err_var) // printf("\033[1;36m%s\033[0m: \033[35m%s\033[0m at \033[32m%s\033[0m\n", jfs_err_to_string((err_var)), __func__, __FILE__)
// TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP

#define RETURN_ERR(err_var) \
    do {                    \
        JFS_LOG(err_var);   \
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

#define GOTO_IF_ERR(err_var, label_name) \
    do {                                 \
        if ((err_var) != JFS_OK) {       \
            goto label_name;             \
        }                                \
    } while (0)

#define JFS_ERR jfs_error_t __attribute__((warn_unused_result))

#define JFS_ERROR_LIST           \
    X(JFS_OK)                    \
    X(JFS_ERR_SYS)               \
    X(JFS_ERR_INTER)             \
    X(JFS_ERR_AGAIN)             \
    X(JFS_ERR_ACCESS)            \
    X(JFS_ERR_INVAL_PATH)        \
    X(JFS_ERR_ARG)               \
    X(JFS_ERR_EMPTY)             \
    X(JFS_ERR_GETADDRINFO)       \
    X(JFS_ERR_LAN_HOST_UNREACH)  \
    X(JFS_ERR_CONNECTION_ABORT)  \
    X(JFS_ERR_CONNECTION_RESET)  \
    X(JFS_ERR_PIPE)              \
    X(JFS_ERR_FIO_PATH_LEN)      \
    X(JFS_ERR_FIO_NAME_LEN)      \
    X(JFS_ERR_FIO_PATH_OVERFLOW) \
    X(JFS_ERR_FW_STATE)          \
    X(JFS_ERR_FW_UNSUPPORTED)    \
    X(JFS_ERR_FW_UNKNOWN)

typedef enum {
#define X(name) name,
    JFS_ERROR_LIST
#undef X
} jfs_error_t;

const char *jfs_err_to_string(jfs_error_t err);

JFS_ERR jfs_lstat(const char *path, struct stat *stat_init);
JFS_ERR jfs_opendir(DIR **dir, const char *path);
JFS_ERR jfs_shutdown(int sock_fd, int how);
JFS_ERR jfs_getaddrinfo(const char *name, const char *port_str, const struct addrinfo *hints, struct addrinfo **result);
JFS_ERR jfs_bind(int sock_fd, const struct sockaddr *addr, socklen_t addrlen);
JFS_ERR jfs_listen(int sock_fd, int backlog);
JFS_ERR jfs_accept(int sock_fd, struct sockaddr *addr, socklen_t *addrlen, int *accepted_fd);
JFS_ERR jfs_connect(int sock_fd, const struct sockaddr *addr, socklen_t addrlen);
JFS_ERR jfs_recv(int sock_fd, void *buf, size_t size, int flags, size_t *bytes_received);
JFS_ERR jfs_send(int sock_fd, const void *buf, size_t size, int flags, size_t *bytes_sent);

#endif
