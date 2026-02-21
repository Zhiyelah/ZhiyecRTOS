#include <../kernel/task_list.h>
#include <stddef.h>
#include <zhiyec/assert.h>
#include <zhiyec/compiler.h>
#include <zhiyec/mutex.h>
#include <zhiyec/semaphore.h>

struct Mutex {
    /* 基于信号量 */
    struct Semaphore *sem;
    /* 持有锁的任务 */
    struct TaskStruct *owner;
    /* 天花板优先级(请求该锁的任务中最高的任务优先级) */
    enum TaskPriority ceiling_priority;
};

static_assert(Mutex_byte == sizeof(struct Mutex) + Semaphore_byte, "size mismatch");

struct Mutex *Mutex_init(void *const mutex_mem, const enum TaskPriority ceiling_priority) {
    if (!mutex_mem) {
        return NULL;
    }

    struct Mutex *mutex = (struct Mutex *)mutex_mem;
    mutex->owner = NULL;
    mutex->ceiling_priority = ceiling_priority;

    /* 初始化信号量 */
    void *sem_mem = mutex + 1;
    mutex->sem = Semaphore_initBinary(sem_mem);

    return mutex;
}

struct TaskStruct *Mutex_getOwner(struct Mutex *const mutex) {
    return mutex->owner;
}

static inline void Mutex_swapPriorityWithTask(struct Mutex *const mutex) {
    const enum TaskPriority owner_priority = Task_getPriority(mutex->owner);

    Task_suspendAll();
    struct SListHead *const front_node = TaskList_removeFront(owner_priority);

    if (front_node) {
        Task_setPriority(mutex->owner, mutex->ceiling_priority);
        TaskList_append(mutex->ceiling_priority, front_node);
        mutex->ceiling_priority = owner_priority;
    }
    Task_resumeAll();
}

static always_inline void Mutex_doLock(struct Mutex *const mutex) {
    mutex->owner = Task_currentTask();
    Mutex_swapPriorityWithTask(mutex);
}

void Mutex_lock(struct Mutex *const mutex) {
    Semaphore_acquire(mutex->sem);
    Mutex_doLock(mutex);
}

bool Mutex_tryLock(struct Mutex *const mutex, const tick_t timeout) {
    if (Semaphore_tryAcquire(mutex->sem, timeout)) {
        Mutex_doLock(mutex);
        return true;
    }
    return false;
}

void Mutex_unlock(struct Mutex *const mutex) {
    if (mutex->owner == Task_currentTask()) {
        Mutex_swapPriorityWithTask(mutex);

        mutex->owner = NULL;
        Semaphore_release(mutex->sem);
    }
}
