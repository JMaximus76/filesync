#include "error.h"
#include "net_socket.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define SERV_PORT 20727

struct test_data {
    int  a;
    int  b;
    int  c;
    char str[100]; // NOLINT
};

void print_test_data(const struct test_data *data) {
    printf("test_data {\n");
    printf("  a = %d\n", data->a);
    printf("  b = %d\n", data->b);
    printf("  c = %d\n", data->c);
    printf("  str = \"%s\"\n", data->str);
    printf("}\n");
}

void run(jfs_err_t *err) { // NOLINT
    jfs_ns_socket_t *client;
    jfs_ns_socket_t *server;
    struct test_data recv_data = {0};
    struct test_data send_data = {
        .a = 85, // NOLINT
        .b = 69, // NOLINT
        .c = 42, // NOLINT
        .str = "uma musume: pretty derby",
    };

    server = jfs_ns_socket_create(err);
    GOTO_IF_ERR(cleanup);

    jfs_ns_socket_open(server, err);
    GOTO_IF_ERR(cleanup);

    jfs_ns_socket_set_ip(server, SERV_PORT, "0.0.0.0", err);
    GOTO_IF_ERR(cleanup);

    jfs_ns_socket_bind(server, err);
    GOTO_IF_ERR(cleanup);

    jfs_ns_socket_listen(server, err);
    GOTO_IF_ERR(cleanup);

    client = jfs_ns_socket_accept(server, err);
    GOTO_IF_ERR(cleanup);

    jfs_ns_socket_recv(client, &recv_data, sizeof(recv_data), err);
    GOTO_IF_ERR(cleanup);

    print_test_data(&recv_data);

    jfs_ns_socket_send(client, &send_data, sizeof(send_data), 0, err);
    GOTO_IF_ERR(cleanup);

    jfs_ns_socket_shutdown(client, err);
    GOTO_IF_ERR(cleanup);

    while (1) {
        jfs_ns_socket_recv(client, &recv_data, sizeof(recv_data), err);
        if (*err == JFS_ERR_NS_CONNECTION_CLOSE) {
            printf("safe shutdown\n");
            RES_ERR;
            break;
        }

        if (*err != JFS_OK) {
            printf("unsafe shutdown\n");
            GOTO_IF_ERR(cleanup);
        }
        print_test_data(&recv_data);
    }

    jfs_ns_socket_close(client, err);
    if (*err != JFS_OK) RES_ERR;
    jfs_ns_socket_destroy(&client);
    jfs_ns_socket_destroy(&server);

    return;
cleanup:
    jfs_ns_socket_destroy(&server);
    jfs_ns_socket_destroy(&client);
    VOID_RETURN_ERR;
}

int main() {
    jfs_err_t err;
    run(&err);
    return 0;
}
