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

/* 任务获取 */

bool TaskList_pushNewTask(struct TaskStruct *const task);
struct TaskStruct *TaskList_getFrontActiveTask(void);
void TaskList_moveFrontActiveTaskToBack(void);

/* 阻塞支持 */

bool TaskList_hasBlockedTask(void);
void TaskList_moveFrontActiveTaskToBlockedList(const Tick_t blocking_ticks);
Tick_t TaskList_getFrontBlockedTaskTime(void);
void TaskList_putFrontBlockedTaskToActiveListBack(void);

/* 事件支持 */

bool TaskList_hasEventTask(void);
struct TaskListNode *TaskList_getFrontEventTaskNode(void);
struct TaskStruct *TaskList_moveFrontActiveTaskToEventList(void);
void TaskList_putEventTaskToActiveListBack(struct TaskListNode *const prev_node,
                                           struct TaskListNode *const node);

/* 消息支持 */

bool TaskList_hasMessageTask(void);
struct TaskListNode *TaskList_getFrontMessageTaskNode(void);
struct TaskStruct *TaskList_moveFrontActiveTaskToMessageList(void);
void TaskList_putMessageTaskToActiveListBack(struct TaskListNode *const prev_node,
                                             struct TaskListNode *const node);

#endif /* _TaskList_h */
