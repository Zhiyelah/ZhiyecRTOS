#ifndef _QueueList_h
#define _QueueList_h

#include "List.h"

struct QueueList {
    struct SListHead head;
    struct SListHead *tail;
};

#define QueueList_init(queue_list)                \
    do {                                          \
        (queue_list).tail = &((queue_list).head); \
    } while (0)

#define QueueList_front(queue_list) ((queue_list).head.next)

#define QueueList_isEmpty(queue_list) (&((queue_list).head) == (queue_list).tail)

#define QueueList_push(queue_list, node)             \
    do {                                             \
        (node)->next = NULL;                         \
        (queue_list).tail->next = (node);            \
        (queue_list).tail = (queue_list).tail->next; \
    } while (0)

#define QueueList_pop(queue_list)                                         \
    do {                                                                  \
        struct SListHead *const front_node = QueueList_front(queue_list); \
        (queue_list).head.next = front_node->next;                        \
        if (front_node == (queue_list).tail) {                            \
            (queue_list).tail = &((queue_list).head);                     \
        }                                                                 \
        front_node->next = NULL;                                          \
    } while (0)

#endif /* _QueueList_h */
