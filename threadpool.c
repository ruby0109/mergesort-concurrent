#include "threadpool.h"

int task_free(task_t *the_task)
{
    free(the_task->arg);
    free(the_task);
    return 0;
}

int tqueue_init(tqueue_t *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&(queue->mutex), NULL);
    pthread_cond_init(&(queue->cond), NULL);
    queue->size = 0;
    return 0;
}

task_t *tqueue_pop(tqueue_t *queue)
{
    task_t *ret;
    ret = queue->head;
    if (ret) {
        queue->head = ret->prev;
        if (queue->head) {
            queue->head->next = NULL;
        } else {
            queue->tail = NULL;
        }
        queue->size--;
    }
    return ret;
}

int tqueue_push(tqueue_t *queue, task_t *task)
{
    pthread_mutex_lock(&(queue->mutex));
    task->prev = NULL;
    task->next = queue->tail;
    if (queue->tail)
        queue->tail->prev = task;
    queue->tail = task;
    if (queue->size++ == 0)
        queue->head = task;
    pthread_cond_signal(&(queue->cond));
    pthread_mutex_unlock(&(queue->mutex));
    return 0;
}

int tqueue_free(tqueue_t *queue)
{
    task_t *cur = queue->tail;
    while (cur) {
        queue->tail = queue->tail->next;
        free(cur);
        cur = queue->tail;
    }
    pthread_mutex_lock(&(queue->mutex));
    pthread_mutex_destroy(&(queue->mutex));
    pthread_cond_destroy(&(queue->cond));
    return 0;
}

int tpool_init(tpool_t *pool, uint32_t tcount, void *(*func)(void *))
{
    pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * tcount);
    pool->count = tcount;
    pool->queue = (tqueue_t *) malloc(sizeof(tqueue_t));
    tqueue_init(pool->queue);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (uint32_t i = 0; i < tcount; ++i)
        pthread_create(&(pool->threads[i]), &attr, func, NULL);
    pthread_attr_destroy(&attr);
    return 0;
}

int tpool_free(tpool_t *pool)
{
    pthread_cond_broadcast(&(pool->queue->cond));
    pthread_mutex_unlock(&(pool->queue->mutex));
    for (uint32_t i = 0; i < pool->count; ++i)
        pthread_join(pool->threads[i], NULL);
    free(pool->threads);
    tqueue_free(pool->queue);
    return 0;
}
