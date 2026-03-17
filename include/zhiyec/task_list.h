/**
 * @file task_list.h
 * @author Zhiyelah
 * @brief 任务列表
 */

#ifndef _ZHIYEC_TASKLIST_H
#define _ZHIYEC_TASKLIST_H

#include <stdbool.h>
#include <zhiyec/assert.h>
#include <zhiyec/compiler.h>
#include <zhiyec/list.h>
#include <zhiyec/task.h>

#define HARDWARE_ACCELERATED_TASK_SWITCHING (ENABLE_HARDWARE_ACCELERATED_TASK_SWITCHING)

extern struct queue_list kernel_task_list[TASKPRIORITY_NUM];
extern uint32_t kernel_task_list_bitmap;

/* 任务列表是否已初始化 */
static always_inline bool tasklist_is_init() {
    return kernel_task_list->tail != NULL;
}

/* 初始化任务列表 */
static inline void tasklist_init() {
    for (size_t i = 0; i < TASKPRIORITY_NUM; ++i) {
        queue_list_init(kernel_task_list[i]);
    }
}

/* 将节点添加到列表尾部 */
static always_inline void tasklist_append(const enum task_priority priority, struct slist_head *const node) {
    assert(node != NULL);

    queue_list_push(kernel_task_list[priority], node);

    kernel_task_list_bitmap |= 1U << priority;
}

/* 将列表的头节点移除并返回它 */
static always_inline struct slist_head *tasklist_remove_front(const enum task_priority priority) {
    struct queue_list *const list = &(kernel_task_list[priority]);

    if (queue_list_is_empty(*list)) {
        return NULL;
    }

    struct slist_head *const front_node = queue_list_front(*list);

    queue_list_pop(*list);

    if (queue_list_is_empty(*list)) {
        kernel_task_list_bitmap &= ~(1U << priority);
    }

    return front_node;
}

/* 获取任务列表第一个任务 */
static always_inline struct task_struct *tasklist_get_front_task(const enum task_priority priority) {
    if (queue_list_is_empty(kernel_task_list[priority])) {
        return NULL;
    }

    return task_get_from_node(queue_list_front(kernel_task_list[priority]));
}

/* 获取列表中存在任务的最高优先级 */
#if (HARDWARE_ACCELERATED_TASK_SWITCHING)
#define tasklist_get_highest_priority(priority) \
    (priority = (enum task_priority)(31U - __clz(kernel_task_list_bitmap)))

#else
#define tasklist_get_highest_priority(priority)                  \
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
