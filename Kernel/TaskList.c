#include "TaskList.h"
#include "Config.h"
#include <stddef.h>

#define TASK_MAX_NUM (CONFIG_TASK_MAX_NUM)

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
#include "Memory.h"
#else
/* 任务列表 */
static struct TaskListNode task_list[TASK_MAX_NUM];
/* 空闲节点链表 */
static struct TaskListNode *task_list_free_head = NULL;
/* 列表已初始化 */
static bool task_list_is_initialized = false;
#endif

/* 活跃任务列表 */
static struct TaskListNode active_task_list;
#define active_task_list_head ((struct TaskListNode *)&active_task_list)
static struct TaskListNode *active_task_list_tail = active_task_list_head;

/* 实时任务列表 */
static struct TaskListNode realtime_task_list;
#define realtime_task_list_head ((struct TaskListNode *)&realtime_task_list)
static struct TaskListNode *realtime_task_list_tail = realtime_task_list_head;

/**
 * @return 任务列表是否为空
 */
#define TaskList_isActiveListEmpty() (active_task_list_head == active_task_list_tail)
#define TaskList_isRealTimeListEmpty() (realtime_task_list_head == realtime_task_list_tail)

/**
 * 内置特殊列表
 */
/* 阻塞任务列表 */
static struct TaskListNode *blocked_task_list_head = NULL;
/* 等待删除任务列表 */
static struct TaskListNode *to_delete_task_list_head = NULL;

/**
 * head节点不存储数据
 * head->next是当前运行的任务
 */

/* 初始化列表 */
#if (!USE_DYNAMIC_MEMORY_ALLOCATION)
static __forceinline void TaskList_init() {
    /* 初始化空闲链表 */
    for (size_t i = 0; i < TASK_MAX_NUM - 1; ++i) {
        task_list[i].next = &task_list[i + 1];
    }
    task_list[TASK_MAX_NUM - 1].next = NULL;

    task_list_free_head = task_list;
}
#endif

/* 将节点添加到活跃列表尾部 */
void TaskList_pushBack(const enum TaskListType type, struct TaskListNode *const node) {
    struct TaskListNode **list_tail = NULL;
    if (type == ACTIVE_TASK_LIST) {
        list_tail = &active_task_list_tail;
    } else if (type == REALTIME_TASK_LIST) {
        list_tail = &realtime_task_list_tail;
    } else {
        return;
    }

    node->next = NULL;
    (*list_tail)->next = node;
    *list_tail = (*list_tail)->next;
}

/* 将活跃列表的头节点移除并返回它 */
struct TaskListNode *TaskList_pop(const enum TaskListType type) {
    struct TaskListNode *list_head = NULL;
    struct TaskListNode **list_tail = NULL;
    if (type == ACTIVE_TASK_LIST) {
        list_head = active_task_list_head;
        list_tail = &active_task_list_tail;
    } else if (type == REALTIME_TASK_LIST) {
        list_head = realtime_task_list_head;
        list_tail = &realtime_task_list_tail;
    } else {
        return NULL;
    }

    if (list_head == *list_tail) {
        return NULL;
    }

    struct TaskListNode *const front_node = list_head->next;

    /* 从活跃列表中移除 */
    list_head->next = front_node->next;
    /* 回退尾指针 */
    if (front_node == *list_tail) {
        *list_tail = list_head;
    }

    front_node->next = NULL;

    return front_node;
}

/* 将节点添加到特殊列表头部 */
void TaskList_pushSpecialList(struct TaskListNode **const list_head, struct TaskListNode *const node) {
    node->next = *list_head;
    *list_head = node;
}

/* 通过枚举量将节点添加到特殊列表头部 */
void TaskList_pushSpecialListByEnum(const enum FunctionalListType type, struct TaskListNode *const node) {
    struct TaskListNode **list_head = NULL;
    if (type == BLOCKED_TASK_LIST) {
        list_head = &blocked_task_list_head;
    } else if (type == TO_DELETE_TASK_LIST) {
        list_head = &to_delete_task_list_head;
    } else {
        return;
    }

    TaskList_pushSpecialList(list_head, node);
}

