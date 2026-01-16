#include "threadpool.h"

#include "../executor/executor.h"
#include "../parser/parser.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *worker_loop (void *arg);

void thread_pool_init (ThreadPool *pool, Database *db)
{
    pool->db = db;
    pool->queue.head = 0;
    pool->queue.tail = 0;
    pool->queue.count = 0;

    if (pthread_mutex_init (&pool->queue.lock, NULL) != 0)
    {
        perror ("Mutex init failed");
        exit (EXIT_FAILURE);
    };

    if (pthread_cond_init (&pool->queue.not_empty, NULL) != 0)
    {
        perror ("Cond init failed");
        exit (EXIT_FAILURE);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (pthread_create (&pool->threads[i], NULL, worker_loop, (void *) pool)
            != 0)
        {
            perror ("Failed to create thread");
            exit (EXIT_FAILURE);
        }
    }
}

void thread_pool_submit (ThreadPool *pool, int client_fd,
                         struct sockaddr_in addr)
{
    pthread_mutex_lock (&pool->queue.lock);

    if (pool->queue.count == QUEUE_SIZE)
    {
        printf ("Queue full. Connection dropped");
        close (client_fd);
        pthread_mutex_unlock (&pool->queue.lock);
        return;
    }

    pool->queue.tasks[pool->queue.tail].client_fd = client_fd;
    pool->queue.tasks[pool->queue.tail].client_sockaddr = addr;

    pool->queue.tail = (pool->queue.tail + 1) % QUEUE_SIZE;
    pool->queue.count++;

    pthread_cond_signal (&pool->queue.not_empty);
    pthread_mutex_unlock (&pool->queue.lock);
}

static void *worker_loop (void *arg)
{
    ThreadPool *pool = (ThreadPool *) arg;
    char buffer[4096];

    unsigned char conn_buffer[SIZE_MB];
    Arena conn_arena;
    arena_init (&conn_arena, conn_buffer, SIZE_MB);

    while (1)
    {
        pthread_mutex_lock (&pool->queue.lock);

        while (pool->queue.count == 0)
        {
            pthread_cond_wait (&pool->queue.not_empty, &pool->queue.lock);
        }

        Task task = pool->queue.tasks[pool->queue.head];
        pool->queue.head = (pool->queue.head + 1) % QUEUE_SIZE;
        pool->queue.count--;

        pthread_mutex_unlock (&pool->queue.lock);

        printf ("[Thread %lu] Accepted Session: %s:%d\n",
                (unsigned long) pthread_self (),
                inet_ntoa (task.client_sockaddr.sin_addr),
                ntohs (task.client_sockaddr.sin_port));

        while (1)
        {
            memset (buffer, 0, 4096);
            int bytes_read = read (task.client_fd, buffer, 4096);

            if (bytes_read <= 0)
                break;

            buffer[bytes_read] = '\0';

            Parser p;
            parser_init (&p, buffer);
            Statement stmt = parser_parse_statement (&p);

            if (stmt.type == STMT_ERROR)
            {
                write (task.client_fd, "Error: ", 7);
                write (task.client_fd, stmt.error.msg, strlen (stmt.error.msg));
                write (task.client_fd, "\n", 1);
                write (task.client_fd, "",
                       1); // null terminator to show done
                continue;
            }

            pthread_mutex_lock (&pool->db->lock);

            char *result_msg = NULL;
            ExecuteResult result =
                execute_statement (&stmt, pool->db, task.client_fd);
            pthread_mutex_unlock (&pool->db->lock);

            switch (result)
            {
            case EXECUTE_SUCCESS:
                write (task.client_fd, "OK.\n", 4);
                break;
            case EXECUTE_DB_FULL:
                write (task.client_fd, "Error: Database full.\n", 22);
                break;
            case EXECUTE_TABLE_EXISTS:
                write (task.client_fd, "Error: Table exists.\n", 21);
                break;
            case EXECUTE_TABLE_FULL:
                write (task.client_fd, "Error: Table full.\n", 19);
                break;
            case EXECUTE_TABLE_NOT_EXISTS:
                write (task.client_fd, "Error: Table not found.\n", 24);
                break;
            case EXECUTE_TABLE_COL_COUNT_MISMATCH:
                write (task.client_fd, "Error: Column count mismatch.\n", 30);
                break;
            case EXECUTE_COL_NOT_FOUND:
                write (task.client_fd, "Error: Column not found.\n", 25);
                break;
            case EXECUTE_DUPLICATE_KEY:
                write (task.client_fd, "Error: Duplicate key.\n", 22);
                break;
            default:
                write (task.client_fd, "Execution failed.\n", 10);
                break;
            }
            write (task.client_fd, "",
                   1); // null terminator so show done
        }

        close (task.client_fd);
        printf ("[Thread %lu] Closed Session\n",
                (unsigned long) pthread_self ());
    }

    return NULL;
}
