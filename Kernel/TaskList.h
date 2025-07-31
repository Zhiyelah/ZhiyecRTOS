/**
 * @file TaskList.h
 * @author Zhiyelah
 * @brief 任务列表
 */

#ifndef _TaskList_h
#define _TaskList_h

#include "Task.h"
#include <stdbool.h>

struct TaskListNode {
    struct TaskStruct *task;
    struct TaskListNode *next;
};

enum TaskListType {
    ACTIVE_TASK_LIST = 0U,
    REALTIME_TASK_LIST
};

enum FunctionalListType {
    BLOCKED_TASK_LIST = 0U,
    TO_DELETE_TASK_LIST
};

/* 任务列表接口 */

void TaskList_pushBack(const enum TaskListType type, struct TaskListNode *const node);
struct TaskListNode *TaskList_pop(const enum TaskListType type);
void TaskList_pushSpecialList(struct TaskListNode **const list_head, struct TaskListNode *const node);
void TaskList_pushSpecialListByEnum(const enum FunctionalListType type, struct TaskListNode *const node);
struct TaskListNode *TaskList_popSpecialList(struct TaskListNode **const list_head);
struct TaskListNode *TaskList_popSpecialListByEnum(const enum FunctionalListType type);
bool TaskList_addNewTask(struct TaskStruct *const task);
void TaskList_freeTask(struct TaskListNode *const node);

/* 实时任务接口 */

bool TaskList_hasRealTimeTask(void);
struct TaskStruct *TaskList_getFrontRealTimeTask(void);

/* 活跃任务接口 */

bool TaskList_hasActiveTask(void);
struct TaskStruct *TaskList_getFrontActiveTask(void);

/* 阻塞支持 */

bool TaskList_hasBlockedTask(void);
void TaskList_insertBlockedList(struct TaskListNode *const front_node);
Tick_t TaskList_getFrontBlockedTaskTime(void);

#endif /* _TaskList_h */
