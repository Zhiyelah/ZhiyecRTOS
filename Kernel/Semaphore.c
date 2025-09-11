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
void Semaphore_acquire(struct Semaphore *const sem) {
    if (sem == NULL) {
        return;
    }

    bool is_suspended = false;

    Task_atomic({
        --(sem->state);

        /* 没有信号量, 进入阻塞 */
        if (sem->state < 0) {
            extern const struct TaskStruct *const volatile current_task;
            struct SListHead *const front_node = TaskList_removeFront(current_task->type);

            if (front_node) {
                QueueList_push(sem->tasks_waiting_to_acquire, front_node);
                is_suspended = true;
            }
        }
    });

    if (is_suspended) {
        Task_yield();
    }
}

/* 尝试获得信号量, 超时后返回 */
bool Semaphore_tryAcquire(struct Semaphore *const sem, Tick_t timeout) {
    if (sem == NULL) {
        return false;
    }

    Tick_t current_tick = Tick_currentTicks();

    Task_atomic({
        --(sem->state);
    });

    /* 超时处理 */
    while (sem->state < 0) {

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

    return true;
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

    if (Task_needSwitch()) {
        Task_yield();
    }
}
