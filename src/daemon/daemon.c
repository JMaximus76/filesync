#include "net_socket.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    struct net_socket *client = net_socket_create();
    if (client == NULL) {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    if (net_socket_set_host(client, 30727, "jmax-server.local") == -1) {
        perror("failed to address");
        exit(EXIT_FAILURE);
    }

    if (net_socket_connect(client) == -1) {
        perror("failed to connect");
        exit(EXIT_FAILURE);
    }

    printf("connected\n");

    int fd = open("pic.jpg", O_RDONLY);
    if (fd == -1) {
        perror("failed to open file");
        net_socket_destroy(&client);
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("failed to get stat");
        net_socket_destroy(&client);
    }

    const size_t size_of_file = sb.st_size;
    const size_t size_to_send = sizeof(size_of_file);

    printf("size of file: %zu\n", size_of_file);

    ssize_t size_sent = net_socket_send(client, &size_of_file, size_to_send, 0);
    if (size_sent == -1) {
        perror("failed to send");
        exit(EXIT_FAILURE);
    }
    if ((size_t) size_sent < size_to_send) {
        printf("partial send then error: %zu\n", size_sent);
        perror("failed to send");
        exit(EXIT_FAILURE);
    }

    printf("starting send\n");
    size_t total_sent = 0;
    int    file_status = net_socket_file_send(client, fd, sb.st_size, &total_sent);
    if (file_status < 0) {
        if (file_status == -1) {
            perror("failed to read");
        }
        if (file_status == -2) {
            perror("failed to send");
        }
        if (file_status == -3) {
            perror("file size to small");
        }
        net_socket_destroy(&client);
    }

    net_socket_shutdown(client);
    char recv_buf[4000];
    while (1) {
        ssize_t r = net_socket_recv(client, recv_buf, 4000);

        if (r == 0) {
            printf("safe shutdown\n");
            break;
        }

        if (r == -1) {
            perror("shutdown recv fail");
            net_socket_destroy(&client);
            exit(EXIT_FAILURE);
        }
    }

    net_socket_destroy(&client);
    return 0;
}
