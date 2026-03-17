#ifndef _ZHIYEC_LIST_H
#define _ZHIYEC_LIST_H

#include <zhiyec/types.h>

/* 双向链表API */

#define list_init(head)        \
    do {                       \
        (head)->prev = (head); \
        (head)->next = (head); \
    } while (0)

#define list_insert(pos, node)      \
    do {                            \
        (pos)->next->prev = (node); \
        (node)->next = (pos)->next; \
        (pos)->next = (node);       \
        (node)->prev = (pos);       \
    } while (0)

#define list_push_back(head, node)                       \
    do {                                                 \
        if ((head) != NULL) {                            \
            struct list_head *const tail = (head)->prev; \
            list_insert(tail, (node));                   \
        }                                                \
    } while (0)

#define list_remove(node)                  \
    do {                                   \
        (node)->prev->next = (node)->next; \
        (node)->next->prev = (node)->prev; \
        (node)->prev = NULL;               \
        (node)->next = NULL;               \
    } while (0)

/* 结束 */

struct stack_list {
    struct slist_head *head;
};

#define stack_list_init(stack_list) \
    do {                            \
        (stack_list).head = NULL;   \
    } while (0)

#define stack_list_front(stack_list) ((stack_list).head)

#define stack_list_is_empty(stack_list) ((stack_list).head == NULL)

#define stack_list_push(stack_list, node) \
    do {                                  \
        (node)->next = (stack_list).head; \
        (stack_list).head = (node);       \
    } while (0)

#define stack_list_pop(stack_list)                   \
    do {                                             \
        (stack_list).head = (stack_list).head->next; \
    } while (0)

struct queue_list {
    struct slist_head head;
    struct slist_head *tail;
};

#define queue_list_init(queue_list)               \
    do {                                          \
        (queue_list).tail = &((queue_list).head); \
    } while (0)

#define queue_list_front(queue_list) ((queue_list).head.next)

#define queue_list_back(queue_list) ((queue_list).tail)

#define queue_list_is_empty(queue_list) (&((queue_list).head) == (queue_list).tail)

#define queue_list_push(queue_list, node)            \
    do {                                             \
        (node)->next = NULL;                         \
        (queue_list).tail->next = (node);            \
        (queue_list).tail = (queue_list).tail->next; \
    } while (0)

#define queue_list_pop(queue_list)                                          \
    do {                                                                    \
        struct slist_head *const front_node = queue_list_front(queue_list); \
        (queue_list).head.next = front_node->next;                          \
        if (front_node == (queue_list).tail) {                              \
            (queue_list).tail = &((queue_list).head);                       \
        }                                                                   \
        front_node->next = NULL;                                            \
    } while (0)

#endif /* _ZHIYEC_LIST_H */
