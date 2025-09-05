#include "Semaphore.h"
#include "Defines.h"
#include "Task.h"
#include "TaskList.h"
#include <stddef.h>

/* 初始化为计数值为布尔类型的信号量 */
void Semaphore_initBinary(struct Semaphore *const sem) {
    sem->state = 1;
    QueueList_init(sem->tasks_waiting_to_acquire);
}

/* 初始化为计数值为整型的信号量 */
void Semaphore_initCounting(struct Semaphore *const sem, const unsigned int count) {
    sem->state = count;
    QueueList_init(sem->tasks_waiting_to_acquire);
}

/* 获得信号量 */
static bool Semaphore_acquireHelper(struct Semaphore *const sem,
                                    const bool has_timeout, Tick_t timeout) {
    if (sem == NULL) {
        return false;
    }

    Tick_t current_tick = Tick_currentTicks();

    Task_beginAtomic();
    --(sem->state);
    if (sem->state >= 0) {
        Task_endAtomic();
        return true;
    }
    Task_endAtomic();

    /* 没有信号量, 进入阻塞 */
    while (sem->state < 0) {

        /* 超时处理 */
        if (has_timeout) {

            if (timeout > 0) {
                if (!Tick_after(Tick_currentTicks(),
                                current_tick + 1)) {
                    Task_yield();
                    continue;
                }

                current_tick = Tick_currentTicks();

                --timeout;
            }

            if (timeout == 0) {
                return false;
            }

            continue;
        }

        Task_atomic({
            extern const struct TaskStruct *const volatile current_task;
            struct SListHead *const front_node = TaskList_removeFront(current_task->type);

            if (front_node != NULL) {
                QueueList_push(sem->tasks_waiting_to_acquire, front_node);
            }
        });

        Task_yield();
    }

    return true;
}

/* 获得信号量 */
void Semaphore_acquire(struct Semaphore *const sem) {
    (void)Semaphore_acquireHelper(sem, false, 0);
}

/* 尝试获得信号量, 超时后返回 */
bool Semaphore_tryAcquire(struct Semaphore *const sem, const Tick_t timeout) {
    return Semaphore_acquireHelper(sem, true, timeout);
}

/* 释放信号量 */
void Semaphore_release(struct Semaphore *const sem) {
    Task_atomic({
        ++(sem->state);

        if (sem->state <= 0) {

            if (!QueueList_isEmpty(sem->tasks_waiting_to_acquire)) {
                /* 唤醒一个等待获得信号量的任务 */
                struct SListHead *const node = QueueList_front(sem->tasks_waiting_to_acquire);
                QueueList_pop(sem->tasks_waiting_to_acquire);

                TaskList_append(container_of(node, struct TaskStruct, node)->type, node);
            }
        }
    });

    Task_yield();
}
