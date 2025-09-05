#include "Mutex.h"
#include "TaskList.h"
#include <stddef.h>

extern const struct TaskStruct *const volatile current_task;

void Mutex_init(struct Mutex *const mutex) {
    Semaphore_initBinary(&(mutex->sem));
    mutex->owner = NULL;
}

void Mutex_lock(struct Mutex *const mutex) {
    Semaphore_acquire(&(mutex->sem));

    Task_atomic({
        TaskList_insertFront(REALTIME_TASK, TaskList_removeFront(current_task->type));
    });

    mutex->owner = current_task;
}

bool Mutex_tryLock(struct Mutex *const mutex, const Tick_t timeout) {
    if (Semaphore_tryAcquire(&(mutex->sem), timeout)) {
        mutex->owner = current_task;

        return true;
    }
    return false;
}

void Mutex_unlock(struct Mutex *const mutex) {
    if (mutex->owner == current_task) {
        Task_atomic({
            TaskList_insertFront(current_task->type, TaskList_removeFront(REALTIME_TASK));
        });

        mutex->owner = NULL;

        Semaphore_release(&(mutex->sem));
    }
}
