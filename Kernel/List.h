#ifndef _List_h
#define _List_h

/* 单向链表 */
struct SListHead {
    struct SListHead *next;
};

/* 双向链表 */
struct ListHead {
    struct ListHead *prev;
    struct ListHead *next;
};

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

#endif /* _List_h */
