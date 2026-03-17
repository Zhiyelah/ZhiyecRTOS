#include <zhiyec/assert.h>
#include <zhiyec/mutex.h>
#include <zhiyec/reentrant_lock.h>

struct reentrantlock {
    /* 基于互斥锁 */
    struct mutex *mutex;
    /* 锁计数器 */
    volatile int state;
};

static_assert(REENTRANTLOCK_BYTE == sizeof(struct reentrantlock) + MUTEX_BYTE, "size mismatch");

struct reentrantlock *reentrantlock_init(void *const lock_mem, const enum task_priority ceiling_priority) {
    assert(lock_mem != NULL);

    struct reentrantlock *lock = (struct reentrantlock *)lock_mem;
    lock->state = 0;

    /* 初始化互斥锁 */
    void *mutex_mem = lock + 1;
    lock->mutex = mutex_init(mutex_mem, ceiling_priority);

    return lock;
}

void reentrantlock_lock(struct reentrantlock *const lock) {
    assert(lock != NULL);

    if (mutex_get_owner(lock->mutex) != task_get_current()) {
        mutex_lock(lock->mutex);
    }

    if (mutex_get_owner(lock->mutex) == task_get_current()) {
        ++(lock->state);
    }
}

bool reentrantlock_try_lock(struct reentrantlock *const lock, const tick_t timeout) {
    assert(lock != NULL);

    if (mutex_try_lock(lock->mutex, timeout)) {
        ++(lock->state);

        return true;
    }
    return false;
}

void reentrantlock_unlock(struct reentrantlock *const lock) {
    assert(lock != NULL);

    if (mutex_get_owner(lock->mutex) == task_get_current()) {
        --(lock->state);
    }

    if (lock->state == 0) {
        mutex_unlock(lock->mutex);
    }
}
