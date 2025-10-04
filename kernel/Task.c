#include <../kernel/Atomic.h>
#include <../kernel/Hook.h>
#include <../kernel/TaskList.h>
#include <zhiyec/List.h>
#include <zhiyec/Task.h>

#define SYSTICK_RATE_HZ (CONFIG_SYSTICK_RATE_HZ)
#define CPU_CLOCK_HZ (CONFIG_CPU_CLOCK_HZ)
#define SYSTICK_LOAD_REG_VALUE (CPU_CLOCK_HZ / SYSTICK_RATE_HZ)

#define MANAGED_INTERRUPT_MAX_PRIORITY (CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY)

#define DEFAULT_TASK_STACK_SIZE (CONFIG_DEFAULT_TASK_STACK_SIZE)
#define TASK_MAX_NUM (CONFIG_TASK_MAX_NUM)

#if (SYSTICK_LOAD_REG_VALUE > 0xFFFFFFul)
#error "the value "CONFIG_SYSTICK_RATE_HZ" in file Config.h is too small."
#endif /* SYSTICK_LOAD_REG_VALUE overflow */

/* 用于管理当前任务 */
static size_t task_count = 0U;
static volatile int task_suspended_count = 0;
struct TaskStruct *volatile current_task = NULL;

/* 用于管理空闲任务 */
static struct TaskStruct *kernel_idle_task = NULL;
#define KERNEL_IDLE_TASK_TYPE ((enum TaskType)(-1))

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
#include <zhiyec/Memory.h>
#else
static struct TaskStruct task_structures[TASK_MAX_NUM];
static size_t task_structures_pos = 0U;
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */

/* 阻塞任务列表 */
static struct StackList *blocked_task_list_curr_cycle = &(struct StackList){};
static struct StackList *blocked_task_list_next_cycle = &(struct StackList){};
static bool blocked_task_list_ready_switch_next_cycle = false;

/* 等待删除任务列表 */
static struct StackList to_delete_task_list;

static inline struct TaskStruct *Task_newTaskStruct(stack_t *const stack, stack_t *const top_of_stack);
static inline void Task_deleteTaskStruct(struct TaskStruct *const task);

