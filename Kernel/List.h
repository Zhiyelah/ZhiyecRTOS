#ifndef _List_h
#define _List_h

/**
 * @brief 双向链表
 */
struct ListNode {
    struct ListNode *prev;
    struct ListNode *next;
};

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

#endif /* _List_h */
