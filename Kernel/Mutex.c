#include "Mutex.h"
#include "TaskList.h"
#include <stddef.h>

extern struct TaskStruct *const volatile current_task;

void Mutex_init(struct Mutex *const mutex) {
    Semaphore_initBinary(&(mutex->sem));
    mutex->owner = NULL;
}

static inline void Mutex_lockHelper(struct Mutex *const mutex) {
    mutex->owner = current_task;
    mutex->owner_type = mutex->owner->type;

    Task_suspendAll();
    struct SListHead *front_node = TaskList_removeFront(mutex->owner->type);
    mutex->owner->type = REALTIME_TASK;
    if (front_node) {
        TaskList_insertFront(mutex->owner->type, front_node);
    }
    Task_resumeAll();
}

void Mutex_lock(struct Mutex *const mutex) {
    Semaphore_acquire(&(mutex->sem));
    Mutex_lockHelper(mutex);
}

bool Mutex_tryLock(struct Mutex *const mutex, const Tick_t timeout) {
    if (Semaphore_tryAcquire(&(mutex->sem), timeout)) {
        Mutex_lockHelper(mutex);
        return true;
    }
    return false;
}

void Mutex_unlock(struct Mutex *const mutex) {
    if (mutex->owner == current_task) {
        Task_suspendAll();
        struct SListHead *front_node = TaskList_removeFront(mutex->owner->type);
        mutex->owner->type = mutex->owner_type;
        if (front_node) {
            TaskList_insertFront(mutex->owner->type, front_node);
        }
        Task_resumeAll();

        mutex->owner = NULL;

        Semaphore_release(&(mutex->sem));
    }
}
