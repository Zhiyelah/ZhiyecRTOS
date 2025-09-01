#ifndef _StackList_h
#define _StackList_h

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

#endif /* _StackList_h */
