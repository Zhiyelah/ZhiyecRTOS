#include <zhiyec/assert.h>
#include <zhiyec/mutex.h>
#include <zhiyec/reentrant_lock.h>
#include <zhiyec/task.h>

struct ReentrantLock {
    /* 基于互斥锁 */
    struct Mutex *mutex;
    /* 锁计数器 */
    volatile int state;
};

static_assert(ReentrantLock_byte == sizeof(struct ReentrantLock) + Mutex_byte, "size mismatch");

struct ReentrantLock *ReentrantLock_init(void *const lock_mem) {
    if (!lock_mem) {
        return NULL;
    }

    struct ReentrantLock *lock = (struct ReentrantLock *)lock_mem;
    lock->state = 0;

    /* 获取为互斥锁分配的内存 */
    void *mutex_mem = lock + 1;

    lock->mutex = Mutex_init(mutex_mem);
    return lock;
}

void ReentrantLock_lock(struct ReentrantLock *const lock) {
    if (Mutex_getOwner(lock->mutex) != Task_currentTask()) {
        Mutex_lock(lock->mutex);
    }

    if (Mutex_getOwner(lock->mutex) == Task_currentTask()) {
        ++(lock->state);
    }
}

bool ReentrantLock_tryLock(struct ReentrantLock *const lock, const tick_t timeout) {
    if (Mutex_tryLock(lock->mutex, timeout)) {
        ++(lock->state);

        return true;
    }
    return false;
}

void ReentrantLock_unlock(struct ReentrantLock *const lock) {
    if (Mutex_getOwner(lock->mutex) == Task_currentTask()) {
        --(lock->state);
    }

    if (lock->state == 0) {
        Mutex_unlock(lock->mutex);
    }
}
