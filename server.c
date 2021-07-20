#define _GNU_SOURCE
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>



int socket_fd, client_fd;

void shutdown_server () {
    printf("\nServer is shutting down...\n");

    if (client_fd == -1) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    if (socket_fd == -1) {
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    }

    exit(0);
}

void handler (int signum) { shutdown_server(); }

void attach_handlers () {
    signal(SIGPIPE, SIG_IGN);

    struct sigaction action_int;
    memset(&action_int, 0, sizeof(action_int));
    action_int.sa_handler = handler;
    action_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &action_int, NULL);

    struct sigaction action_term;
    memset(&action_term, 0, sizeof(action_term));
    action_term.sa_handler = handler;
    action_term.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &action_term, NULL);
}

void wheel (int socket_fd) {
    printf("Press ^C (or send a SIGTERM to %d) in order to exit\n", getpid());

    while (true) {
        char receivement[4096];
        memset(receivement, '\0', sizeof(receivement));

        if (read(socket_fd, receivement, sizeof(receivement)) <= 0)
            break;

        printf("Server received a string: %s\n", receivement);
        fflush(stdout);

        char sendment[4096];
        memset(sendment, '\0', sizeof(sendment));
        snprintf(sendment, sizeof(sendment), "%lx", strlen(receivement));

        printf("Server sent a string: %s\n", sendment);
        fflush(stdout);

        if (write(socket_fd, sendment, sizeof(sendment)) <= 0)
            break;
    }
}

int main () {
    attach_handlers();

    printf("Input the port on which you want to start the server: ");
    unsigned port;
    scanf("%d", &port);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("Error while creating a socket for server\n");
        shutdown_server();
    }

    else
        printf("Socket for server created successfully\n");


    int setsockopt_val = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &setsockopt_val, sizeof(setsockopt_val));
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &setsockopt_val, sizeof(setsockopt_val));

    struct sockaddr_in server_address = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(port)
    };

    if (bind(socket_fd, (const struct sockaddr*)(&server_address), sizeof(server_address)) == -1) {
        perror("Error while binding the socket for the client to the proxy endpoint\n");
        shutdown_server();
    }

    else
        printf("Socket for server bound successfully\n");

    listen(socket_fd, 1);

    struct sockaddr_in client_address;
    socklen_t client_address_length;
    client_fd = accept(socket_fd, (struct sockaddr*)(&client_address), &client_address_length);

    if (client_fd == -1) {
        perror("Error while accepting client\n");
        shutdown_server();
    }

    else
        printf("Client accepted successfully\n");

    wheel(client_fd);
    shutdown_server();

    return 0;
}

