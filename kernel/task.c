#include <../kernel/atomic.h>
#include <../kernel/hook.h>
#include <../kernel/task_list.h>
#include <stddef.h>
#include <zhiyec/task.h>
#include <zhiyec/tick.h>

#define SHIELDABLE_INTERRUPT_MAX_PRIORITY (CONFIG_SHIELDABLE_INTERRUPT_MAX_PRIORITY)
#define TASK_MAX_NUM (CONFIG_TASK_MAX_NUM)
#define DYNAMIC_MEMORY_ALLOCATION (USE_DYNAMIC_MEMORY_ALLOCATION)

/* 调度器状态 */
static volatile int task_suspended_count = 0;
struct TaskStruct *volatile kernel_current_task = NULL;

/* 阻塞任务列表 */
static struct StackList blocked_task_list[2];
static struct StackList *blocked_task_list_curr_cycle = &blocked_task_list[0];
static struct StackList *blocked_task_list_next_cycle = &blocked_task_list[1];
static bool blocked_task_list_ready_switch_next_cycle = false;

/* 等待删除任务列表 */
static struct StackList to_delete_task_list;

static struct TaskStruct *Task_newTaskStruct(stack_t *const stack, stack_t *const top_of_stack);
static void Task_deleteTaskStruct(struct TaskStruct *const task);

/* 创建任务 */
bool Task_create(void (*const fn)(void *), void *const arg, stack_t *const stack, const stack_t stack_size,
                 const struct TaskAttribute *const attr) {
    if ((fn == NULL) || (stack == NULL) || (attr == NULL)) {
        return false;
    }

    stack_t *top_of_stack = &stack[stack_size - (stack_t)1U];

    /* 内存对齐 */
    top_of_stack = (stack_t *)(((stack_t)top_of_stack) & ~((stack_t)(BYTE_ALIGNMENT - 1)));

    /* 初始化任务栈 */
    top_of_stack = InitTaskStack_Port(top_of_stack, fn, arg);

    struct TaskStruct *task = NULL;

    /* 创建任务对象 */
    atomic({
        task = Task_newTaskStruct(stack, top_of_stack);
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
        if (!TaskList_isInit()) {
            TaskList_init();
        }

        TaskList_append(task->attr.priority, &(task->task_node));

        if (kernel_current_task == NULL) {
            kernel_current_task = task;
        }
    });

    return true;
}

