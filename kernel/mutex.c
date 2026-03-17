#include <stddef.h>
#include <zhiyec/assert.h>
#include <zhiyec/compiler.h>
#include <zhiyec/mutex.h>
#include <zhiyec/semaphore.h>
#include <zhiyec/task_list.h>

struct mutex {
    /* 基于信号量 */
    struct semaphore *sem;
    /* 持有锁的任务 */
    struct task_struct *owner;
    /* 天花板优先级(请求该锁的任务中最高的任务优先级) */
    enum task_priority ceiling_priority;
};

static_assert(MUTEX_BYTE == sizeof(struct mutex) + SEMAPHORE_BYTE, "size mismatch");

struct mutex *mutex_init(void *const mutex_mem, const enum task_priority ceiling_priority) {
    assert(mutex_mem != NULL);

    struct mutex *mutex = (struct mutex *)mutex_mem;
    mutex->owner = NULL;
    mutex->ceiling_priority = ceiling_priority;

    /* 初始化信号量 */
    void *sem_mem = mutex + 1;
    mutex->sem = semaphore_init_binary(sem_mem);

    return mutex;
}

struct task_struct *mutex_get_owner(struct mutex *const mutex) {
    assert(mutex != NULL);

    return mutex->owner;
}

static inline void mutex_swap_priority_with_task(struct mutex *const mutex) {
    const enum task_priority owner_priority = task_get_priority(mutex->owner);

    task_suspend_all();
    struct slist_head *const front_node = tasklist_remove_front(owner_priority);

    if (front_node) {
        task_set_priority(mutex->owner, mutex->ceiling_priority);
        tasklist_append(mutex->ceiling_priority, front_node);
        mutex->ceiling_priority = owner_priority;
    }
    task_resume_all();
}

static always_inline void mutex_do_lock(struct mutex *const mutex) {
    mutex->owner = task_get_current();
    mutex_swap_priority_with_task(mutex);
}

void mutex_lock(struct mutex *const mutex) {
    assert(mutex != NULL);

    semaphore_acquire(mutex->sem);
    mutex_do_lock(mutex);
}

bool mutex_try_lock(struct mutex *const mutex, const tick_t timeout) {
    assert(mutex != NULL);

    if (semaphore_try_acquire(mutex->sem, timeout)) {
        mutex_do_lock(mutex);
        return true;
    }
    return false;
}

void mutex_unlock(struct mutex *const mutex) {
    assert(mutex != NULL);

    /* 未持有锁的任务意外释放了锁 */
    assert(mutex->owner == task_get_current());

    mutex_swap_priority_with_task(mutex);

    mutex->owner = NULL;
    semaphore_release(mutex->sem);
}
