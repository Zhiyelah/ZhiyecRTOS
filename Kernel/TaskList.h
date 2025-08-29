/**
 * @file TaskList.h
 * @author Zhiyelah
 * @brief 任务列表
 */

#ifndef _TaskList_h
#define _TaskList_h

#include "Task.h"
#include <stdbool.h>

/* 无哨兵节点的链表管理 */

#define TaskList_push(list_head, node) \
    do {                               \
        (node)->next = (list_head);    \
        (list_head) = (node);          \
    } while (0)

#define TaskList_pop(list_head) ((list_head != NULL) && ({            \
                                     (list_head) = (list_head)->next; \
                                 }))

/* 结束 */

/* 有哨兵节点的链表管理 */

void TaskList_append(const enum TaskType type, struct TaskListNode *const node);
struct TaskListNode *TaskList_removeFront(const enum TaskType type);

/* 结束 */

/* 获取任务接口 */

bool TaskList_hasRealTimeTask(void);
struct TaskStruct *TaskList_getFrontRealTimeTask(void);

bool TaskList_hasCommonTask(void);
struct TaskStruct *TaskList_getFrontCommonTask(void);

/* 结束 */

#endif /* _TaskList_h */
