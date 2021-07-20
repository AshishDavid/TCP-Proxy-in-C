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



int socket_proxy_as_server_fd, socket_proxy_as_client_fd, client_fd;

void shutdown_proxy_as_server () {
    printf("\nProxy Server is shutting down...\n");

    if (client_fd == -1) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    if (socket_proxy_as_server_fd== -1) {
        shutdown(socket_proxy_as_server_fd, SHUT_RDWR);
        close(socket_proxy_as_server_fd);
    }

    exit(0);
}

void shutdown_proxy_as_client () {
    printf("\nProxy Client is shutting down...\n");

    if (socket_proxy_as_client_fd != -1)
        close(socket_proxy_as_client_fd);

    exit(0);
}

void handler (int signum) {
    shutdown_proxy_as_client();
    shutdown_proxy_as_server();
}

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

void wheel (int socket_proxy_as_client_fd, int socket_proxy_as_server_fd_) {
	printf("Press ^C (or send a SIGTERM to %d) in order to exit\n", getpid());

       	while (true) {
        char receivement[4096];
        memset(receivement, '\0', sizeof(receivement));

        if (read(socket_proxy_as_server_fd_, receivement, sizeof(receivement)) <= 0)
            break;
        
        char sendment[4096];
        memset(sendment, '\0', sizeof(sendment));
        strcpy(sendment, receivement);

        if (write(socket_proxy_as_client_fd, sendment, sizeof(sendment)) <= 0)
            break;

        char receive[4096];
        memset(receive, '\0', sizeof(receive));
        if (read(socket_proxy_as_client_fd, receive, sizeof(receive)) <= 0)
            break;

        char send[4096];
        memset(send, '\0', sizeof(send));
        strcpy(send, receive);
        
        if (write(socket_proxy_as_server_fd_, send, sizeof(send)) <= 0)
            break;
    }
}

int main () {
    attach_handlers();

    printf("Input the port on which you want to start the proxy: ");
    unsigned port_proxy;
    scanf("%d", &port_proxy);

    printf("Input the port of the server: ");
    unsigned port_server;
    scanf("%d", &port_server);

    socket_proxy_as_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    socket_proxy_as_client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_proxy_as_client_fd == -1) {
        perror("Error while creating socket\n");
        shutdown_proxy_as_client();
    }

    else
        printf("Socket for server created successfully\n");


    struct sockaddr_in server_address_as_client = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = inet_addr("127.0.0.1"),
            .sin_port = htons(port_server)
    };

    if (connect(socket_proxy_as_client_fd, (const struct sockaddr*)(&server_address_as_client), sizeof(server_address_as_client)) == -1) {
        perror("Error while connecting to the server\n");
        shutdown_proxy_as_client();
    }

    else
        printf("Connected to the server.\n");

    if (socket_proxy_as_server_fd== -1) {
        perror("Error while creating a socket for server\n");
        shutdown_proxy_as_server();
    }

    else
        printf("Socket for server created successfully\n");


    int setsockopt_val = 1;
    setsockopt(socket_proxy_as_server_fd, SOL_SOCKET, SO_REUSEADDR, &setsockopt_val, sizeof(setsockopt_val));
    setsockopt(socket_proxy_as_server_fd, SOL_SOCKET, SO_REUSEPORT, &setsockopt_val, sizeof(setsockopt_val));

    struct sockaddr_in server_address_as_server = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(port_proxy)
    };

    if (bind(socket_proxy_as_server_fd, (const struct sockaddr*)(&server_address_as_server), sizeof(server_address_as_server)) == -1) {
        perror("Socket for client bound unsuccessful\n");
        shutdown_proxy_as_server();
    }

    else {
        printf("Socket for client bound successfully\n");
    }

    listen(socket_proxy_as_server_fd, 1);

    struct sockaddr_in client_address;
    socklen_t client_address_length;
    client_fd = accept(socket_proxy_as_server_fd, (struct sockaddr*)(&client_address), &client_address_length);

    if (client_fd == -1) {
        perror("Error while accepting client\n");
        shutdown_proxy_as_server();
    }

    else {
        printf("Client accepted successfully\n");
    }

    wheel(socket_proxy_as_client_fd, client_fd);
    shutdown_proxy_as_client();
    shutdown_proxy_as_server();
    return 0;
}
