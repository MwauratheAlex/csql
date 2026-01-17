#include "server.h"

#include "../btree/btree.h"
#include "threadpool.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

ThreadPool conn_pool;

#define GLOBAL_HEAP_SIZE (SIZE_MB * 64)
unsigned char global_buffer[GLOBAL_HEAP_SIZE];
Arena global_arena;

/**
 * server_start - starts a web server and waits for connections
 */
void server_start ()
{
    arena_init (&global_arena, global_buffer, GLOBAL_HEAP_SIZE);

    Database *db = push_struct_zero (&global_arena, Database);
    db->global_arena = &global_arena;

    db->pager = pager_open (&global_arena, "csql.db");
    if (db->pager == NULL)
    {
        fprintf (stderr, "Error: Could not open file %s\n", "csql.db");
        exit (EXIT_FAILURE);
    }

    if (pthread_mutex_init (&db->lock, NULL) != 0)
    {
        perror ("DB Mutex Init failed");
        exit (EXIT_FAILURE);
    }

    if (db->pager->num_pages == 0)
    {
        // page 0 - catalog root
        void *page_zero = pager_get_page (db->global_arena, db->pager, 0);
        initialize_leaf_node (page_zero);
        set_node_root (page_zero, 1);
        db->pager->num_pages = 1;
    }
    else
    {
        catalog_init_from_disk (db);
    }

    thread_pool_init (&conn_pool, db);

    struct sockaddr_in server_sockaddr;
    int opt = 1;

    int socket_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror ("Socket failed!");
        exit (EXIT_FAILURE);
    }

    if (setsockopt (socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) < 0
        || setsockopt (socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt))
               < 0)
    {
        perror ("setsockopt failed!");
        exit (EXIT_FAILURE);
    };

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    server_sockaddr.sin_port = htons (PORT);

    if (bind (socket_fd, (struct sockaddr *) &server_sockaddr,
              sizeof (server_sockaddr))
        < 0)
    {
        perror ("bind failed!");
        exit (EXIT_FAILURE);
    };

    if (listen (socket_fd, 5) < 0)
    {
        perror ("listening failed!");
        exit (EXIT_FAILURE);
    };

    char server_ip[INET_ADDRSTRLEN];
    inet_ntop (AF_INET, &(server_sockaddr.sin_addr), server_ip,
               INET_ADDRSTRLEN);
    printf ("CSQL Server listening on %s:%d\n", server_ip, PORT);

    struct sockaddr_in client_sockaddr;
    socklen_t client_sockaddr_len = sizeof (client_sockaddr);

    while (1)
    {
        printf ("Waiting for a connection...\n");

        int client_fd = accept (socket_fd, (struct sockaddr *) &client_sockaddr,
                                &client_sockaddr_len);
        if (client_fd < 0)
        {
            perror ("accept!");
            continue;
        }

        printf ("Accepted connection from %s:%d\n",
                inet_ntoa (client_sockaddr.sin_addr),
                ntohs (client_sockaddr.sin_port));
        thread_pool_submit (&conn_pool, client_fd, client_sockaddr);
    }

    pager_close (db->pager);
    close (socket_fd);
}
