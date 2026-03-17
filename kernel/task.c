#include <asm/systick.h>
#include <stddef.h>
#include <zhiyec/atomic.h>
#include <zhiyec/hook.h>
#include <zhiyec/task.h>
#include <zhiyec/task_list.h>
#include <zhiyec/tick.h>

#define TASK_MAX_NUM (CONFIG_TASK_MAX_NUM)
#define DYNAMIC_MEMORY_ALLOCATION (USE_DYNAMIC_MEMORY_ALLOCATION)

/* 调度器状态 */
static volatile int task_suspended_count = 0;
struct task_struct *volatile kernel_current_task = NULL;

/* 阻塞任务列表 */
static struct stack_list blocked_task_list[2];
static struct stack_list *blocked_task_list_curr_cycle = &blocked_task_list[0];
static struct stack_list *blocked_task_list_next_cycle = &blocked_task_list[1];
static bool blocked_task_list_ready_switch_next_cycle = false;

/* 等待删除任务列表 */
static struct stack_list to_delete_task_list;

static struct task_struct *task_new_task_struct(stack_t *const stack, stack_t *const top_of_stack);
static void task_delete_task_struct(struct task_struct *const task);
void task_return_handler(void);

/* 创建任务 */
bool task_create(void (*const fn)(void *), void *const arg, stack_t *const stack, const stack_t stack_size,
                 const struct task_attribute *const attr) {
    assert(fn != NULL);
    assert(stack != NULL);
    assert(attr != NULL);

    stack_t *top_of_stack = &stack[stack_size - (stack_t)1U];

    /* 内存对齐 */
    top_of_stack = (stack_t *)(((stack_t)top_of_stack) & ~((stack_t)(BYTE_ALIGNMENT - 1)));

    /* 初始化任务栈 */
    top_of_stack = port_init_task_stack(top_of_stack, fn, arg,
                                        task_return_handler);

    struct task_struct *task = NULL;

    /* 创建任务对象 */
    atomic({
        task = task_new_task_struct(stack, top_of_stack);
    });

    if (task == NULL) {
        if (attr->destroy) {
            attr->destroy(stack);
        }

        return false;
    }

    task->attr.priority = attr->priority;
    task->attr.sched_method = attr->sched_method;
    task->attr.destroy = attr->destroy;

    /* 添加到任务列表 */
    atomic({
        if (!tasklist_is_init()) {
            tasklist_init();
        }

        tasklist_append(task->attr.priority, &(task->task_node));

        if (kernel_current_task == NULL) {
            kernel_current_task = task;
        }
    });

    return true;
}

/* 将任务节点插入阻塞列表 */
static inline void task_insert_blocked_list(struct stack_list *blocked_list,
                                            struct slist_head *const node) {
    if (stack_list_is_empty(*blocked_list)) {
        stack_list_front(*blocked_list) = node;
        return;
    }

    /* 按时间升序插入到阻塞列表 */
    struct slist_head *current_node = stack_list_front(*blocked_list);
    struct slist_head *prev_node = NULL;
    while (current_node != NULL) {
        if (tick_after(task_get_from_node(current_node)->resume_time,
                       task_get_from_node(node)->resume_time)) {
            node->next = current_node;
            if (prev_node != NULL) {
                prev_node->next = node;
            } else {
                stack_list_front(*blocked_list) = node;
            }
            return;
        }

        prev_node = current_node;
        current_node = current_node->next;
    }

    /* 比阻塞列表所有元素时间都大, 添加到阻塞列表末尾 */
    prev_node->next = node;
}

/* 将当前任务阻塞 */
static inline void task_do_sleep(const tick_t resume_time) {
    /* 如果任务未超时, 添加到阻塞任务列表 */
    if (!tick_after(tick_get_current(), resume_time)) {
        atomic({
            struct slist_head *const front_node = tasklist_remove_front(kernel_current_task->attr.priority);

            if (front_node != NULL) {
                task_get_from_node(front_node)->resume_time = resume_time;

                const tick_t current_time = tick_get_current();

                /* 将下一个tick周期的任务添加到另一个列表 */
                if (resume_time < current_time && !tick_after(current_time, resume_time)) {
                    task_insert_blocked_list(blocked_task_list_next_cycle, front_node);
                } else {
                    task_insert_blocked_list(blocked_task_list_curr_cycle, front_node);
                }
            }
        });
    }

    task_yield();
}

/* 添加任务到阻塞列表 */
void task_sleep(const tick_t ticks) {
    /* 如果ticks的值为0, 则仅让出CPU */
    task_do_sleep(tick_get_current() + ticks - 1U);
}

/* 使任务周期性执行 */
void task_sleep_until(tick_t *const prev_wake_time, const tick_t interval) {
    const tick_t resume_time = (*prev_wake_time) + interval - 1U;
    task_do_sleep(resume_time);
    *prev_wake_time = resume_time;
}

/* 添加任务到删除列表 */
void task_delete_later() {
    atomic({
        struct slist_head *const front_node = tasklist_remove_front(kernel_current_task->attr.priority);

        if (front_node != NULL) {
            stack_list_push(to_delete_task_list, front_node);
        }
    });

    task_yield();
}

/* 暂停所有任务 */
void task_suspend_all() {
    atomic({
        ++task_suspended_count;
    });

    DMB();
}

/* 恢复所有任务 */
void task_resume_all() {
    atomic({
        --task_suspended_count;
    });
}

