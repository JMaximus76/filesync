#ifndef JFS_ERROR_H
#define JFS_ERROR_H

#include <dirent.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>

// TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP
#include <stdio.h>
#define LOG_STR "\033[1;31m%s\033[0m: \033[35m%s\033[0m  errno: %s\n"
#define RES_STR "\033[1;36m%s\033[0m: \033[35m%s\033[0m\n"

#define LOG_ERR printf(LOG_STR, jfs_err_str(err), __func__, strerror(errno))
#define RES_ERR                                  \
    printf(RES_STR, jfs_err_str(err), __func__); \
    *err = JFS_OK
// TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP TEMP

#define RETURN_VOID_ERR \
    do {                \
        LOG_ERR;        \
        return;         \
    } while (0)

#define RETURN_NULL_ERR \
    do {                \
        LOG_ERR;        \
        return NULL;    \
    } while (0)

#define RETURN_VAL_ERR(val_var) \
    do {                        \
        LOG_ERR;                \
        return (val_var);       \
    } while (0)

#define CHECK_VOID_ERR        \
    do {                      \
        if (*err != JFS_OK) { \
            RETURN_VOID_ERR;  \
        }                     \
    } while (0)

#define CHECK_NULL_ERR        \
    do {                      \
        if (*err != JFS_OK) { \
            RETURN_NULL_ERR;  \
        }                     \
    } while (0)

#define CHECK_VAL_ERR(val_var)       \
    do {                             \
        if (*err != JFS_OK) {        \
            RETURN_VAL_ERR(val_var); \
        }                            \
    } while (0)

#define VOID_FAIL_IF(cond_expr, err_var) \
    do {                                 \
        if ((cond_expr)) {               \
            *err = err_var;              \
            RETURN_VOID_ERR;             \
        }                                \
    } while (0)

#define NULL_FAIL_IF(cond_expr, err_var) \
    do {                                 \
        if ((cond_expr)) {               \
            *err = err_var;              \
            RETURN_NULL_ERR;             \
        }                                \
    } while (0)

#define VAL_FAIL_IF(cond_expr, err_var, val_var) \
    do {                                         \
        if ((cond_expr)) {                       \
            *err = err_var;                      \
            RETURN_VAL_ERR(val_var);             \
        }                                        \
    } while (0)

#define GOTO_IF_ERR(label_name) \
    do {                        \
        if (*err != JFS_OK) {   \
            goto label_name;    \
        }                       \
    } while (0)

#define REMAP_ERR(from_var, to_var) \
    do {                            \
        if (*err == (from_var)) {   \
            *err = (to_var);        \
            printf("remap err\n");  \
        }                           \
    } while (0)

#define WUR __attribute__((warn_unused_result))

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
    X(JFS_ERR_FW_SKIP)           \
    X(JFS_ERR_FW_FAIL)           \
    X(JFS_ERR_FW_UNSUPPORTED)    \
    X(JFS_ERR_FW_UNKNOWN)

typedef enum {
#define X(name) name,
    JFS_ERROR_LIST
#undef X
} jfs_err_t;

const char *jfs_err_str(const jfs_err_t *err);

void            *jfs_malloc(size_t size, jfs_err_t *err) WUR;
void            *jfs_realloc(void *ptr, size_t size, jfs_err_t *err) WUR;
void             jfs_lstat(const char *path, struct stat *stat_init, jfs_err_t *err);
DIR             *jfs_opendir(const char *path, jfs_err_t *err) WUR;
void             jfs_shutdown(int sock_fd, int how, jfs_err_t *err);
struct addrinfo *jfs_getaddrinfo(const char *name, const char *port_str, const struct addrinfo *hints, jfs_err_t *err) WUR;
void             jfs_bind(int sock_fd, const struct sockaddr *addr, socklen_t addrlen, jfs_err_t *err);
void             jfs_listen(int sock_fd, int backlog, jfs_err_t *err);
int              jfs_accept(int sock_fd, struct sockaddr *addr, socklen_t *addrlen, jfs_err_t *err) WUR;
void             jfs_connect(int sock_fd, const struct sockaddr *addr, socklen_t addrlen, jfs_err_t *err);
size_t           jfs_recv(int sock_fd, void *buf, size_t size, int flags, jfs_err_t *err) WUR;
size_t           jfs_send(int sock_fd, const void *buf, size_t size, int flags, jfs_err_t *err) WUR;

#endif
