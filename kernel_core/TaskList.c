#include "TaskList.h"
#include <stddef.h>
#include <zhiyec/Types.h>

/**
 * head节点没有容器
 * head->next是当前运行的任务
 */

/**
 * @brief 任务列表索引
 * @param index 任务类型
 */
static struct QueueList task_list[TASKTYPE_NUM];

/* 任务列表是否已初始化 */
bool TaskList_isInit() {
    return task_list->tail != NULL;
}

/* 初始化任务列表 */
void TaskList_init() {
    for (size_t i = 0; i < TASKTYPE_NUM; ++i) {
        QueueList_init(task_list[i]);
    }
}

/* 将节点添加到列表尾部 */
void TaskList_append(const enum TaskType type, struct SListHead *const node) {
    QueueList_push(task_list[type], node);
}

void TaskList_insertFront(const enum TaskType type, struct SListHead *const node) {
    struct QueueList *const list = &(task_list[type]);

    node->next = list->head.next;
    list->head.next = node;

    if (QueueList_isEmpty(*list)) {
        list->tail = node;
    }
}

/* 将列表的头节点移除并返回它 */
struct SListHead *TaskList_removeFront(const enum TaskType type) {
    struct QueueList *const list = &(task_list[type]);

    if (QueueList_isEmpty(*list)) {
        return NULL;
    }

    struct SListHead *const front_node = QueueList_front(*list);

    QueueList_pop(*list);

    return front_node;
}

/* 任务列表是否有任务 */
bool TaskList_hasTask(const enum TaskType type) {
    return !QueueList_isEmpty(task_list[type]);
}

/* 获取任务列表的第一个任务 */
struct TaskStruct *TaskList_getFrontTask(const enum TaskType type) {
    if (QueueList_isEmpty(task_list[type])) {
        return NULL;
    }

    return Task_fromTaskNode(QueueList_front(task_list[type]));
}
