#include "Task.h"
#include "TaskList.h"
#include "Hook.h"
#include <stddef.h>

#define SYSTICK_RATE_HZ (CONFIG_SYSTICK_RATE_HZ)
#define CPU_CLOCK_HZ (CONFIG_CPU_CLOCK_HZ)
#define SYSTICK_LOAD_REG_VALUE (CPU_CLOCK_HZ / SYSTICK_RATE_HZ)

#define MANAGED_INTERRUPT_MAX_PRIORITY (CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY)

#define DEFAULT_TASK_STACK_SIZE (CONFIG_DEFAULT_TASK_STACK_SIZE)
#define TASK_MAX_NUM (CONFIG_TASK_MAX_NUM)

#if (SYSTICK_LOAD_REG_VALUE > 0xFFFFFFul)
#error "the value "CONFIG_SYSTICK_RATE_HZ" in file Config.h is too small."
#endif

volatile unsigned int scheduling_suspended = 0U;
struct TaskStruct *volatile current_task = NULL;

static struct TaskStruct *kernel_idle_task = NULL;
#define KERNEL_IDLE_TASK_TYPE ((enum TaskType)(-1))

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
#include "Memory.h"
#else
static struct TaskStruct task_structures[TASK_MAX_NUM];
static size_t task_structures_pos = 0U;
#endif

static Stack_t *Task_initTaskStack(Stack_t *top_of_stack, void (*const fn)(void *), void *const arg);
static void Task_startFirstTask(void);
static struct TaskStruct *Task_newTaskStruct(Stack_t *const stack, Stack_t *const top_of_stack);
static void Task_deleteTaskStruct(struct TaskStruct *const task);

void Task_suspendScheduling() {
    ++scheduling_suspended;
}

void Task_resumeScheduling() {
    if (scheduling_suspended > 0U) {
        --scheduling_suspended;
    }
}

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
#endif

    Stack_t *stack = attr->stack;
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    bool is_dynamic_stack = false;

    /* 未指定栈指针 */
    if (stack == NULL) {
        stack = Memory_alloc(attr->stack_size * sizeof(Stack_t));
        /* 内存分配失败 */
        if (stack == NULL) {
            return false;
        }

        is_dynamic_stack = true;
    }
#endif

    Stack_t *top_of_stack = &stack[attr->stack_size - (Stack_t)1U];

    /* 内存对齐 */
    top_of_stack = (Stack_t *)(((Stack_t)top_of_stack) & ~((Stack_t)(BYTE_ALIGNMENT - 1)));

    Task_suspendScheduling();

    struct TaskStruct* task = Task_newTaskStruct(
        stack,
        Task_initTaskStack(top_of_stack, fn, arg)
    );

    Task_resumeScheduling();

    if (task == NULL) {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
        if (is_dynamic_stack) {
            Memory_free(stack);
        }
#endif
        return false;
    }

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    task->is_dynamic_stack = is_dynamic_stack;
#endif
    task->type = attr->type;

    /* 实时任务才支持更改调度 */
    if (task->type == REALTIME_TASK) {
        task->sched_method = attr->sched_method;
    }

    Task_suspendScheduling();

    if (task->type == KERNEL_IDLE_TASK_TYPE) {
        kernel_idle_task = task;
    } else if (!TaskList_addNewTask(task)) {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
        if (attr->stack == NULL) {
            Memory_free(stack);
        }
#endif
        Task_deleteTaskStruct(task);

        Task_resumeScheduling();
        return false;
    }

    if (current_task == NULL) {
        current_task = task;
    }

    Task_resumeScheduling();

    return true;
}

void Task_sleep(const Tick_t ticks) {
    if (ticks > 0U) {
        const Tick_t blocking_ticks = Tick_currentTicks() + ticks;

        Task_suspendScheduling();

        struct TaskListNode *front_node = NULL;
        if (current_task->type == COMMON_TASK) {
            front_node = TaskList_pop(ACTIVE_TASK_LIST);
        } else if (current_task->type == REALTIME_TASK) {
            front_node = TaskList_pop(REALTIME_TASK_LIST);
        }

        if (front_node == NULL) {
            Task_resumeScheduling();
            return;
        }

        front_node->task->resume_time = blocking_ticks;

        TaskList_insertBlockedList(front_node);

        Task_resumeScheduling();
        yield();
    }
}

void Task_deleteSelf() {
    Task_suspendScheduling();

    struct TaskListNode *front_node;
    if (current_task->type == COMMON_TASK) {
        front_node = TaskList_pop(ACTIVE_TASK_LIST);
    } else if (current_task->type == REALTIME_TASK) {
        front_node = TaskList_pop(REALTIME_TASK_LIST);
    }

    if (front_node == NULL) {
        Task_resumeScheduling();
        return;
    }

    TaskList_pushSpecialListByEnum(TO_DELETE_TASK_LIST, front_node);

    Task_resumeScheduling();
    yield();
}