/* 创建任务 */
bool Task_create(void (*const fn)(void *), void *const arg, const struct TaskAttribute *attr) {
    if (fn == NULL) {
        return false;
    }

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    if (attr == NULL) {
        const struct TaskAttribute default_attr = {
            .type = COMMON_TASK,
            .sched_method = SCHED_RR,
            .stack_size = DEFAULT_TASK_STACK_SIZE,
        };
        attr = &default_attr;
    }
#else
    if ((attr == NULL) || (attr->stack == NULL)) {
        return false;
    }
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */

    stack_t *stack = attr->stack;
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    bool is_dynamic_stack = false;

    /* 未指定栈指针 */
    if (stack == NULL) {
        stack = Memory_alloc(attr->stack_size * sizeof(stack_t));
        /* 内存分配失败 */
        if (stack == NULL) {
            return false;
        }

        is_dynamic_stack = true;
    }
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */

    stack_t *top_of_stack = &stack[attr->stack_size - (stack_t)1U];

    /* 内存对齐 */
    top_of_stack = (stack_t *)(((stack_t)top_of_stack) & ~((stack_t)(BYTE_ALIGNMENT - 1)));

    Atomic_begin();

    struct TaskStruct *task = Task_newTaskStruct(
        stack,
        InitTaskStack_Port(top_of_stack, fn, arg));

    Atomic_end();

    if (task == NULL) {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
        if (is_dynamic_stack) {
            Memory_free(stack);
        }
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */
        return false;
    }

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    task->is_dynamic_stack = is_dynamic_stack;
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */
    task->type = attr->type;

    /* 实时任务才支持更改调度 */
    if (task->type == REALTIME_TASK) {
        task->sched_method = attr->sched_method;
    }

    atomic({
        if (!TaskList_isInit()) {
            TaskList_init();
        }

        if (task->type == KERNEL_IDLE_TASK_TYPE) {
            kernel_idle_task = task;
        } else {
            TaskList_append(task->type, &(task->task_node));
        }

        if (current_task == NULL) {
            current_task = task;
        }

        ++task_count;
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
static inline void Task_sleepHelper(const tick_t resume_time) {
    atomic({
        struct SListHead *const front_node = TaskList_removeFront(current_task->type);

        if (front_node != NULL) {
            Task_fromTaskNode(front_node)->resume_time = resume_time;

            const tick_t current_time = Tick_currentTicks();

            /* 是否为下一个tick周期的任务 */
            if (resume_time < current_time && !Tick_after(current_time, resume_time)) {
                Task_insertBlockedList(blocked_task_list_next_cycle, front_node);
            } else {
                Task_insertBlockedList(blocked_task_list_curr_cycle, front_node);
            }
        }
    });

    Task_yield();
}

/* 任务睡眠 */
void Task_sleep(const tick_t ticks) {
    if (ticks > 0U) {
        Task_sleepHelper(Tick_currentTicks() + ticks);
    } else {
        Task_yield();
    }
}

/* 任务周期睡眠 */
void Task_sleepUntil(tick_t *const prev_wake_time, const tick_t interval) {
    const tick_t resume_time = (*prev_wake_time) + interval;

    /* 如果任务执行时间小于周期, 阻塞任务 */
    if (!Tick_after(Tick_currentTicks(), resume_time)) {
        Task_sleepHelper(resume_time);
    }

    *prev_wake_time = resume_time;
}

/* 任务删除自身 */
void Task_deleteSelf() {
    atomic({
        struct SListHead *const front_node = TaskList_removeFront(current_task->type);

        if (front_node != NULL) {
            StackList_push(to_delete_task_list, front_node);
            --task_count;
        }
    });

    Task_yield();
}

/* 获取任务数量 */
size_t Task_getCount() {
    return task_count;
}

/* 暂停所有任务 */
void Task_suspendAll() {
    atomic({
        ++task_suspended_count;
    });
}

/* 恢复所有任务 */
void Task_resumeAll() {
    atomic({
        --task_suspended_count;
    });
}

/* 是否需要切换任务 */
bool Task_needSwitch() {
    if (Tick_currentTicks() == 0U) {
        blocked_task_list_ready_switch_next_cycle = true;
    }

    /* 检查阻塞列表任务是否超时 */
    if ((!StackList_isEmpty(*blocked_task_list_curr_cycle)) &&
        Tick_after(Tick_currentTicks(), Task_fromTaskNode(StackList_front(*blocked_task_list_curr_cycle))->resume_time)) {

        struct TaskStruct *const timeout_task = Task_fromTaskNode(StackList_front(*blocked_task_list_curr_cycle));
        StackList_pop(*blocked_task_list_curr_cycle);

        TaskList_append(timeout_task->type, &(timeout_task->task_node));
    }

    /* 若当前tick周期的任务处理完毕, 切换到下一个tick周期的列表 */
    if (blocked_task_list_ready_switch_next_cycle &&
        StackList_isEmpty(*blocked_task_list_curr_cycle)) {
        blocked_task_list_ready_switch_next_cycle = false;

        struct StackList *tmp = blocked_task_list_curr_cycle;
        blocked_task_list_curr_cycle = blocked_task_list_next_cycle;
        blocked_task_list_next_cycle = tmp;
    }

    return (task_suspended_count == 0) && (current_task->sched_method == SCHED_RR);
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

#ifdef Hook_taskDeletion
                Hook_taskDeletion(Task_fromTaskNode(to_delete_node));
#endif /* Hook_taskDeletion */

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
                if (Task_fromTaskNode(to_delete_node)->is_dynamic_stack) {
                    Memory_free(Task_fromTaskNode(to_delete_node)->stack);
                }
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */
                Task_deleteTaskStruct(Task_fromTaskNode(to_delete_node));
            }
        });

        if (TaskList_hasTask(REALTIME_TASK) || TaskList_hasTask(COMMON_TASK)) {
            Task_yield();
        }
    }
}

