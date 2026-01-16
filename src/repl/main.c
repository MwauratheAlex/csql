#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT      9000

#define BUFFER_SIZE 4096

int main ()
{
    struct sockaddr_in server_sockaddr;

    char buffer[BUFFER_SIZE] = {0};

    int socket_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror ("Socket failed!");
        exit (EXIT_FAILURE);
    }

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = inet_addr (SERVER_IP);
    server_sockaddr.sin_port = htons (PORT);

    if (connect (socket_fd, (struct sockaddr *) &server_sockaddr,
                 sizeof (server_sockaddr))
        < 0)
    {
        perror ("Connection failed! Is the server running?\n");
        exit (EXIT_FAILURE);
    }

    printf ("--- CSQL REPL ---\n");
    printf ("Type 'exit' to quit\n\n");

    while (1)
    {
        fputs ("csql> ", stdout);

        memset (buffer, 0, BUFFER_SIZE);
        if (fgets (buffer, BUFFER_SIZE, stdin) == NULL)
            break;

        buffer[strcspn (buffer, "\n")] = 0;

        if (strcmp (buffer, "exit") == 0)
            break;
        if (strlen (buffer) == 0)
            continue;

        send (socket_fd, buffer, strlen (buffer), 0);

        while (1)
        {
            int bytes_received = read (socket_fd, buffer, BUFFER_SIZE - 1);
            if (bytes_received <= 0)
            {
                printf ("Server closed connecton\n");
                close (socket_fd);
                return 0;
            }

            buffer[bytes_received] = '\0';
            bool done = false;
            if (strlen (buffer) < bytes_received)
            {
                done = true;
            }

            printf ("%s", buffer);
            if (done)
            {
                break;
            }
            memset (buffer, 0, BUFFER_SIZE);
        }

        printf ("\n");
    }

    close (socket_fd);

    return 0;
}
