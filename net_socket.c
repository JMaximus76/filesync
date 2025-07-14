#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>

#include "net_socket.h"
#include "file_tools.h"

static const int MAX_LISTEN = 5;
static const size_t FILE_READ_BUFFER_SIZE = 65536;
static const size_t FILE_SEND_BUFFER_SIZE = 65536;


struct net_socket_address_info {
    struct sockaddr *addr;
    size_t len;
};

struct net_socket {
    int fd;
    struct net_socket_address_info *addrinfo;
};


static int net_socket_set_address(struct net_socket *nsock, const struct sockaddr *addr, size_t len);
static int init_socket(struct net_socket* nsock);
static int map_gai_error_to_errno(int gai_err);

struct net_socket* net_socket_create() {
    struct net_socket* nsock = malloc(sizeof(*nsock));
    if (nsock == NULL) {
        return NULL;
    }

    if (init_socket(nsock) == -1) {
        free(nsock);
        return NULL;
    }

    nsock->addrinfo = NULL;
    return nsock;
}

int net_socket_destroy(struct net_socket **nsock) {
    if (!(nsock && *nsock)) {
        errno = EINVAL;
        return -1;
    }

    const int close_status = close((*nsock)->fd);
    if ((*nsock)->addrinfo != NULL) {
        free((*nsock)->addrinfo->addr);
        free((*nsock)->addrinfo);
    }
    free(*nsock);
    *nsock = NULL;
    return close_status;
}

int net_socket_shutdown(const struct net_socket *nsock) {
    return shutdown(nsock->fd, SHUT_WR);
}

int net_socket_set_ip(struct net_socket *nsock, int port, const char *ip) {
    if (port <= 1024 || port >= 49125) {
        errno = EINVAL;
        return -1;
    }


    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    return net_socket_set_address(nsock, (struct sockaddr*) &addr, sizeof(addr));
}


int net_socket_set_host(struct net_socket *nsock, int port, const char *hostname) {
    if (port <= 1024 || port >= 49125) {
        errno = EINVAL;
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[NI_MAXSERV];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int status = getaddrinfo(hostname, port_str, &hints, &result);
    if (status != 0) {
        errno = map_gai_error_to_errno(status);
        return -1;
    }

    if (net_socket_set_address(nsock, result->ai_addr, result->ai_addrlen) == -1) {
        freeaddrinfo(result);
        return -1;
    }

    freeaddrinfo(result);
    return 0;
}


int net_socket_bind(const struct net_socket *nsock) {
    if (nsock->addrinfo == NULL) {
        errno = EINVAL;
        return -1;
    }

    return bind(nsock->fd, nsock->addrinfo->addr, nsock->addrinfo->len);
}


int net_socket_listen(const struct net_socket *nsock) {
    return listen(nsock->fd, MAX_LISTEN);
}

int net_socket_accept(const struct net_socket *serv, struct net_socket **client) {
    const int client_fd = accept(serv->fd, NULL, NULL);

    if (client_fd == -1) {
        return -1;
    }

    *client = malloc(sizeof(**client));
    if (client == NULL) {
        close(client_fd);
        return -1;
    }

    (*client)->fd = client_fd;
    (*client)->addrinfo = NULL;
    return 0;
}


int net_socket_connect(const struct net_socket *nsock) {
    return connect(nsock->fd, nsock->addrinfo->addr, nsock->addrinfo->len);
}


ssize_t net_socket_recv(const struct net_socket *nsock, void *buf, size_t size) {
    if (size > SSIZE_MAX || size == 0) {
        errno = EINVAL;
        return -1;
    }

    ssize_t r;
    do {
        r = recv(nsock->fd, buf, size, 0);
    } while (r == -1 && errno == EINTR);

    return r;
}


ssize_t net_socket_send(const struct net_socket *nsock, const void *buf, size_t size, int flags) {
    if (size > SSIZE_MAX || size == 0) {
        errno = EINVAL;
        return -1;
    }

    const uint8_t *send_buf = (uint8_t*) buf;

    size_t total_sent = 0;
    while (total_sent < size) {
        const ssize_t s = send(nsock->fd, send_buf + total_sent, size - total_sent, MSG_NOSIGNAL | flags);

        if (s == -1)  {
            if (errno == EINTR) continue;
            return (total_sent == 0) ? -1 : (ssize_t) total_sent;
        }

        total_sent += (size_t) s;
    }

    return (ssize_t) total_sent;
}


int net_socket_file_recv(const struct net_socket *nsock, int file_fd, size_t size, size_t *total_recv, size_t *total_written) {
    uint8_t buf[FILE_READ_BUFFER_SIZE];
    *total_recv = 0;
    *total_written = 0;

    while (*total_recv < size) {
        const size_t size_remaining = size - *total_recv;
        const size_t size_to_read = (size_remaining < FILE_READ_BUFFER_SIZE) ? size_remaining : FILE_READ_BUFFER_SIZE;

        const ssize_t r = net_socket_recv(nsock, buf, size_to_read); 
        if (r == -1) return -1;
        if (r == 0) return -3;

        const size_t total_read = (size_t) r;
        *total_recv += total_read;

        ssize_t w = ft_write(file_fd, buf, total_read);
        if (w == -1) return -2;
        *total_written += (size_t) w;
        if ((size_t) w < total_read) return -2;

    }

    return 0;
}


int net_socket_file_send(const struct net_socket *nsock, int file_fd, size_t size, size_t *total_sent) {
    uint8_t buf[FILE_SEND_BUFFER_SIZE];
    *total_sent = 0;

    while (*total_sent < size) {
        const size_t size_remaining = size - *total_sent;
        const size_t size_to_send = (size_remaining < FILE_SEND_BUFFER_SIZE) ? size_remaining : FILE_SEND_BUFFER_SIZE;

        ssize_t r = ft_read(file_fd, buf, size_to_send);
        if (r == -1) return -1;
        if (r == 0) return -3;

        const size_t total_read = (size_t) r;

        ssize_t s = net_socket_send(nsock, buf, total_read, 0);
        if (s == -1) return -2;
        *total_sent += s;
        if ((size_t) s < total_read) return -2;
    }

    return 0;
}


static int net_socket_set_address(struct net_socket *nsock, const struct sockaddr *addr, size_t len) {
    struct net_socket_address_info *temp_addrinfo = malloc(sizeof(*temp_addrinfo));
    struct sockaddr *temp_sockaddr = malloc(len);
    if (temp_addrinfo == NULL || temp_sockaddr == NULL) {
        free(temp_addrinfo);
        free(temp_sockaddr);
        return -1; 
    }

    if (nsock->addrinfo != NULL) {
        free(nsock->addrinfo->addr);
        free(nsock->addrinfo);
    }

    nsock->addrinfo = temp_addrinfo;
    nsock->addrinfo->addr = temp_sockaddr;
    nsock->addrinfo->len = len;

    memcpy(nsock->addrinfo->addr, addr, len);
    return 0;
}

static int init_socket(struct net_socket* nsock) {
    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return -1;
    }
    nsock->fd = sockfd;
    return 0;
}


static int map_gai_error_to_errno(int gai_err) {
    switch (gai_err) {
        case EAI_AGAIN:  return EAGAIN;
        case EAI_FAIL:   return EADDRNOTAVAIL;
        case EAI_FAMILY: return EADDRNOTAVAIL;
        case EAI_MEMORY: return ENOMEM;
        case EAI_NONAME: return ENOENT;
        default:         return EINVAL;
    }
}