/* 开始执行任务, 调度器初始化配置 */
int Task_exec() {
    /* 尝试创建空闲任务 */
    const struct TaskAttribute idle_task_attr = {
        .stack = kernelIdleTask_stack,
        .stack_size = KERNEL_IDLE_TASK_STACK_SIZE,
        .type = KERNEL_IDLE_TASK_TYPE,
    };
    if (Task_create(kernelIdleTask, NULL, &idle_task_attr)) {
        /* 屏蔽中断 */
        uint32_t new_basepri = MANAGED_INTERRUPT_MAX_PRIORITY;
        __asm {
            msr basepri, new_basepri
            dsb
            isb
        }

        /* 配置SysTick及PendSV优先级 */
        SHPR3_Reg |= SHPR3_PENDSV_Priority;
        SHPR3_Reg |= SHPR3_SYSTICK_Priority;
        /* 配置SysTick */
        SysTick_CTRL_Reg = 0;
        SysTick_LOAD_Reg = (SYSTICK_LOAD_REG_VALUE & 0xFFFFFFul) - 1;
        SysTick_VALUE_Reg = 0;
        SysTick_CTRL_Reg = SysTick_ENABLE_Bit | SysTick_INT_Bit | SysTick_CLK_Bit;

        /* 开始执行任务 */
        StartFirstTask_Port();
        /* 不会返回 */
        return 0;
    }

    return -1;
}

/* 创建任务对象 */
static inline struct TaskStruct *Task_newTaskStruct(stack_t *const stack, stack_t *const top_of_stack) {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    struct TaskStruct *new_task = Memory_alloc(sizeof(struct TaskStruct));
    if (new_task != NULL) {
        new_task->stack = stack;
        new_task->top_of_stack = top_of_stack;
        new_task->sched_method = SCHED_RR;
        new_task->resume_time = 0U;
    }
#else
    const struct TaskStruct task = {
        .stack = stack,
        .top_of_stack = top_of_stack,
        .sched_method = SCHED_RR,
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
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */

    return new_task;
}

/* 删除任务对象 */
static inline void Task_deleteTaskStruct(struct TaskStruct *const task) {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    Memory_free(task);
#else
    for (size_t i = 0; i < TASK_MAX_NUM; ++i) {
        if (&task_structures[i] == task) {
            task_structures[i].stack = NULL;
            break;
        }
    }
#endif /* USE_DYNAMIC_MEMORY_ALLOCATION */
}

/* 外部调用接口 */

struct TaskStruct *Task_currentTask() {
    return current_task;
}

/* 当任务函数返回时会回到这个函数 */
void TaskReturnHandler() {
    Task_deleteSelf();

    for (;;) {
    }
}

/* 切换下一个任务 */
void Task_switchNextTask() {
    struct TaskStruct *front_realtime_task = TaskList_getFrontTask(REALTIME_TASK);
    struct TaskStruct *front_common_task = TaskList_getFrontTask(COMMON_TASK);

    if (front_realtime_task == current_task) {
        TaskList_append(REALTIME_TASK, TaskList_removeFront(REALTIME_TASK));
        front_realtime_task = TaskList_getFrontTask(REALTIME_TASK);
    } else if (front_common_task == current_task) {
        TaskList_append(COMMON_TASK, TaskList_removeFront(COMMON_TASK));
        front_common_task = TaskList_getFrontTask(COMMON_TASK);
    }

    current_task = (front_realtime_task) ? front_realtime_task
                   : (front_common_task) ? front_common_task
                                         : kernel_idle_task;
}

/* 结束 */
