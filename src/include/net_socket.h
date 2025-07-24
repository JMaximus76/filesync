#ifndef JFS_NET_SOCKET_H
#define JFS_NET_SOCKET_H

#include "error.h"
#include <arpa/inet.h>

typedef struct jfs_ns_socket jfs_ns_socket_t;

struct jfs_ns_socket;

jfs_ns_socket_t *jfs_ns_socket_create(jfs_err_t *err) WUR;
void             jfs_ns_socket_open(jfs_ns_socket_t *sock, jfs_err_t *err);
void             jfs_ns_socket_close(jfs_ns_socket_t *sock, jfs_err_t *err);
void             jfs_ns_socket_destroy(jfs_ns_socket_t **sock_give);

void             jfs_ns_socket_shutdown(const jfs_ns_socket_t *sock, jfs_err_t *err);
void             jfs_ns_socket_set_ip(jfs_ns_socket_t *sock, uint16_t server_port, const char *server_ip, jfs_err_t *err);
void             jfs_ns_socket_set_hostname(jfs_ns_socket_t *sock, uint16_t server_port, const char *hostname, jfs_err_t *err);
void             jfs_ns_socket_bind(const jfs_ns_socket_t *sock, jfs_err_t *err);
void             jfs_ns_socket_listen(const jfs_ns_socket_t *sock, jfs_err_t *err);
jfs_ns_socket_t *jfs_ns_socket_accept(const jfs_ns_socket_t *sock, jfs_err_t *err) WUR;
void             jfs_ns_socket_connect(const jfs_ns_socket_t *sock, jfs_err_t *err);
size_t           jfs_ns_socket_recv(const jfs_ns_socket_t *sock, void *buf, size_t buf_size, jfs_err_t *err);
size_t           jfs_ns_socket_send(const jfs_ns_socket_t *sock, const void *buf, size_t buf_size, int flags, jfs_err_t *err);

#endif
