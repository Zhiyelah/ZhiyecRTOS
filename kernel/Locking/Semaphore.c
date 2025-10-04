#include <../kernel/TaskList.h>
#include <stddef.h>
#include <zhiyec/Assert.h>
#include <zhiyec/List.h>
#include <zhiyec/Semaphore.h>
#include <zhiyec/Task.h>

struct Semaphore {
    /* 信号量状态 */
    volatile int state;
    /* 最大计数值 */
    int max_value;
    /* 等待获得信号量的任务 */
    struct QueueList tasks_waiting_to_acquire;
};

static_assert(Semaphore_byte == sizeof(struct Semaphore), "size mismatch");

/* 初始化为计数值为布尔类型的信号量 */
struct Semaphore *Semaphore_initBinary(void *const sem_mem) {
    if (!sem_mem) {
        return NULL;
    }

    struct Semaphore *sem = (struct Semaphore *)sem_mem;

    sem->state = 1;
    sem->max_value = 1;
    QueueList_init(sem->tasks_waiting_to_acquire);
    return sem;
}

/* 初始化为计数值为整型的信号量 */
struct Semaphore *Semaphore_initCounting(void *const sem_mem,
                                         const int max_value, const unsigned int init_value) {
    if (!sem_mem) {
        return NULL;
    }

    struct Semaphore *sem = (struct Semaphore *)sem_mem;

    sem->state = init_value;
    sem->max_value = max_value;
    QueueList_init(sem->tasks_waiting_to_acquire);
    return sem;
}

/* 获得信号量 */
void Semaphore_acquire(struct Semaphore *const sem) {
    if (sem == NULL) {
        return;
    }

    bool is_suspended = false;

    Task_atomic({
        --(sem->state);

        /* 没有信号量, 进入阻塞 */
        if (sem->state < 0) {
            struct SListHead *const front_node = TaskList_removeFront(Task_getType(Task_currentTask()));

            if (front_node) {
                QueueList_push(sem->tasks_waiting_to_acquire, front_node);
                is_suspended = true;
            }
        }
    });

    if (is_suspended) {
        Task_yield();
    }
}

/* 尝试获得信号量, 超时后返回 */
bool Semaphore_tryAcquire(struct Semaphore *const sem, tick_t timeout) {
    if (sem == NULL) {
        return false;
    }

    tick_t current_tick = Tick_currentTicks();

    Task_atomic({
        --(sem->state);
    });

    /* 超时处理 */
    while (sem->state < 0) {

        if (timeout > 0) {
            if (!Tick_after(Tick_currentTicks(),
                            current_tick + 1)) {
                Task_yield();
                continue;
            }

            current_tick = Tick_currentTicks();

            --timeout;
        }

        if (timeout == 0) {
            return false;
        }

        continue;
    }

    return true;
}

/* 释放信号量 */
void Semaphore_release(struct Semaphore *const sem) {
    Task_atomic({
        if (sem->state < sem->max_value) {
            ++(sem->state);

            if (sem->state <= 0) {

                if (!QueueList_isEmpty(sem->tasks_waiting_to_acquire)) {
                    /* 唤醒一个等待获得信号量的任务 */
                    struct SListHead *const node = QueueList_front(sem->tasks_waiting_to_acquire);
                    QueueList_pop(sem->tasks_waiting_to_acquire);

                    TaskList_append(Task_getType(Task_fromTaskNode(node)), node);
                }
            }
        }
    });

    if (Task_needSwitch()) {
        Task_yield();
    }
}