/* 将特殊列表的头节点移除并返回 */
struct TaskListNode *TaskList_popSpecialList(struct TaskListNode **const list_head) {
    if ((list_head == NULL) || (*list_head == NULL)) {
        return NULL;
    }

    struct TaskListNode *const node = *list_head;
    *list_head = (*list_head)->next;
    node->next = NULL;
    return node;
}

/* 通过枚举量将特殊列表的节点移除 */
struct TaskListNode *TaskList_popSpecialListByEnum(const enum FunctionalListType type) {
    struct TaskListNode **list_head = NULL;
    if (type == BLOCKED_TASK_LIST) {
        list_head = &blocked_task_list_head;
    } else if (type == TO_DELETE_TASK_LIST) {
        list_head = &to_delete_task_list_head;
    }

    return TaskList_popSpecialList(list_head);
}

/* 添加新任务到对应列表尾部 */
bool TaskList_addNewTask(struct TaskStruct *const task) {
#if (!USE_DYNAMIC_MEMORY_ALLOCATION)
    if (!task_list_is_initialized) {
        task_list_is_initialized = true;
        TaskList_init();
    }
#endif

    struct TaskListNode *new_node;

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    new_node = Memory_alloc(sizeof(struct TaskListNode));
    if (new_node == NULL) {
        return false;
    }
#else
    if (task_list_free_head == NULL) {
        return false; /* 没有可用节点 */
    }

    new_node = task_list_free_head;

    /* 从空闲链表移除 */
    task_list_free_head = task_list_free_head->next;
#endif

    new_node->task = task;

    if (task->type == COMMON_TASK) {
        TaskList_pushBack(ACTIVE_TASK_LIST, new_node);
    } else if (task->type == REALTIME_TASK) {
        TaskList_pushBack(REALTIME_TASK_LIST, new_node);
    } else {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
        Memory_free(new_node);
#else
        /* 将节点放回空闲节点列表 */
        new_node->next = task_list_free_head;
        task_list_free_head = new_node;
#endif
        return false;
    }

    return true;
}

/* 从任务列表中释放任务节点 */
void TaskList_freeTask(struct TaskListNode *const node) {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    Memory_free(node);
#else
    /* 添加到空闲节点链表 */
    node->next = task_list_free_head;
    task_list_free_head = node;
#endif
}

/* 有实时任务 */
bool TaskList_hasRealTimeTask() {
    return !TaskList_isRealTimeListEmpty();
}

/* 获取第一个实时任务 */
struct TaskStruct *TaskList_getFrontRealTimeTask() {
    if (TaskList_isRealTimeListEmpty()) {
        return NULL;
    }

    return realtime_task_list_head->next->task;
}

/* 有活跃任务 */
bool TaskList_hasActiveTask() {
    return !TaskList_isActiveListEmpty();
}

/* 获取第一个活跃任务 */
struct TaskStruct *TaskList_getFrontActiveTask() {
    if (TaskList_isActiveListEmpty()) {
        return NULL;
    }

    return active_task_list_head->next->task;
}

/* 有阻塞任务 */
bool TaskList_hasBlockedTask() {
    return blocked_task_list_head != NULL;
}

/* 将任务插入阻塞列表 */
void TaskList_insertBlockedList(struct TaskListNode *const front_node) {
    if (blocked_task_list_head == NULL) {
        blocked_task_list_head = front_node;
    } else {
        /* 按时间升序插入到阻塞列表 */
        struct TaskListNode *current_node = blocked_task_list_head;
        struct TaskListNode *prev_node = NULL;
        while (current_node != NULL) {
            if (Tick_after(current_node->task->resume_time, front_node->task->resume_time)) {
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

        /* 比阻塞列表所有元素时间都大, 添加到阻塞列表末尾 */
        prev_node->next = front_node;
    }
}

/* 获取第一个阻塞任务的时间 */
Tick_t TaskList_getFrontBlockedTaskTime() {
    return blocked_task_list_head->task->resume_time;
}
