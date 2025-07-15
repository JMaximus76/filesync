#include "net_socket.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    struct net_socket *server = net_socket_create();
    if (server == NULL) {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    if (net_socket_set_ip(server, 30727, "0.0.0.0") == -1) {
        perror("failed to address");
        exit(EXIT_FAILURE);
    }

    if (net_socket_bind(server) == -1) {
        perror("failed to bind");
        exit(EXIT_FAILURE);
    }

    if (net_socket_listen(server) == -1) {
        perror("failed to listen");
        exit(EXIT_FAILURE);
    }

    struct net_socket *connection;
    if (net_socket_accept(server, &connection) == -1) {
        perror("failed to accept");
        exit(EXIT_FAILURE);
    }

    printf("connected to client\n");


    size_t size_of_file;

    ssize_t size_recv = net_socket_recv(connection, &size_of_file, sizeof(size_of_file));
    if (size_recv == -1) {
        perror("failed to read");
        net_socket_destroy(&connection);
        net_socket_destroy(&server);
        exit(EXIT_FAILURE);
    }
    if (size_recv == 0) {
        perror("peer closed connection unexpectedly");
        net_socket_destroy(&connection);
        net_socket_destroy(&server);
        exit(EXIT_FAILURE);
    }

    printf("the size of the file is: %zu\n", size_of_file);

    int fd = open("pic.jpg", O_WRONLY);
    if (fd == -1) {
        perror("failed to open file");
        net_socket_destroy(&connection);
        net_socket_destroy(&server);
        exit(EXIT_FAILURE);
    }

    size_t tr;
    size_t tw;
    int status = net_socket_file_recv(connection, fd, size_of_file, &tr, &tw);
    if (status == -1) {
        perror("error on file recv");
        net_socket_destroy(&connection);
        net_socket_destroy(&server);
        exit(EXIT_FAILURE);
    }
    if (status == -2) {
        perror("error on file write");
        net_socket_destroy(&connection);
        net_socket_destroy(&server);
        exit(EXIT_FAILURE);
    }
    if (status == -3) {
        perror("peer closed connection");
        net_socket_destroy(&connection);
        net_socket_destroy(&server);
        exit(EXIT_FAILURE);
    }

    printf("size recv: %zu\n", tr);
    printf("size written: %zu\n", tw);



    net_socket_shutdown(connection);
    char recv_buf[4000];
    while (1) {
        ssize_t r = net_socket_recv(connection, recv_buf, 4000);

        if (r == 0) {
            printf("safe shutdown\n");
            break;
        }

        if (r == -1) {
            perror("shutdown recv fail");
            net_socket_destroy(&connection);
            net_socket_destroy(&server);
            exit(EXIT_FAILURE);
        }
    }
    net_socket_destroy(&server);
    net_socket_destroy(&connection);

    return 0;
}
