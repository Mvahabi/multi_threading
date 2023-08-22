#include "server.h"

int main(int argc, char *argv[]) {
    int port;
    int result;
    Listener_Socket sock;

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }

    port = atoi(argv[1]);
    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    result = listener_init(&sock, port);
    if (result == -1) {
        fprintf(stderr, "Failed to listen\n");
        exit(1);
    }
    for (;;) {
        int fd = listener_accept(&sock);
        if (fd == -1) {
            fprintf(stderr, "Failed to accept\n");
            exit(1);
        }
        result = echo(fd);
        close(fd);
    }
    return 0;
}
