#include "Task.h"
#include "Hook.h"
#include "TaskList.h"
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

/* 调度器暂停计数器 */
volatile unsigned int scheduling_suspended = 0U;

/* 正在运行任务的指针 */
struct TaskStruct *volatile current_task = NULL;

/* 用于管理空闲任务 */
static struct TaskStruct *kernel_idle_task = NULL;
#define KERNEL_IDLE_TASK_TYPE ((enum TaskType)(-1))

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
#include "Memory.h"
#else
static struct TaskStruct task_structures[TASK_MAX_NUM];
static size_t task_structures_pos = 0U;
#endif

/* 阻塞任务列表 */
static struct TaskListNode *blocked_task_list_head = NULL;

/* 等待删除任务列表 */
static struct TaskListNode *to_delete_task_list_head = NULL;

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

    struct TaskStruct *task = Task_newTaskStruct(
        stack,
        InitTaskStack_Port(top_of_stack, fn, arg));

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
    } else {
        TaskList_append(task->type, &(task->node));
    }

    if (current_task == NULL) {
        current_task = task;
    }

    Task_resumeScheduling();

    return true;
}

/* 将任务插入阻塞列表 */
static inline void Task_insertBlockedList(struct TaskListNode *const front_node) {
    if (blocked_task_list_head == NULL) {
        blocked_task_list_head = front_node;
    } else {
        /* 按时间升序插入到阻塞列表 */
        struct TaskListNode *current_node = blocked_task_list_head;
        struct TaskListNode *prev_node = NULL;
        while (current_node != NULL) {
            if (Tick_after(container_of(current_node, struct TaskStruct, node)->resume_time,
                           container_of(front_node, struct TaskStruct, node)->resume_time)) {
                front_node->next = current_node;
                if (prev_node != NULL) {
                    prev_node->next = front_node;
                } else {
                    blocked_task_list_head = front_node;
                }
                return;
            }

            prev_node = current_node;
            current_node = current_node->next;
        }

        /* 比阻塞列表所有元素时间都大, 添加到阻塞列表末尾 */
        prev_node->next = front_node;
    }
}

void Task_sleep(const Tick_t ticks) {
    if (ticks > 0U) {
        const Tick_t blocking_ticks = Tick_currentTicks() + ticks;

        Task_suspendScheduling();

        struct TaskListNode *const front_node = TaskList_remove(current_task->type);

        if (front_node == NULL) {
            Task_resumeScheduling();
            return;
        }

        container_of(front_node, struct TaskStruct, node)->resume_time = blocking_ticks;

        Task_insertBlockedList(front_node);

        Task_resumeScheduling();
        yield();
    }
}

void Task_deleteSelf() {
    Task_suspendScheduling();

    struct TaskListNode *const front_node = TaskList_remove(current_task->type);

    if (front_node == NULL) {
        Task_resumeScheduling();
        return;
    }

    TaskList_push(to_delete_task_list_head, front_node);

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

        struct TaskListNode *to_delete_node = to_delete_task_list_head;
        TaskList_pop(to_delete_task_list_head);

        if (to_delete_node != NULL) {
#ifdef Hook_deleteTask
            Hook_deleteTask(container_of(to_delete_node, struct TaskStruct, node));
#endif

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
            if (container_of(to_delete_node, struct TaskStruct, node)->is_dynamic_stack) {
                Memory_free(container_of(to_delete_node, struct TaskStruct, node)->stack);
            }
#endif
            Task_deleteTaskStruct(container_of(to_delete_node, struct TaskStruct, node));
        }

        Task_resumeScheduling();

        if (TaskList_hasRealTimeTask() || TaskList_hasCommonTask()) {
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
        Port_disableInterrupt();

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

/* 外部调用接口 */

/* 当任务函数返回时会回到这个函数 */
void TaskReturnHandler() {
    Task_deleteSelf();

    for (;;) {
    }
}

/* 是否需要切换任务 */
bool Task_needsSwitch() {
    if ((scheduling_suspended == 0U) && (current_task->sched_method == SCHED_RR)) {
        return true;
    }
    return false;
}

/* 切换下一个任务 */
void Task_switchNextTask() {
    /* 检查阻塞列表任务是否超时 */
    if ((blocked_task_list_head != NULL) && Tick_after(Tick_currentTicks(),
                                                       container_of(blocked_task_list_head, struct TaskStruct, node)->resume_time)) {
        struct TaskStruct *const timeout_task = container_of(blocked_task_list_head, struct TaskStruct, node);
        TaskList_pop(blocked_task_list_head);
        timeout_task->resume_time = 0U;

        TaskList_append(timeout_task->type, &(timeout_task->node));
    }

    struct TaskStruct *task = TaskList_getFrontRealTimeTask();
    if (task == current_task) {
        struct TaskListNode *const front_node = TaskList_remove(REALTIME_TASK);
        TaskList_append(REALTIME_TASK, front_node);
        task = TaskList_getFrontRealTimeTask();
    }

    /* 选择任务 */
    if (task == NULL) {
        /* 从活跃列表中获取第一个任务 */
        task = TaskList_getFrontCommonTask();
        if (task == current_task) {
            struct TaskListNode *const front_node = TaskList_remove(COMMON_TASK);
            TaskList_append(COMMON_TASK, front_node);
            task = TaskList_getFrontCommonTask();
        }

        if (task == NULL) {
            task = kernel_idle_task;
        }
    }

    current_task = task;
}

/* 结束 */
