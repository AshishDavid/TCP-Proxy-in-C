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



int socket_fd;

void shutdown_client () {
    printf("\nClient is shutting down...\n");

    if (socket_fd != -1)
        close(socket_fd);

    exit(0);
}

void handler (int signum) { shutdown_client(); }

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
    printf("Press ^C (or send the SIGTERM to %d) in order to exit\n", getpid());

    while (true) {
        char sendment[4096];
        memset(sendment, '\0', sizeof(sendment));

        printf("Write a message to the server: ");

        if (scanf("%s", sendment) <= 0)
            break;

        if (write(socket_fd, sendment, sizeof(sendment)) <= 0) {
            printf("Connection to the server lost.\n");
            break;
        }

        char receivement[4096];
        memset(receivement, '\0', sizeof(receivement));

        if (read(socket_fd, receivement, sizeof(receivement)) <= 0) {
            printf("Connection to the server lost.\n");
            break;
        }

        printf("Received the data from the server: %s\n", receivement);
    }
}

int main () {
    attach_handlers();

    printf("Input the port of the server: ");
    unsigned port;
    scanf("%d", &port);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("Error while creating socket\n");
        shutdown_client();
    }

    else
        printf("Socket created successfully\n");

    struct sockaddr_in server_address = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = inet_addr("127.0.0.1"),
            .sin_port = htons(port)
    };

    if (connect(socket_fd, (const struct sockaddr*)(&server_address), sizeof(server_address)) == -1) {
        perror("Error while connecting to the server\n");
        shutdown_client();
    }

    else
        printf("Client connected to the server.\n");

    wheel(socket_fd);
    shutdown_client();

    return 0;
}

