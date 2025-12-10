/**
 * @file task_list.h
 * @author Zhiyelah
 * @brief 任务列表
 */

#ifndef _ZHIYEC_TASKLIST_H
#define _ZHIYEC_TASKLIST_H

#include <stdbool.h>
#include <zhiyec/list.h>
#include <zhiyec/task.h>

extern struct QueueList kernel_task_list[TASKTYPE_NUM];

/* 任务列表是否已初始化 */
static inline bool TaskList_isInit() {
    return kernel_task_list->tail != NULL;
}

/* 初始化任务列表 */
static inline void TaskList_init() {
    for (size_t i = 0; i < TASKTYPE_NUM; ++i) {
        QueueList_init(kernel_task_list[i]);
    }
}

/* 将节点添加到列表尾部 */
static inline void TaskList_append(const enum TaskType type, struct SListHead *const node) {
    QueueList_push(kernel_task_list[type], node);
}

static inline void TaskList_insertFront(const enum TaskType type, struct SListHead *const node) {
    struct QueueList *const list = &(kernel_task_list[type]);

    node->next = list->head.next;
    list->head.next = node;

    if (QueueList_isEmpty(*list)) {
        list->tail = node;
    }
}

/* 将列表的头节点移除并返回它 */
static inline struct SListHead *TaskList_removeFront(const enum TaskType type) {
    struct QueueList *const list = &(kernel_task_list[type]);

    if (QueueList_isEmpty(*list)) {
        return NULL;
    }

    struct SListHead *const front_node = QueueList_front(*list);

    QueueList_pop(*list);

    return front_node;
}

/* 任务列表是否有任务 */
static inline bool TaskList_hasTask(const enum TaskType type) {
    return !QueueList_isEmpty(kernel_task_list[type]);
}

/* 获取任务列表第一个任务 */
static inline struct TaskStruct *TaskList_getFrontTask(const enum TaskType type) {
    if (QueueList_isEmpty(kernel_task_list[type])) {
        return NULL;
    }

    return Task_fromTaskNode(QueueList_front(kernel_task_list[type]));
}

#endif /* _ZHIYEC_TASKLIST_H */