/* 将任务节点插入阻塞列表 */
static inline void Task_insertBlockedList(struct StackList *blocked_list,
                                          struct SListHead *const node) {
    if (StackList_isEmpty(*blocked_list)) {
        StackList_front(*blocked_list) = node;
        return;
    }

    /* 按时间升序插入到阻塞列表 */
    struct SListHead *current_node = StackList_front(*blocked_list);
    struct SListHead *prev_node = NULL;
    while (current_node != NULL) {
        if (Tick_after(Task_fromTaskNode(current_node)->resume_time,
                       Task_fromTaskNode(node)->resume_time)) {
            node->next = current_node;
            if (prev_node != NULL) {
                prev_node->next = node;
            } else {
                StackList_front(*blocked_list) = node;
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
static inline void Task_doSleep(const tick_t resume_time) {
    /* 如果任务未超时, 添加到阻塞任务列表 */
    if (!Tick_after(Tick_currentTick(), resume_time)) {
        atomic({
            struct SListHead *const front_node = TaskList_removeFront(kernel_current_task->attr.priority);

            if (front_node != NULL) {
                Task_fromTaskNode(front_node)->resume_time = resume_time;

                const tick_t current_time = Tick_currentTick();

                /* 将下一个tick周期的任务添加到另一个列表 */
                if (resume_time < current_time && !Tick_after(current_time, resume_time)) {
                    Task_insertBlockedList(blocked_task_list_next_cycle, front_node);
                } else {
                    Task_insertBlockedList(blocked_task_list_curr_cycle, front_node);
                }
            }
        });
    }

    Task_yield();
}

/* 添加任务到阻塞列表 */
void Task_sleep(const tick_t ticks) {
    /* 如果ticks的值为0, 则仅让出CPU */
    Task_doSleep(Tick_currentTick() + ticks - 1U);
}

/* 使任务周期性执行 */
void Task_sleepUntil(tick_t *const prev_wake_time, const tick_t interval) {
    const tick_t resume_time = (*prev_wake_time) + interval - 1U;
    Task_doSleep(resume_time);
    *prev_wake_time = resume_time;
}

/* 添加任务到删除列表 */
void Task_deleteLater() {
    atomic({
        struct SListHead *const front_node = TaskList_removeFront(kernel_current_task->attr.priority);

        if (front_node != NULL) {
            StackList_push(to_delete_task_list, front_node);
        }
    });

    Task_yield();
}

/* 暂停所有任务 */
void Task_suspendAll() {
    atomic({
        ++task_suspended_count;
    });

    DMB();
}

/* 恢复所有任务 */
void Task_resumeAll() {
    atomic({
        --task_suspended_count;
    });
}

#define KERNEL_IDLE_TASK_STACK_SIZE 64
static stack_t kernelIdleTask_stack[KERNEL_IDLE_TASK_STACK_SIZE];
/* 空闲任务(最低优先级) */
static void kernelIdleTask(void *arg) {
    (void)arg;

    for (;;) {

    #ifdef Hook_idleTaskRunning
        Hook_idleTaskRunning();
    #endif /* Hook_idleTaskRunning */

        atomic({
            if (!StackList_isEmpty(to_delete_task_list)) {
                struct SListHead *to_delete_node = StackList_front(to_delete_task_list);
                StackList_pop(to_delete_task_list);

                struct TaskStruct *const task = Task_fromTaskNode(to_delete_node);

                /* 当to_delete_node非空时task不会为空 */

                /* 调用销毁回调 */
                if (task->attr.destroy) {
                    task->attr.destroy(task->stack);
                }

                /* 删除对象 */
                Task_deleteTaskStruct(task);
            }
        });

        enum TaskPriority priority;
        TaskList_getHighestPriority(priority);
        if (priority > TASKPRIORITY_IDLE) {
            Task_yield();
        }
    }
}

/* 初始化调度器, 并开始执行任务 */
int Task_schedule() {
    /* 尝试创建空闲任务 */
    const struct TaskAttribute idle_task_attr = {
        .priority = TASKPRIORITY_IDLE,
    };

    if (Task_create(kernelIdleTask, NULL, kernelIdleTask_stack, KERNEL_IDLE_TASK_STACK_SIZE,
                    &idle_task_attr)) {
        /* 屏蔽中断 */
        uint32_t new_basepri = SHIELDABLE_INTERRUPT_MAX_PRIORITY;
        __asm {
            msr basepri, new_basepri
            dsb
            isb
        }

        /* 初始化Tick */
        InitSysTick_Port();

        /* 开始执行任务 */
        StartFirstTask_Port();

        /* 正常不会返回 */
        return 0;
    }

    return -1;
}

/* 创建和删除任务对象 */
#if (DYNAMIC_MEMORY_ALLOCATION)
#include <zhiyec/memory.h>

static struct TaskStruct *Task_newTaskStruct(stack_t *const stack,
                                             stack_t *const top_of_stack) {
    struct TaskStruct *new_task = Memory_alloc(sizeof(struct TaskStruct));
    if (new_task != NULL) {
        new_task->stack = stack;
        new_task->top_of_stack = top_of_stack;
        new_task->resume_time = 0U;
    }

    return new_task;
}

static void Task_deleteTaskStruct(struct TaskStruct *const task) {
    Memory_free(task);
}

#else
static struct TaskStruct task_structures[TASK_MAX_NUM];
static size_t task_structures_pos = 0U;

static struct TaskStruct *Task_newTaskStruct(stack_t *const stack,
                                             stack_t *const top_of_stack) {
    const struct TaskStruct task = {
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

    struct TaskStruct *const new_task = &task_structures[task_structures_pos];
    ++task_structures_pos;
    *new_task = task;

    return new_task;
}

static void Task_deleteTaskStruct(struct TaskStruct *const task) {
    for (size_t i = 0; i < TASK_MAX_NUM; ++i) {
        if (&task_structures[i] == task) {
            task_structures[i].stack = NULL;
            break;
        }
    }
}

#endif /* DYNAMIC_MEMORY_ALLOCATION */

/* -------私有接口------- */

/* 当任务函数返回时会回到这个函数 */
void TaskReturnHandler() {
    Task_deleteLater();

    for (;;) {
    }
}

/* 是否需要切换任务 */
bool Task_needSwitch() {
    /* 内核Tick计数 */
    ++kernel_ticks;

    /* 检测到Tick计数溢出, 准备切换到下一个Tick周期 */
    if (Tick_currentTick() == 0U) {
        blocked_task_list_ready_switch_next_cycle = true;
    }

    /* 检查阻塞列表任务是否超时 */
    if ((!StackList_isEmpty(*blocked_task_list_curr_cycle)) &&
        Tick_after(Tick_currentTick(), Task_fromTaskNode(StackList_front(*blocked_task_list_curr_cycle))->resume_time)) {

        struct TaskStruct *const timeout_task = Task_fromTaskNode(StackList_front(*blocked_task_list_curr_cycle));
        StackList_pop(*blocked_task_list_curr_cycle);

        TaskList_append(timeout_task->attr.priority, &(timeout_task->task_node));
    }

    /* 若当前Tick周期的任务处理完毕, 切换到下一个Tick周期的列表 */
    if (blocked_task_list_ready_switch_next_cycle &&
        StackList_isEmpty(*blocked_task_list_curr_cycle)) {
        blocked_task_list_ready_switch_next_cycle = false;

        struct StackList *tmp = blocked_task_list_curr_cycle;
        blocked_task_list_curr_cycle = blocked_task_list_next_cycle;
        blocked_task_list_next_cycle = tmp;
    }

    /* 调度器已暂停, 不切换任务 */
    if (task_suspended_count) {
        return false;
    }

    enum TaskPriority priority;
    TaskList_getHighestPriority(priority);
    /* 有更高优先级的任务, 需要切换任务 */
    if (priority > kernel_current_task->attr.priority) {
        return true;
    }

    /* 同优先级之间如果为时间片轮转调度则切换任务 */
    return (kernel_current_task->attr.sched_method == SCHED_RR);
}

/* 切换下一个任务 */
void Task_switchNextTask() {
    /* 是否需要将当前任务移动到列表尾部 */
    if (TaskList_getFrontTask(kernel_current_task->attr.priority) == kernel_current_task) {
        TaskList_append(kernel_current_task->attr.priority,
                        TaskList_removeFront(kernel_current_task->attr.priority));
    }

    /* 获取最高优先级的任务 */
    enum TaskPriority priority;
    TaskList_getHighestPriority(priority);
    kernel_current_task = TaskList_getFrontTask(priority);
}

/* --------------------- */
