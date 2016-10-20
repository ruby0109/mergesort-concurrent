#ifndef LLIST_H_
#define LLIST_H_

#define MAX_LAST_NAME_SIZE 16
#include <stdint.h>

typedef char* val_t;

typedef struct node {
    char data[MAX_LAST_NAME_SIZE];
    struct node *next;
} node_t;

typedef struct llist {
    node_t *head;
    uint32_t size;
} llist_t;

llist_t *list_new();
int list_add(llist_t *the_list, val_t val);
void list_print(llist_t *the_list);
node_t *list_nth(llist_t *the_list, uint32_t index);

#endif
