#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

/* 
 * Struct task_t : Contain the function and arguments for the task.
 * *next and *prev : For the double linked list structure.
 */ 
typedef struct _task {
    void (*func)(void *);
    void *arg;
    struct _task *next, *prev;
} task_t;

int task_free(task_t *the_task);

/*
 * Struct tqueue_t : Record the double linked list structrue of the task.
 * *head, *tail : Record the head and tail of the structure.
 * size : the size of the linked list structure.
 */
typedef struct {
    task_t *head, *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint32_t size;
} tqueue_t;

int tqueue_init(tqueue_t *the_queue);
task_t *tqueue_pop(tqueue_t *the_queue);
uint32_t tqueue_size(tqueue_t *the_queue);
int tqueue_push(tqueue_t *the_queue, task_t *task);
int tqueue_free(tqueue_t *the_queue);

typedef struct {
    pthread_t *threads;
    uint32_t count;
    tqueue_t *queue;
} tpool_t;

int tpool_init(tpool_t *the_pool, uint32_t count, void *(*func)(void *));
int tpool_free(tpool_t *the_pool);

#endif
