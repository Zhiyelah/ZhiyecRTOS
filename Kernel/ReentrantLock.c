#include "ReentrantLock.h"

void ReentrantLock_init(struct ReentrantLock *const lock) {
    Mutex_init(&(lock->mutex));
    lock->state = 0;
}

void ReentrantLock_lock(struct ReentrantLock *const lock) {
    Mutex_lock(&(lock->mutex));

    ++(lock->state);
}

bool ReentrantLock_tryLock(struct ReentrantLock *const lock, const Tick_t timeout) {
    if (Mutex_tryLock(&(lock->mutex), timeout)) {
        ++(lock->state);

        return true;
    }
    return false;
}

void ReentrantLock_unlock(struct ReentrantLock *const lock) {
    --(lock->state);

    if (lock->state == 0) {
        Mutex_unlock(&(lock->mutex));
    }
}
