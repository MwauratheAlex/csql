#include "threadpool.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void* worker_loop(void *arg);

void thread_pool_init(ThreadPool *pool) {
    pool->queue.head = 0;
    pool->queue.tail = 0;
    pool->queue.count = 0;

    if (pthread_mutex_init(&pool->queue.lock, NULL) != 0) {
        perror("Mutex init failed");
        exit(EXIT_FAILURE);
    };

    if (pthread_cond_init(&pool->queue.not_empty, NULL) != 0) {
        perror("Cond init failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_loop, (void *)pool) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }

    }
}

void thread_pool_submit(ThreadPool *pool, int client_fd, struct sockaddr_in addr) {
        pthread_mutex_lock(&pool->queue.lock);

        if (pool->queue.count == QUEUE_SIZE) {
            printf("Queue full. Connection dropped");
            close(client_fd);
            pthread_mutex_unlock(&pool->queue.lock);
            return;
        }

        pool->queue.tasks[pool->queue.tail].client_fd = client_fd; 
        pool->queue.tasks[pool->queue.tail].client_sockaddr = addr; 

        pool->queue.tail = (pool->queue.tail + 1) % QUEUE_SIZE;
        pool->queue.count++;

        pthread_cond_signal(&pool->queue.not_empty);
        pthread_mutex_unlock(&pool->queue.lock);

}

static void* worker_loop(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    char buffer[4096];

    while (1) {
        pthread_mutex_lock(&pool->queue.lock);

        while(pool->queue.count == 0) {
            pthread_cond_wait(&pool->queue.not_empty, &pool->queue.lock);
        }

        Task task = pool->queue.tasks[pool->queue.head];
        pool->queue.head = (pool->queue.head + 1) % QUEUE_SIZE;
        pool->queue.count--;

        pthread_mutex_unlock(&pool->queue.lock);

        printf("[Thread %lu] Accepted Session: %s:%d\n", 
               (unsigned long)pthread_self(), 
               inet_ntoa(task.client_sockaddr.sin_addr), 
               ntohs(task.client_sockaddr.sin_port));

        while (1) {
            memset(buffer, 0, 4096);
            int bytes_read = read(task.client_fd, buffer, 4096);

            if (bytes_read <= 0) break;

            buffer[bytes_read] = '\0';

            printf("[%s:%d] Query: %s", 
                   inet_ntoa(task.client_sockaddr.sin_addr), 
                   ntohs(task.client_sockaddr.sin_port), 
                   buffer);

            // --- ENGINE EXECUTION GOES HERE ---
            write(task.client_fd, "Query Ok\n", 9);
        }



        close(task.client_fd);
        printf("[Thread %lu] Closed Session\n", (unsigned long)pthread_self());
    }

    return NULL;
}
