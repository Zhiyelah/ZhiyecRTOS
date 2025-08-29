#include "Semaphore.h"
#include "Defines.h"
#include "Task.h"
#include <stddef.h>

/* 初始化为计数值为布尔类型的信号量 */
void Semaphore_initBinary(struct Semaphore *const sem) {
    sem->state = 1;
    sem->timeout = 0U;
    sem->tasks_waiting_to_acquire = NULL;
}

/* 初始化为计数值为整型的信号量 */
void Semaphore_initCounting(struct Semaphore *const sem, const unsigned int count) {
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

        extern const struct TaskStruct *const volatile current_task;
        struct TaskListNode *const front_node = TaskList_removeFront(current_task->type);

        if (front_node == NULL) {
            Task_resumeScheduling();
            return false;
        }

        TaskList_push(sem->tasks_waiting_to_acquire, front_node);

        Task_resumeScheduling();

        Task_yield();
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
        struct TaskListNode *const node = sem->tasks_waiting_to_acquire;
        TaskList_pop(sem->tasks_waiting_to_acquire);
        if (node != NULL) {
            TaskList_append(container_of(node, struct TaskStruct, node)->type, node);
        }

        Task_resumeScheduling();
    }
}
