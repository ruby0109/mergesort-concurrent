#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define TEST 1
#include "threadpool.h"
#include "list.h"
#define USAGE "usage: ./sort [ThrdCount] [InputFile]\n"

struct {
    pthread_mutex_t mutex;
    int cut_ThrdCount;
} data_context;

static llist_t *tmp_list;
static llist_t *the_list = NULL;

static int ThrdCount = 0, DataCount = 0, max_cut = 0;
static tpool_t *pool = NULL;

#if defined(TEST)
static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}
#endif


llist_t *MergeList(llist_t *a, llist_t *b)
{
    llist_t *_list = list_new();
    node_t *current = NULL;
    while (a->size && b->size) {
        /* If a->head->data smaller than or equel to b->head->data, return 1*/
        int cmp = (strcmp(a->head->data, b->head->data) <= 0) ;
        llist_t *small = (llist_t *)((intptr_t) a * cmp +
                                     (intptr_t) b * (1-cmp));
        if (current) {
            current->next = small->head;
            current = current->next;
        } else {
            _list->head = small->head;
            current = _list->head;
        }
        small->head = small->head->next;
        --small->size;
        ++_list->size;
        current->next = NULL;
    }

    llist_t *remaining = (llist_t *) ((intptr_t) a * (a->size > 0) +
                                      (intptr_t) b * (b->size > 0));
    if (current) current->next = remaining->head;
    _list->size += remaining->size;
    free(a);
    free(b);
    return _list;
}

llist_t *merge_sort(llist_t *list)
{
    if (list->size < 2)
        return list;
    int mid = list->size / 2;
    llist_t *left = list;
    llist_t *right = list_new();
    right->head = list_nth(list, mid);
    right->size = list->size - mid;
    list_nth(list, mid - 1)->next = NULL;
    left->size = mid;
    return MergeList(merge_sort(left), merge_sort(right));
}

void merge(void *data)
{
    llist_t *_list = (llist_t *) data;
    if (_list->size < (uint32_t) DataCount) {
        llist_t *_t = tmp_list;
        if (!_t) {
            tmp_list = _list;
        } else {
            tmp_list = NULL;
            task_t *_task = (task_t *) malloc(sizeof(task_t));
            _task->func = merge;
            _task->arg = MergeList(_list, _t);
            tqueue_push(pool->queue, _task);
        }
    } else {
        the_list = _list;
        task_t *_task = (task_t *) malloc(sizeof(task_t));
        _task->func = NULL;
        tqueue_push(pool->queue, _task);
        list_print(_list);
    }
}

void cut_func(void *data)
{
    llist_t *Llist = (llist_t *) data;
    pthread_mutex_lock(&(data_context.mutex));
    int cut_count = data_context.cut_ThrdCount;
    if (Llist->size > 1 && cut_count < max_cut) {
        ++data_context.cut_ThrdCount;
        pthread_mutex_unlock(&(data_context.mutex));

        /* cut list */
        int mid = Llist->size / 2;
        llist_t *Rlist = list_new();
        Rlist->head = list_nth(Llist, mid);
        Rlist->size = Llist->size - mid;
        list_nth(Llist, mid - 1)->next = NULL;
        Llist->size = mid;

        /* Create new task for the right list */
        task_t *_task = (task_t *) malloc(sizeof(task_t));
        _task->func = cut_func;
        _task->arg = Rlist;
        tqueue_push(pool->queue, _task);

        /* Create new task for the left list */
        _task = (task_t *) malloc(sizeof(task_t));
        _task->func = cut_func;
        _task->arg = Llist;
        tqueue_push(pool->queue, _task);
    } else {
        pthread_mutex_unlock(&(data_context.mutex));
        merge(merge_sort(Llist));
    }
}

static void *task_run(void *data)
{
    (void) data;
    while (1) {

        /* Barrier to wait before tasks in queue*/
        pthread_mutex_lock(&(pool->queue->mutex));

        while( pool->queue->size == 0) {
            pthread_cond_wait(&(pool->queue->cond), &(pool->queue->mutex));
        }
        pthread_mutex_unlock(&(pool->queue->mutex));

        /* Check again whether the queue is empty*/
        if (pool->queue->size == 0) break;

        /* Thread grab the task*/
        task_t *_task = tqueue_pop(pool->queue);

        if (_task) {
            if (!_task->func) {
                tqueue_push(pool->queue, _task);
                break;
            } else {
                _task->func(_task->arg);
                free(_task);
            }
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{

    FILE *fp;
    char line[MAX_LAST_NAME_SIZE];


    /*==========Read The Command==========*/
    if (argc < 3) {
        printf(USAGE);
        return -1;
    }
    ThrdCount = atoi(argv[1]);

    /* Initialize the list*/
    the_list = list_new();

    /*==========File Preprocessing==========*/
    /* check file opening*/
    fp = fopen(argv[2],"r");
    if(!fp) {
        printf("Cannot open the file\n");
        return -1;
    }

    /* Read the File*/
    while(fgets(line,MAX_LAST_NAME_SIZE,fp)) {
        line[strlen(line)-1] = '\0';
        list_add(the_list,line);
        /* Get the quantity of the data*/
        DataCount++;
    }

    /* Close the file*/
    fclose(fp);

    max_cut = ThrdCount * (ThrdCount <= DataCount) +
              DataCount * (ThrdCount > DataCount) - 1;
    /*========== Thread Pool Initialization==========*/
    /* initialize tasks inside thread pool */
    pthread_mutex_init(&(data_context.mutex), NULL);
    data_context.cut_ThrdCount = 0;
    tmp_list = NULL;
    pool = (tpool_t *) malloc(sizeof(tpool_t));
    tpool_init(pool, ThrdCount, task_run);

#if defined(TEST)
    struct timespec start, end;
    double cpu_time;
    clock_gettime(CLOCK_REALTIME, &start);
#endif
    /* launch the first task */
    task_t *_task = (task_t *) malloc(sizeof(task_t));
    _task->func = cut_func;
    _task->arg = the_list;
    tqueue_push(pool->queue, _task);

    /* release thread pool */
    tpool_free(pool);

#if defined(TEST)
    clock_gettime(CLOCK_REALTIME, &end);
    FILE *output;
    cpu_time=diff_in_second(start,end);
    output = fopen("output.txt","a+");
    fprintf(output, "Thread number: %d, excution time: %lf second\n", ThrdCount, cpu_time);
    fclose(output);
#endif

    return 0;
}


