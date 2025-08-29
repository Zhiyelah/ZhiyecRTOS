#include "TaskList.h"
#include "Defines.h"
#include <stddef.h>

/* 普通任务列表 */
static struct TaskListNode common_task_list;

/* 实时任务列表 */
static struct TaskListNode realtime_task_list;

/* 获取列表头节点 */
#define TaskList_getHead(list) ((struct TaskListNode *)&(list))

/**
 * head节点没有容器
 * head->next是当前运行的任务
 */

/* 任务列表索引 */
static struct {
    struct TaskListNode *const head;
    struct TaskListNode *tail;
} task_list[TASKTYPE_NUM] = {
    /* 普通任务的链表指针 */ {
        .head = TaskList_getHead(common_task_list),
        .tail = TaskList_getHead(common_task_list),
    },
    /* 实时任务的链表指针 */ {
        .head = TaskList_getHead(realtime_task_list),
        .tail = TaskList_getHead(realtime_task_list),
    },
};

/* 任务列表是否为空 */
#define TaskList_isEmpty(task_type) (task_list[(task_type)].head == task_list[(task_type)].tail)

/* 将节点添加到列表尾部 */
void TaskList_append(const enum TaskType type, struct TaskListNode *const node) {
    struct TaskListNode **const list_tail = &(task_list[type].tail);

    node->next = NULL;
    (*list_tail)->next = node;
    *list_tail = (*list_tail)->next;
}

/* 将列表的头节点移除并返回它 */
struct TaskListNode *TaskList_removeFront(const enum TaskType type) {
    struct TaskListNode *const list_head = task_list[type].head;
    struct TaskListNode **const list_tail = &(task_list[type].tail);

    if (list_head == *list_tail) {
        return NULL;
    }

    struct TaskListNode *const front_node = list_head->next;

    /* 从列表中移除 */
    list_head->next = front_node->next;
    /* 回退尾指针 */
    if (front_node == *list_tail) {
        *list_tail = list_head;
    }

    front_node->next = NULL;

    return front_node;
}

/* 有实时任务 */
bool TaskList_hasRealTimeTask() {
    return !TaskList_isEmpty(REALTIME_TASK);
}

/* 获取第一个实时任务 */
struct TaskStruct *TaskList_getFrontRealTimeTask() {
    if (TaskList_isEmpty(REALTIME_TASK)) {
        return NULL;
    }

    return container_of(TaskList_getHead(realtime_task_list)->next, struct TaskStruct, node);
}

/* 有普通任务 */
bool TaskList_hasCommonTask() {
    return !TaskList_isEmpty(COMMON_TASK);
}

/* 获取第一个普通任务 */
struct TaskStruct *TaskList_getFrontCommonTask() {
    if (TaskList_isEmpty(COMMON_TASK)) {
        return NULL;
    }

    return container_of(TaskList_getHead(common_task_list)->next, struct TaskStruct, node);
}
