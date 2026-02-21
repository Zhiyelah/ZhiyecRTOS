/**
 * @file task_list.h
 * @author Zhiyelah
 * @brief 任务列表
 */

#ifndef _ZHIYEC_TASKLIST_H
#define _ZHIYEC_TASKLIST_H

#include <stdbool.h>
#include <zhiyec/compiler.h>
#include <zhiyec/list.h>
#include <zhiyec/task.h>

#define HARDWARE_ACCELERATED_TASK_SWITCHING (ENABLE_HARDWARE_ACCELERATED_TASK_SWITCHING)

extern struct QueueList kernel_task_list[TASKPRIORITY_NUM];
extern uint32_t kernel_task_list_bitmap;

/* 任务列表是否已初始化 */
static always_inline bool TaskList_isInit() {
    return kernel_task_list->tail != NULL;
}

/* 初始化任务列表 */
static inline void TaskList_init() {
    for (size_t i = 0; i < TASKPRIORITY_NUM; ++i) {
        QueueList_init(kernel_task_list[i]);
    }
}

/* 将节点添加到列表尾部 */
static always_inline void TaskList_append(const enum TaskPriority priority, struct SListHead *const node) {
    QueueList_push(kernel_task_list[priority], node);

    kernel_task_list_bitmap |= 1U << priority;
}

/* 将列表的头节点移除并返回它 */
static always_inline struct SListHead *TaskList_removeFront(const enum TaskPriority priority) {
    struct QueueList *const list = &(kernel_task_list[priority]);

    if (QueueList_isEmpty(*list)) {
        return NULL;
    }

    struct SListHead *const front_node = QueueList_front(*list);

    QueueList_pop(*list);

    if (QueueList_isEmpty(*list)) {
        kernel_task_list_bitmap &= ~(1U << priority);
    }

    return front_node;
}

/* 获取任务列表第一个任务 */
static always_inline struct TaskStruct *TaskList_getFrontTask(const enum TaskPriority priority) {
    if (QueueList_isEmpty(kernel_task_list[priority])) {
        return NULL;
    }

    return Task_fromTaskNode(QueueList_front(kernel_task_list[priority]));
}

/* 获取列表中存在任务的最高优先级 */
#if (HARDWARE_ACCELERATED_TASK_SWITCHING)
#define TaskList_getHighestPriority(priority) \
    (priority = (enum TaskPriority)(31U - __clz(kernel_task_list_bitmap)))

#else
#define TaskList_getHighestPriority(priority)                    \
    do {                                                         \
        for (enum TaskPriority i = TASKPRIORITY_NUM; i-- > 0;) { \
            if (kernel_task_list_bitmap & (1U << i)) {           \
                priority = i;                                    \
                break;                                           \
            }                                                    \
        }                                                        \
    } while (0)

#endif /* HARDWARE_ACCELERATED_TASK_SWITCHING */

#endif /* _ZHIYEC_TASKLIST_H */
