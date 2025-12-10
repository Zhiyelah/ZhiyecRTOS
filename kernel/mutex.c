#include <../kernel/TaskList.h>
#include <stddef.h>
#include <zhiyec/Assert.h>
#include <zhiyec/Mutex.h>
#include <zhiyec/Semaphore.h>
#include <zhiyec/Task.h>

struct Mutex {
    /* 基于信号量 */
    struct Semaphore *sem;
    /* 持有锁的任务 */
    struct TaskStruct *owner;
    /* 持有锁任务的类型 */
    enum TaskType owner_type;
};

static_assert(Mutex_byte == sizeof(struct Mutex) + Semaphore_byte, "size mismatch");

struct Mutex *Mutex_init(void *const mutex_mem) {
    if (!mutex_mem) {
        return NULL;
    }

    struct Mutex *mutex = (struct Mutex *)mutex_mem;
    mutex->owner = NULL;
    mutex->owner_type = COMMON_TASK;

    /* 获取为信号量分配的内存 */
    void *sem_mem = mutex + 1;

    mutex->sem = Semaphore_initBinary(sem_mem);
    return mutex;
}

struct TaskStruct *Mutex_getOwner(struct Mutex *const mutex) {
    return mutex->owner;
}

static inline void Mutex_lockHelper(struct Mutex *const mutex) {
    mutex->owner = Task_currentTask();
    mutex->owner_type = Task_getType(mutex->owner);

    Task_suspendAll();
    struct SListHead *front_node = TaskList_removeFront(Task_getType(mutex->owner));
    Task_setType(mutex->owner, REALTIME_TASK);

    if (front_node) {
        TaskList_insertFront(Task_getType(mutex->owner), front_node);
    }
    Task_resumeAll();
}

void Mutex_lock(struct Mutex *const mutex) {
    Semaphore_acquire(mutex->sem);
    Mutex_lockHelper(mutex);
}

bool Mutex_tryLock(struct Mutex *const mutex, const tick_t timeout) {
    if (Semaphore_tryAcquire(mutex->sem, timeout)) {
        Mutex_lockHelper(mutex);
        return true;
    }
    return false;
}

void Mutex_unlock(struct Mutex *const mutex) {
    if (mutex->owner == Task_currentTask()) {
        Task_suspendAll();
        struct SListHead *front_node = TaskList_removeFront(Task_getType(mutex->owner));
        Task_setType(mutex->owner, mutex->owner_type);

        if (front_node) {
            TaskList_insertFront(Task_getType(mutex->owner), front_node);
        }
        Task_resumeAll();

        mutex->owner = NULL;

        Semaphore_release(mutex->sem);
    }
}
