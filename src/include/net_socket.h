#ifndef JFS_NET_SOCKET_H
#define JFS_NET_SOCKET_H

#include "error.h"
#include <arpa/inet.h>

typedef struct jfs_ns_socket jfs_ns_socket_t;

struct jfs_ns_socket {
    int             fd;
    struct sockaddr addr;
};

void    jfs_ns_socket_transfer(jfs_ns_socket_t *sock_init, jfs_ns_socket_t *sock_free);
JFS_ERR jfs_ns_socket_register(jfs_ns_socket_t *sock);
JFS_ERR jfs_ns_socket_shutdown(const jfs_ns_socket_t *sock);
JFS_ERR jfs_ns_socket_close(jfs_ns_socket_t *sock);
JFS_ERR jfs_ns_socket_set_ip(jfs_ns_socket_t *sock_s, int server_port, const char *server_ip);
JFS_ERR jfs_ns_socket_set_hostname(jfs_ns_socket_t *sock_c, int server_port, const char *hostname);
JFS_ERR jfs_ns_socket_bind(const jfs_ns_socket_t *sock_s);
JFS_ERR jfs_ns_socket_listen(const jfs_ns_socket_t *sock_s);
JFS_ERR jfs_ns_socket_accept(const jfs_ns_socket_t *sock_s, jfs_ns_socket_t *sock_c);
JFS_ERR jfs_ns_socket_connect(const jfs_ns_socket_t *sock_c);
JFS_ERR jfs_ns_socket_recv(const jfs_ns_socket_t *sock, void *buf, size_t buf_size, size_t *bytes_received);
JFS_ERR jfs_ns_socket_send(const jfs_ns_socket_t *sock, const void *buf, size_t buf_size, int flags, size_t *bytes_sent);

#endif
