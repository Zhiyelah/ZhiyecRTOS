#ifndef _ZHIYEC_LIST_H
#define _ZHIYEC_LIST_H

#include <zhiyec/types.h>

/* 双向链表API */

#define List_init(head) ({ \
    (head)->prev = (head); \
    (head)->next = (head); \
})

#define List_insert(pos, node) ({ \
    (pos)->next->prev = (node);   \
    (node)->next = (pos)->next;   \
    (pos)->next = (node);         \
    (node)->prev = (pos);         \
})

#define List_push_back(head, node) (((head) != NULL) && ({                     \
                                        class Node *const tail = (head)->prev; \
                                        Node_insert(tail, (node));             \
                                    }))

#define List_remove(node) ({           \
    (node)->prev->next = (node)->next; \
    (node)->next->prev = (node)->prev; \
    (node)->prev = NULL;               \
    (node)->next = NULL;               \
})

/* 结束 */

struct StackList {
    struct SListHead *head;
};

#define StackList_init(stack_list) \
    do {                           \
        (stack_list).head = NULL;  \
    } while (0)

#define StackList_front(stack_list) ((stack_list).head)

#define StackList_isEmpty(stack_list) ((stack_list).head == NULL)

#define StackList_push(stack_list, node)  \
    do {                                  \
        (node)->next = (stack_list).head; \
        (stack_list).head = (node);       \
    } while (0)

#define StackList_pop(stack_list)                    \
    do {                                             \
        (stack_list).head = (stack_list).head->next; \
    } while (0)

struct QueueList {
    struct SListHead head;
    struct SListHead *tail;
};

#define QueueList_init(queue_list)                \
    do {                                          \
        (queue_list).tail = &((queue_list).head); \
    } while (0)

#define QueueList_front(queue_list) ((queue_list).head.next)

#define QueueList_back(queue_list) ((queue_list).tail)

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

#endif /* _ZHIYEC_LIST_H */
