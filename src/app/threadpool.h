#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "server.h"

#include <netinet/in.h>
#include <pthread.h>

#define THREAD_POOL_SIZE 4
#define QUEUE_SIZE 256

typedef struct
{
    int client_fd;
    struct sockaddr_in client_sockaddr;
} Task;

typedef struct
{
    Task tasks[QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} Queue;

typedef struct
{
    Queue queue;
    pthread_t threads[THREAD_POOL_SIZE];

    Database *db;
} ThreadPool;

void thread_pool_init (ThreadPool *pool, Database *db);
void thread_pool_submit (ThreadPool *pool, int client_fd,
                         struct sockaddr_in addr);

#endif /* THREAD_POOL_H */