#define KERNEL_IDLE_TASK_STACK_SIZE 64
static Stack_t kernelIdleTask_stack[KERNEL_IDLE_TASK_STACK_SIZE];
/* 空闲任务(最低优先级) */
static void kernelIdleTask(void *arg) {
    (void)arg;

    for (;;) {
        #ifdef Hook_IdleTask
            Hook_runIdleTask();
        #endif

        Task_suspendScheduling();

        struct TaskListNode *to_delete_node = TaskList_popSpecialListByEnum(TO_DELETE_TASK_LIST);

        if (to_delete_node != NULL) {
            #ifdef Hook_deleteTask
                Hook_deleteTask(to_delete_node->task);
            #endif

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
            if (to_delete_node->task->is_dynamic_stack) {
                Memory_free(to_delete_node->task->stack);
            }
#endif
            Task_deleteTaskStruct(to_delete_node->task);
            TaskList_freeTask(to_delete_node);
        }

        Task_resumeScheduling();

        if (TaskList_hasRealTimeTask() || TaskList_hasActiveTask()) {
            yield();
        }
    }
}

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

        Task_startFirstTask();
        return 0;
    }

    return -1;
}

/* 创建任务对象 */
static struct TaskStruct *Task_newTaskStruct(Stack_t *const stack, Stack_t *const top_of_stack) {
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
#endif

    return new_task;
}

/* 删除任务对象 */
static void Task_deleteTaskStruct(struct TaskStruct *const task) {
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    Memory_free(task);
#else
    for (size_t i = 0; i < TASK_MAX_NUM; ++i) {
        if (&task_structures[i] == task) {
            task_structures[i].stack = NULL;
            break;
        }
    }
#endif
}

/* 从任务列表中移除当前任务节点并返回 */
struct TaskListNode *Task_popFromTaskList() {
    struct TaskListNode *front_node = NULL;
    if (current_task->type == COMMON_TASK) {
        front_node = TaskList_pop(ACTIVE_TASK_LIST);
    } else if (current_task->type == REALTIME_TASK) {
        front_node = TaskList_pop(REALTIME_TASK_LIST);
    }
    return front_node;
}

/* 当任务函数返回时会回到这个函数 */
static void TaskReturnHandler() {
    Task_deleteSelf();

    for (;;) {
    }
}

/* 初始化任务栈 */
static Stack_t *Task_initTaskStack(Stack_t *top_of_stack, void (*const fn)(void *), void *const arg) {
    /* xPSR寄存器 */
    *(--top_of_stack) = 0x01000000; /* 以Thumb指令模式执行(内存受限场景, Cortex-M平台要求) */
    /* PC寄存器 */
    *(--top_of_stack) = ((Stack_t)fn) & ((Stack_t)0xFFFFFFFEul); /* 将Thumb指令地址转为函数的实际地址 */
    /* LR寄存器 */
    *(--top_of_stack) = (Stack_t)TaskReturnHandler;
    /* r12, r3, r2, r1寄存器 */
    top_of_stack -= 4;
    /* r0寄存器 */
    *(--top_of_stack) = (Stack_t)arg;
    /* r11, r10, r9, r8, r7, r6, r5, r4寄存器 */
    top_of_stack -= 8;

    return top_of_stack;
}

void Task_switchNextTask() {
    /* 检查阻塞列表任务是否超时 */
    if (TaskList_hasBlockedTask() && Tick_after(Tick_currentTicks(),
                                                TaskList_getFrontBlockedTaskTime())) {
        struct TaskListNode *const front_node = TaskList_popSpecialListByEnum(BLOCKED_TASK_LIST);
        front_node->task->resume_time = 0U;
        if (front_node->task->type == COMMON_TASK) {
            TaskList_pushBack(ACTIVE_TASK_LIST, front_node);
        } else if (front_node->task->type == REALTIME_TASK) {
            TaskList_pushBack(REALTIME_TASK_LIST, front_node);
        }
    }

    struct TaskStruct *task = TaskList_getFrontRealTimeTask();
    if (task == current_task) {
        struct TaskListNode *const front_node = TaskList_pop(REALTIME_TASK_LIST);
        TaskList_pushBack(REALTIME_TASK_LIST, front_node);
        task = TaskList_getFrontRealTimeTask();
    }

    /* 选择任务 */
    if (task == NULL) {
        /* 从活跃列表中获取第一个任务 */
        task = TaskList_getFrontActiveTask();
        if (task == current_task) {
            struct TaskListNode *const front_node = TaskList_pop(ACTIVE_TASK_LIST);
            TaskList_pushBack(ACTIVE_TASK_LIST, front_node);
            task = TaskList_getFrontActiveTask();
        }

        if (task == NULL) {
            task = kernel_idle_task;
        }
    }

    current_task = task;
}

__asm static void Task_startFirstTask() {
    /* 8 字节对齐 */
    PRESERVE8

    /* 从VTOR寄存器获取初始堆栈指针 */
    ldr r0, =0xE000ED08
    ldr r0, [r0]
    ldr r0, [r0]

    /* 将MSP设置为初始值 */
    msr msp, r0

    /* 启用全局中断 */
    cpsie i
    cpsie f
    dsb
    isb

    /* 调用SVC处理函数 */
    svc 0
    nop
    nop
}
