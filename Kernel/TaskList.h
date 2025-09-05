/**
 * @file TaskList.h
 * @author Zhiyelah
 * @brief 任务列表
 */

#ifndef _TaskList_h
#define _TaskList_h

#include "List.h"
#include "Task.h"
#include <stdbool.h>

/* 任务列表API */

bool TaskList_isInit(void);
void TaskList_init(void);
void TaskList_append(const enum TaskType type, struct SListHead *const node);
void TaskList_insertFront(const enum TaskType type, struct SListHead *const node);
struct SListHead *TaskList_removeFront(const enum TaskType type);

/* 结束 */

/* 获取任务接口 */

bool TaskList_hasTask(const enum TaskType type);
struct TaskStruct *TaskList_getFrontTask(const enum TaskType type);

/* 结束 */

#endif /* _TaskList_h */
