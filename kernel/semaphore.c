#include <stddef.h>
#include <zhiyec/assert.h>
#include <zhiyec/atomic.h>
#include <zhiyec/compiler.h>
#include <zhiyec/list.h>
#include <zhiyec/semaphore.h>
#include <zhiyec/task_list.h>

struct semaphore {
    /* 信号量状态 */
    volatile int state;
    /* 最大计数值 */
    int max_value;
    /* 等待获得信号量的任务 */
    struct queue_list tasks_waiting_to_acquire;
};

static_assert(SEMAPHORE_BYTE == sizeof(struct semaphore), "size mismatch");

/* 初始化为计数值为布尔类型的信号量 */
struct semaphore *semaphore_init_binary(void *const sem_mem) {
    assert(sem_mem != NULL);

    struct semaphore *sem = (struct semaphore *)sem_mem;

    sem->state = 1;
    sem->max_value = 1;
    queue_list_init(sem->tasks_waiting_to_acquire);

    return sem;
}

/* 初始化为计数值为整型的信号量 */
struct semaphore *semaphore_init_counting(void *const sem_mem,
                                          const int max_value, const unsigned int init_value) {
    assert(sem_mem != NULL);

    struct semaphore *sem = (struct semaphore *)sem_mem;

    sem->state = init_value;
    sem->max_value = max_value;
    queue_list_init(sem->tasks_waiting_to_acquire);

    return sem;
}

/* 获得信号量 */
void semaphore_acquire(struct semaphore *const sem) {
    assert(sem != NULL);

    bool need_yield = false;

    task_suspend_all();

    atomic({
        --(sem->state);
    });

    if (sem->state < 0) {
        /* 进入阻塞 */
        struct slist_head *const front_node = tasklist_remove_front(task_get_priority(task_get_current()));

        if (front_node) {
            queue_list_push(sem->tasks_waiting_to_acquire, front_node);
        }

        need_yield = true;
    }

    task_resume_all();

    if (need_yield) {
        task_yield();
    }

    DMB();
}

/* 尝试获得信号量, 超时后返回 */
bool semaphore_try_acquire(struct semaphore *const sem, tick_t timeout) {
    assert(sem != NULL);

    tick_t current_tick = tick_get_current();

    /* 超时处理 */
    while (true) {
        if (sem->state - 1 >= 0) {
            atomic({
                --(sem->state);
            });
            break;
        }

        if (timeout > 0) {
            if (!tick_after(tick_get_current(), current_tick + 1)) {
                task_yield();
                continue;
            }

            current_tick = tick_get_current();
            --timeout;
        } else {
            return false;
        }
    }

    DMB();
    return true;
}

/* 释放信号量 */
void semaphore_release(struct semaphore *const sem) {
    assert(sem != NULL);

    task_suspend_all();

    atomic({
        if (sem->state < sem->max_value) {
            ++(sem->state);
        }
    });

    if (sem->state <= 0) {

        if (!queue_list_is_empty(sem->tasks_waiting_to_acquire)) {
            /* 唤醒一个等待获得信号量的任务 */
            struct slist_head *const node = queue_list_front(sem->tasks_waiting_to_acquire);
            queue_list_pop(sem->tasks_waiting_to_acquire);

            tasklist_append(task_get_priority(task_get_from_node(node)), node);
        }
    }

    task_resume_all();

    DMB();
}

/* 中断函数中获取信号量 */
bool semaphore_acquire_from_isr(struct semaphore *const sem) {
    assert(sem != NULL);

    bool return_value = false;

    uint32_t prev_basepri = irq_disable_from_isr();

    if (sem->state - 1 >= 0) {
        --(sem->state);
        return_value = true;
    }

    irq_enable_from_isr(prev_basepri);

    return return_value;
}

/* 中断函数中释放信号量 */
void semaphore_release_from_isr(struct semaphore *const sem) {
    assert(sem != NULL);

    uint32_t prev_basepri = irq_disable_from_isr();

    if (sem->state < sem->max_value) {
        ++(sem->state);
    }

    irq_enable_from_isr(prev_basepri);
}