#define KERNEL_IDLE_TASK_STACK_SIZE 64
static stack_t kernel_idle_task_stack[KERNEL_IDLE_TASK_STACK_SIZE];
/* 空闲任务(最低优先级) */
static void kernel_idle_task(void *arg) {
    (void)arg;

    for (;;) {

    #ifdef hook_idle_task_running
        hook_idle_task_running();
    #endif /* hook_idle_task_running */

        atomic({
            if (!stack_list_is_empty(to_delete_task_list)) {
                struct slist_head *to_delete_node = stack_list_front(to_delete_task_list);
                stack_list_pop(to_delete_task_list);

                struct task_struct *const task = task_get_from_node(to_delete_node);

                /* 当to_delete_node非空时task不会为空 */

                /* 调用销毁回调 */
                if (task->attr.destroy) {
                    task->attr.destroy(task->stack);
                }

                /* 删除对象 */
                task_delete_task_struct(task);
            }
        });

        enum task_priority priority;
        tasklist_get_highest_priority(priority);
        if (priority > TASKPRIO_IDLE) {
            task_yield();
        }
    }
}

/* 初始化调度器, 并开始执行任务 */
int task_schedule() {
    const struct task_attribute idle_task_attr = {
        .priority = TASKPRIO_IDLE,
    };

    /* 尝试创建空闲任务 */
    if (task_create(kernel_idle_task, NULL, kernel_idle_task_stack, KERNEL_IDLE_TASK_STACK_SIZE,
                    &idle_task_attr)) {
        /* 屏蔽中断 */
        irq_disable_without_nesting();

        /* 初始化Tick */
        systick_init();

        /* 开始执行任务 */
        port_start_first_task();

        /* 正常不会返回 */
        return 0;
    }

    return -1;
}

/* 创建和删除任务对象 */
#if (DYNAMIC_MEMORY_ALLOCATION)
#include <zhiyec/memory.h>

static struct task_struct *task_new_task_struct(stack_t *const stack,
                                                stack_t *const top_of_stack) {
    struct task_struct *new_task = memory_alloc(sizeof(struct task_struct));
    if (new_task != NULL) {
        new_task->stack = stack;
        new_task->top_of_stack = top_of_stack;
        new_task->resume_time = 0U;
    }

    return new_task;
}

static void task_delete_task_struct(struct task_struct *const task) {
    memory_free(task);
}

#else
static struct task_struct task_structures[TASK_MAX_NUM];
static size_t task_structures_pos = 0U;

static struct task_struct *task_new_task_struct(stack_t *const stack,
                                                stack_t *const top_of_stack) {
    const struct task_struct task = {
        .stack = stack,
        .top_of_stack = top_of_stack,
        .resume_time = 0U,
    };

    if (task_structures_pos >= TASK_MAX_NUM) {
        /* 尝试查找未使用的空间块 */
        for (size_t i = 0; i < TASK_MAX_NUM; ++i) {
            if (task_structures[i].stack == NULL) {
                task_structures[i] = task;
                return &task_structures[i];
            }
        }
        /* task_structures的空间不足 */
        return NULL;
    }

    struct task_struct *const new_task = &task_structures[task_structures_pos];
    ++task_structures_pos;
    *new_task = task;

    return new_task;
}

static void task_delete_task_struct(struct task_struct *const task) {
    for (size_t i = 0; i < TASK_MAX_NUM; ++i) {
        if (&task_structures[i] == task) {
            task_structures[i].stack = NULL;
            break;
        }
    }
}

#endif /* DYNAMIC_MEMORY_ALLOCATION */

/* 任务返回处理函数 */
void task_return_handler() {
    task_delete_later();

    for (;;) {
    }
}

/* 是否需要切换任务 */
bool task_need_switch() {
    /* 内核Tick计数 */
    ++kernel_ticks;

    /* 检测到Tick计数溢出, 准备切换到下一个Tick周期 */
    if (tick_get_current() == 0U) {
        blocked_task_list_ready_switch_next_cycle = true;
    }

    /* 检查阻塞列表任务是否超时 */
    if ((!stack_list_is_empty(*blocked_task_list_curr_cycle)) &&
        tick_after(tick_get_current(), task_get_from_node(stack_list_front(*blocked_task_list_curr_cycle))->resume_time)) {

        struct task_struct *const timeout_task = task_get_from_node(stack_list_front(*blocked_task_list_curr_cycle));
        stack_list_pop(*blocked_task_list_curr_cycle);

        tasklist_append(timeout_task->attr.priority, &(timeout_task->task_node));
    }

    /* 若当前Tick周期的任务处理完毕, 切换到下一个Tick周期的列表 */
    if (blocked_task_list_ready_switch_next_cycle &&
        stack_list_is_empty(*blocked_task_list_curr_cycle)) {
        blocked_task_list_ready_switch_next_cycle = false;

        struct stack_list *tmp = blocked_task_list_curr_cycle;
        blocked_task_list_curr_cycle = blocked_task_list_next_cycle;
        blocked_task_list_next_cycle = tmp;
    }

    /* 调度器已暂停, 不切换任务 */
    if (task_suspended_count) {
        return false;
    }

    enum task_priority priority;
    tasklist_get_highest_priority(priority);
    /* 有更高优先级的任务, 需要切换任务 */
    if (priority > kernel_current_task->attr.priority) {
        return true;
    }

    /* 同优先级之间如果为时间片轮转调度则切换任务 */
    return (kernel_current_task->attr.sched_method == TASKSCHED_RR);
}

/* 切换下一个任务 */
void task_switch_next() {
    /* 是否需要将当前任务移动到列表尾部 */
    if (tasklist_get_front_task(kernel_current_task->attr.priority) == kernel_current_task) {
        tasklist_append(kernel_current_task->attr.priority,
                        tasklist_remove_front(kernel_current_task->attr.priority));
    }

    /* 获取最高优先级的任务 */
    enum task_priority priority;
    tasklist_get_highest_priority(priority);
    kernel_current_task = tasklist_get_front_task(priority);
}
