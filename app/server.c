#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 4096

/**
 * server_start - starts a web server and waits for connections
 */
void server_start() {
    struct sockaddr_in server_sockaddr;
    int opt = 1;

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Socket failed!");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 ||
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0  ) {
        perror("setsockopt failed!");
        exit(EXIT_FAILURE);
    };

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    server_sockaddr.sin_port = htons(PORT);

    if(bind(socket_fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) < 0) {
        perror("bind failed!");
        exit(EXIT_FAILURE);
    };

    if(listen(socket_fd, 5) < 0) {
        perror("listening failed!");
        exit(EXIT_FAILURE);
    };
    printf("CSQL Server listening on port %d\n", PORT);

    struct sockaddr_in client_sockaddr;
    socklen_t client_sockaddr_len = sizeof(client_sockaddr);

    while (1) {
        printf("Waiting for a connection...\n");

        int connection_fd = accept(socket_fd, (struct sockaddr *)&client_sockaddr, &client_sockaddr_len);
        if (connection_fd < 0) {
            perror("accept!");
            continue;
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_sockaddr.sin_addr), ntohs(client_sockaddr.sin_port));

        char buffer[BUFFER_SIZE];
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);

            if (read(connection_fd, buffer, sizeof(buffer)) > 0 ) {
                printf("[%s:%d] says: %s\n", inet_ntoa(client_sockaddr.sin_addr), ntohs(client_sockaddr.sin_port), buffer);

                char *response = "Query Ok\n";
                write(connection_fd, response, strlen(response));

            } else {
                printf("Client disconnected.\n");
                break;
            }
        }

        close(connection_fd);
    }

    close(socket_fd);
}
