#include "threadpool.h"

int threadpool_init(threadpool_t *pool, uint32_t thrdCount, uint32_t queueSize, void *(*func)(void *))
{
    /* Initialize the arguments of threadpool*/
    pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * thrdCount);
    pool->thrdCount = thrdCount;
    pool->queue = (task_t *) malloc(sizeof(task_t) * queueSize);
    pool->size = queueSize;
    pool->count = pool->head = pool->tail = 0;

    /* Initialize mutex and condtion variable*/
    pthread_mutex_init(&(pool->mutex), NULL);
    pthread_cond_init(&(pool->cond), NULL);

    /* Initialize attr to joinable*/
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    /* Create threads*/
    for (uint32_t i = 0; i < thrdCount; ++i)
        pthread_create(&(pool->threads[i]), &attr, func, NULL);

    pthread_attr_destroy(&attr);
    return 0;
}

/* Add tasks from the tail of the queue*/
int tqueue_push(threadpool_t *pool, void (*func)(void *), void *arg)
{
    /* Check whether the queue is full*/
    if(pool->count == pool->size) return 1;

    pthread_mutex_lock(&(pool->mutex));

    /* Add task to the queue*/
    pool->queue[pool->tail].func = func;
    pool->queue[pool->tail].arg = arg;
    pool->tail = (pool->tail+1) % pool->size;
    pool->count += 1;

    /* Signal the threads to work*/
    pthread_cond_signal(&(pool->cond));
    pthread_mutex_unlock(&(pool->mutex));
    return 0;
}

/* Delete task from the head of the queue*/
task_t *tqueue_pop(threadpool_t *pool)
{
    task_t *ret;

    /* Grab the task from the queue*/
    ret = &(pool->queue[pool->head]);

    if (ret) {
        pool->head = (pool->head+1) % pool->size;
        pool->count -= 1;
    }
    return ret;
}

int threadpool_free(threadpool_t *pool)
{
    /* Wake up all threads and unlock*/
    pthread_cond_broadcast(&(pool->cond));
    pthread_mutex_unlock(&(pool->mutex));

    /* Join the threads*/
    for (uint32_t i = 0; i < pool->thrdCount; ++i)
        pthread_join(pool->threads[i], NULL);

    /* Destroy the lock and condition variable*/
    pthread_mutex_lock(&(pool->mutex));
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->cond));

    /*Free thread pool*/
    free(pool->threads);
    free(pool->queue);
    free(pool);

    return 0;
}
