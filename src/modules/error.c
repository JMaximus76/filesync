#include "error.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

const char *jfs_err_to_string(jfs_error_t err) {
    switch (err) {
#define X(name)              \
    case name: return #name;
        JFS_ERROR_LIST;
#undef X
        default: return "failed to map jfs_error_t to string";
    }
}

jfs_error_t jfs_lstat(const char *path_str, struct stat *stat_init) {
    if (lstat(path_str, stat_init) != 0) {
        jfs_error_t err;
        switch (errno) {
            case EACCES:  err = JFS_ERR_ACCESS; break;
            case ENOENT:
            case ENOTDIR: err = JFS_ERR_INVAL_PATH; break;
            default:      err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_opendir(DIR **dir, const char *path_str) {
    *dir = opendir(path_str);
    if (*dir == NULL) {
        jfs_error_t err;
        switch (errno) {
            case ENOENT:
            case ENOTDIR: err = JFS_ERR_INVAL_PATH; break;
            case EACCES:  err = JFS_ERR_ACCESS; break;
            default:      err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_shutdown(int sock_fd, int how) {
    if (shutdown(sock_fd, how) != 0) {
        jfs_error_t err;
        switch (errno) {
            default: err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_getaddrinfo(const char *name, const char *port_str, const struct addrinfo *hints, struct addrinfo **result) {
    int status = getaddrinfo(name, port_str, hints, result);
    if (status != 0) {
        jfs_error_t err;
        switch (status) {
            case EAI_AGAIN:  err = JFS_ERR_AGAIN; break;
            case EAI_FAIL:
            case EAI_NONAME: err = JFS_ERR_LAN_HOST_UNREACH; break;
            case EAI_SYSTEM: err = JFS_ERR_SYS; break;
            default:         err = JFS_ERR_GETADDRINFO; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_bind(int sock_fd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(sock_fd, addr, addrlen) != 0) {
        jfs_error_t err;
        switch (errno) {
            default: err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_listen(int sock_fd, int backlog) {
    if (listen(sock_fd, backlog) != 0) {
        jfs_error_t err;
        switch (errno) {
            default: err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_accept(int sock_fd, struct sockaddr *addr, socklen_t *addrlen, int *accepted_fd) {
    *accepted_fd = accept(sock_fd, addr, addrlen);
    if (*accepted_fd == -1) {
        jfs_error_t err;
        switch (errno) {
            case ECONNABORTED: err = JFS_ERR_CONNECTION_ABORT; break;
            case EPROTO:
            case ENOPROTOOPT:
            case EHOSTDOWN:
            case ENONET:
            case EHOSTUNREACH:
            case EOPNOTSUPP:
            case ENETUNREACH:
            case EAGAIN:       err = JFS_ERR_AGAIN; break;
            case EINTR:        err = JFS_ERR_INTER; break;
            default:           err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_connect(int sock_fd, const struct sockaddr *addr, socklen_t addrlen) {
    if (connect(sock_fd, addr, addrlen) != 0) {
        jfs_error_t err;
        switch (errno) {
            case EAGAIN:       err = JFS_ERR_AGAIN; break;
            case ECONNREFUSED:
            case ETIMEDOUT:
            case ENETUNREACH:  err = JFS_ERR_LAN_HOST_UNREACH; break;
            case EINTR:        err = JFS_ERR_INTER; break;
            default:           err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    return JFS_OK;
}

jfs_error_t jfs_recv(int sock_fd, void *buf, size_t size, int flags, size_t *bytes_received) {
    ssize_t status = recv(sock_fd, buf, size, flags);
    if (status == -1) {
        jfs_error_t err;
        switch (errno) {
            case EAGAIN: err = JFS_ERR_AGAIN; break;
            case EINTR:  err = JFS_ERR_INTER; break;
            default:     err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    *bytes_received = (size_t) status;
    return JFS_OK;
}

jfs_error_t jfs_send(int sock_fd, const void *buf, size_t size, int flags, size_t *bytes_sent) {
    ssize_t status = send(sock_fd, buf, size, flags);
    if (status == -1) {
        jfs_error_t err;
        switch (errno) {
            case EAGAIN:     err = JFS_ERR_AGAIN; break;
            case ECONNRESET: err = JFS_ERR_CONNECTION_RESET; break;
            case EINTR:      err = JFS_ERR_INTER; break;
            case EPIPE:      err = JFS_ERR_PIPE; break;
            default:         err = JFS_ERR_SYS; break;
        }
        RETURN_ERR(err);
    }
    *bytes_sent = (size_t) status;
    return JFS_OK;
}
