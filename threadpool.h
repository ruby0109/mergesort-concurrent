#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Struct task_t : Contain the function and arguments for the task.
 * *next : For the single linked list structure.
 */
typedef struct _task {
    void (*func)(void *);
    void *arg;
    struct _task *next;
} task_t;

int task_free(task_t *the_task);

typedef struct _THREAD_POOL {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t *threads;
    uint32_t thrdCount;
    task_t *queue;
    uint32_t size;
    uint32_t count;
    uint32_t head; //index of the head of the queue
    uint32_t tail; //index of the tail of the queue
} threadpool_t;

int tqueue_push(threadpool_t *pool, void (*func)(void *), void *arg);
task_t *tqueue_pop(threadpool_t *pool);

int threadpool_init(threadpool_t *pool, uint32_t thrdCount, uint32_t size, void *(*func)(void *));
int threadpool_free(threadpool_t *pool);

#endif

