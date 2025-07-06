#include "TaskList.h"
#include "Config.h"
#include <stddef.h>

#define TASK_MAX_NUM (CONFIG_TASK_MAX_NUM)

/* 活跃任务列表 */
static struct TaskListNode task_list[TASK_MAX_NUM];
#define task_list_head ((struct TaskListNode *)task_list)
static struct TaskListNode *task_list_tail = task_list_head;

/**
 * @return 列表是否为空
 */
#define TaskList_isEmpty() (task_list_head == task_list_tail)

/* 阻塞任务列表 */
static struct TaskListNode *blocked_task_list_head = NULL;

/* 等待事件任务列表 */
static struct TaskListNode *event_task_list_head = NULL;

/* 等待消息任务列表 */
static struct TaskListNode *message_task_list_head = NULL;

/* 空闲节点链表 */
static struct TaskListNode *task_list_free_head = NULL;

/* 列表已初始化 */
static bool task_list_is_initialized = false;

/**
 * head节点不存储数据
 * head->next是当前运行的任务
 */

/* 初始化列表 */
static __forceinline void TaskList_init() {
    /* 初始化空闲链表 */
    for (size_t i = 0; i < TASK_MAX_NUM - 1; ++i) {
        task_list[i].next = &task_list[i + 1];
    }
    task_list[TASK_MAX_NUM - 1].next = NULL;

    task_list_free_head = task_list_head->next;
}

/* 将节点添加到活跃列表尾部 */
static void TaskList_push(struct TaskListNode *const node) {
    node->next = NULL;
    task_list_tail->next = node;
    task_list_tail = task_list_tail->next;
}

/* 将活跃列表的头节点移除并返回它 */
static struct TaskListNode *TaskList_pop() {
    if (TaskList_isEmpty()) {
        return NULL;
    }

    struct TaskListNode *const front_node = task_list_head->next;

    /* 从活跃列表中移除 */
    task_list_head->next = front_node->next;
    /* 回退尾指针 */
    if (front_node == task_list_tail) {
        task_list_tail = task_list_head;
    }

    front_node->next = NULL;
    /* 更新时间 */
    front_node->task->time = Tick_getCurrentTicks();

    return front_node;
}

/* 添加新任务到活跃列表尾部 */
bool TaskList_pushNewTask(struct TaskStruct *const task) {
    if (!task_list_is_initialized) {
        task_list_is_initialized = true;
        TaskList_init();
    }

    if (task_list_free_head == NULL) {
        return false; /* 没有可用节点 */
    }

    struct TaskListNode *const new_node = task_list_free_head;

    /* 从空闲链表移除 */
    task_list_free_head = task_list_free_head->next;

    new_node->task = task;
    new_node->task->time = Tick_getCurrentTicks();
    TaskList_push(new_node);

    return true;
}

/* 获取第一个活跃任务 */
struct TaskStruct *TaskList_getFrontActiveTask() {
    return task_list_head->next->task;
}

/* 将第一个活跃任务节点放到链表尾部 */
void TaskList_moveFrontActiveTaskToBack() {
    struct TaskListNode *const front_node = TaskList_pop();

    if (front_node == NULL) {
        return;
    }

    TaskList_push(front_node);
}

/* 有阻塞任务 */
bool TaskList_hasBlockedTask() {
    if (TaskList_isEmpty()) {
        return false;
    }

    return blocked_task_list_head != NULL;
}

/* 将第一个活跃任务移动到阻塞列表 */
void TaskList_moveFrontActiveTaskToBlockedList(const Tick_t blocking_ticks) {
    struct TaskListNode *const front_node = TaskList_pop();

    if (front_node == NULL) {
        return;
    }

    /* 更新阻塞时间 */
    front_node->task->time = blocking_ticks;

    if (blocked_task_list_head == NULL) {
        blocked_task_list_head = front_node;
    } else {
        /* 按时间升序插入到阻塞列表 */
        struct TaskListNode *current_node = blocked_task_list_head;
        struct TaskListNode *prev_node = NULL;
        while (current_node != NULL) {
            if (Tick_after(current_node->task->time, front_node->task->time)) {
                front_node->next = current_node;
                if (prev_node != NULL) {
                    prev_node->next = front_node;
                } else {
                    blocked_task_list_head = front_node;
                }
                return;
            }

            prev_node = current_node;
            current_node = current_node->next;
        }

        /* 比阻塞列表所有元素时间都大, 添加到阻塞列表尾 */
        prev_node->next = front_node;
    }
}

/* 获取第一个阻塞任务的时间 */
Tick_t TaskList_getFrontBlockedTaskTime() {
    return blocked_task_list_head->task->time;
}

/* 将第一个阻塞任务节点放回到活跃列表尾部 */
void TaskList_putFrontBlockedTaskToActiveListBack() {
    struct TaskListNode *const front_node = blocked_task_list_head;
    blocked_task_list_head = blocked_task_list_head->next;

    TaskList_push(front_node);
}

/* 有事件任务 */
bool TaskList_hasEventTask() {
    if (TaskList_isEmpty()) {
        return false;
    }

    return event_task_list_head != NULL;
}

/* 获取第一个事件任务节点 */
struct TaskListNode *TaskList_getFrontEventTaskNode() {
    return event_task_list_head;
}

/* 将第一个活跃任务移动到事件列表 */
struct TaskStruct *TaskList_moveFrontActiveTaskToEventList() {
    struct TaskListNode *const front_node = TaskList_pop();

    if (front_node == NULL) {
        return NULL;
    }

    if (event_task_list_head == NULL) {
        event_task_list_head = front_node;
    } else {
        front_node->next = event_task_list_head;
        event_task_list_head = front_node;
    }

    return front_node->task;
}

/* 将事件任务节点放回到活跃列表尾部 */
void TaskList_putEventTaskToActiveListBack(struct TaskListNode *const prev_node,
                                           struct TaskListNode *const node) {
    if (node == event_task_list_head) {
        event_task_list_head = event_task_list_head->next;
    } else {
        prev_node->next = node->next;
    }

    TaskList_push(node);
}

/* 有消息任务 */
bool TaskList_hasMessageTask(void) {
    if (TaskList_isEmpty()) {
        return false;
    }

    return message_task_list_head != NULL;
}

/* 获取第一个消息任务节点 */
struct TaskListNode *TaskList_getFrontMessageTaskNode() {
    return message_task_list_head;
}

/* 将第一个活跃任务移动到消息列表 */
struct TaskStruct *TaskList_moveFrontActiveTaskToMessageList() {
    struct TaskListNode *const front_node = TaskList_pop();

    if (front_node == NULL) {
        return NULL;
    }

    if (message_task_list_head == NULL) {
        message_task_list_head = front_node;
    } else {
        front_node->next = message_task_list_head;
        message_task_list_head = front_node;
    }

    return front_node->task;
}

/* 将消息任务节点放回到活跃列表尾部并返回前一个节点 */
void TaskList_putMessageTaskToActiveListBack(struct TaskListNode *const prev_node,
                                             struct TaskListNode *const node) {
    if (node == message_task_list_head) {
        message_task_list_head = message_task_list_head->next;
    } else {
        prev_node->next = node->next;
    }

    TaskList_push(node);
}
