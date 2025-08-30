#include "Mutex.h"
#include <stddef.h>

extern const struct TaskStruct *const volatile current_task;

void Mutex_init(struct Mutex *const mutex) {
    Semaphore_initBinary(&(mutex->sem));
    mutex->owner = NULL;
}

void Mutex_lock(struct Mutex *const mutex) {
    Semaphore_acquire(&(mutex->sem));

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
    if (current_task == mutex->owner) {
        mutex->owner = NULL;
        Semaphore_release(&(mutex->sem));
    }
}
