#include "Semaphore.h"
#include "Task.h"
#include <stddef.h>

/* 计数值为布尔值的信号量 */
void Semaphore_newBinary(struct Semaphore *const sem) {
    sem->state = 1;
    sem->timeout = 0U;
    sem->tasks_waiting_to_acquire = NULL;
}

/* 整型计数信号量 */
void Semaphore_newCounting(struct Semaphore *const sem, const unsigned int count) {
    sem->state = count;
    sem->timeout = 0U;
    sem->tasks_waiting_to_acquire = NULL;
}

/* 获得信号量 */
static bool Semaphore_acquireHelper(struct Semaphore *const sem, const bool has_timeout) {
    if (sem == NULL) {
        return false;
    }

    Task_suspendScheduling();
    --(sem->state);
    Task_resumeScheduling();

    Tick_t current_tick = Tick_currentTicks();

    /* 没有信号量, 进入阻塞 */
    while (sem->state < 0) {
        /* 超时处理 */
        if (has_timeout) {
            if (!Tick_after(Tick_currentTicks(),
                            current_tick + 1)) {
                continue;
            }
            current_tick = Tick_currentTicks();

            if (sem->timeout > 0) {
                --(sem->timeout);
            }
            if (sem->timeout == 0) {
                return false;
            }
            continue;
        }

        Task_suspendScheduling();

        extern struct TaskListNode *Task_popFromTaskList();
        struct TaskListNode *const front_node = Task_popFromTaskList();

        if (front_node == NULL) {
            Task_resumeScheduling();
            return false;
        }

        TaskList_pushSpecialList(&(sem->tasks_waiting_to_acquire), front_node);

        Task_resumeScheduling();

        yield();
    }

    return true;
}

/* 获得信号量 */
bool Semaphore_acquire(struct Semaphore *const sem) {
    return Semaphore_acquireHelper(sem, false);
}

/* 尝试获得信号量, 超时后返回 */
bool Semaphore_tryAcquire(struct Semaphore *const sem, Tick_t timeout) {
    sem->timeout = timeout;
    return Semaphore_acquireHelper(sem, true);
}

/* 释放信号量 */
void Semaphore_release(struct Semaphore *const sem) {
    Task_suspendScheduling();
    ++(sem->state);
    Task_resumeScheduling();

    if (sem->state <= 0) {
        Task_suspendScheduling();

        /* 唤醒一个等待获得信号量的任务 */
        struct TaskListNode *const node = TaskList_popSpecialList(&(sem->tasks_waiting_to_acquire));
        if (node != NULL) {
            if (node->task->type == COMMON_TASK) {
                TaskList_pushBack(ACTIVE_TASK_LIST, node);
            } else if (node->task->type == REALTIME_TASK) {
                TaskList_pushBack(REALTIME_TASK_LIST, node);
            }
        }

        Task_resumeScheduling();
    }
}
