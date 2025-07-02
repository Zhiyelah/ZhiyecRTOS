#include "Task.h"
#include "Interrupt.h"
#include "Tick.h"
#include "TaskList.h"
#include <stddef.h>
#include "Config.h"

/* 内存字节对齐位 */
#define BYTE_ALIGNMENT 8

#define Systick_Handler_Port CONFIG_SYSTICK_HANDLER_PORT
#define PendSV_Handler_Port CONFIG_PENDSV_HANDLER_PORT
#define SVC_Handler_Port CONFIG_SVC_HANDLER_PORT

#define INTERRUPT_HZ (CONFIG_INTERRUPT_HZ)
#define CPU_CLOCK_HZ (CONFIG_CPU_CLOCK_HZ)
#define SYSTICK_LOAD_REG_VALUE (CPU_CLOCK_HZ / INTERRUPT_HZ)

#define MAX_SYSCALL_INTERRUPT_PRIORITY (CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY)
#define TASK_MAX_NUM (CONFIG_TASK_MAX_NUM)

#if (SYSTICK_LOAD_REG_VALUE > 0xFFFFFFul)
#error "the value "CONFIG_INTERRUPT_HZ" in file Config.h is too small."
#endif

static volatile unsigned int scheduling_suspended = 0;
volatile struct TaskStruct *current_task = NULL;
static struct TaskStruct task_structures[TASK_MAX_NUM];
static uint16_t task_structures_pos = 0;

static Stack_t *Task_initTaskStack(Stack_t *top_of_stack, void (*fn)(void *), void *arg);
static void ErrorReturnHandler(void);
static void Task_startFirstTask(void);
static struct TaskStruct *Task_newTaskStruct(Stack_t *stack, Stack_t *top_of_stack);

void Task_suspendScheduling() {
    ++scheduling_suspended;
}

void Task_resumeScheduling() {
    if (scheduling_suspended > 0) {
        --scheduling_suspended;
    }
}

bool Task_create(void (*fn)(void *), void *arg, Stack_t *stack, Stack_t stack_size) {
    if (fn == NULL || stack == NULL) {
        return false;
    }

    Task_suspendScheduling();

    /* 内存对齐 */
    Stack_t *top_of_stack = &stack[stack_size - (Stack_t)1];
    top_of_stack = (Stack_t *)(((uint32_t)top_of_stack) & (~((uint32_t)(BYTE_ALIGNMENT - 1))));

    struct TaskStruct* task = Task_newTaskStruct(stack, Task_initTaskStack(top_of_stack, fn, arg));
    if (task == NULL) {
        return false;
    }

    if (!TaskList_pushNewTask(task)) {
        return false;
    }

    if (current_task == NULL) {
        current_task = task;
    }

    Task_resumeScheduling();

    return true;
}

void Task_delay(Tick_t ticks) {
    if (ticks > 0) {
        Task_suspendScheduling();
        TaskList_moveFrontActiveTaskToBlockedList(Tick_getCurrentTicks() + ticks);
        Task_resumeScheduling();
        yield();
    }
}

/* 空闲任务(优先级应该最低) */
static Stack_t kernelIdleTask_stack[64];
static void kernelIdleTask(void *arg) {
    (void)arg;

    for (;;) {
    }
}

int Task_exec() {
    /* 尝试创建空闲任务 */
    if (Task_create(kernelIdleTask, NULL, kernelIdleTask_stack, 64)) {
        __disable_interrupt();

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
static struct TaskStruct *Task_newTaskStruct(Stack_t *stack, Stack_t *top_of_stack) {
    const struct TaskStruct task = {
        .stack = stack,
        .top_of_stack = top_of_stack,
    };

    if (task_structures_pos >= TASK_MAX_NUM) {
        /* 尝试查找未使用的空间块 */
        for (uint16_t i = 0; i < TASK_MAX_NUM; ++i) {
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

/* 初始化任务栈 */
static Stack_t *Task_initTaskStack(Stack_t *top_of_stack, void (*fn)(void *), void *arg) {
    /* xPSR寄存器 */
    *(--top_of_stack) = 0x01000000; /* 以Thumb指令模式执行(内存受限场景, Cortex-M平台要求) */
    /* PC寄存器 */
    *(--top_of_stack) = ((Stack_t)fn) & ((Stack_t)0xFFFFFFFEul); /* 将Thumb指令地址转为函数的实际地址 */
    /* LR寄存器 */
    *(--top_of_stack) = (Stack_t)ErrorReturnHandler;
    /* r12, r3, r2, r1寄存器 */
    top_of_stack -= 4;
    /* r0寄存器 */
    *(--top_of_stack) = (Stack_t)arg;
    /* r11, r10, r9, r8, r7, r6, r5, r4寄存器 */
    top_of_stack -= 8;

    return top_of_stack;
}

/* 当任务函数返回时会回到这个函数 */
static void ErrorReturnHandler() {
    __disable_interrupt();

    for (;;) {
    }
}

void Task_nextTask() {
    if (TaskList_hasBlockedTask() &&
        Tick_after(Tick_getCurrentTicks(), TaskList_getFrontBlockedTaskTime())) {
        TaskList_putFrontBlockedTaskToActiveListBack();
    }

    struct TaskStruct *task = TaskList_getFrontActiveTask();
    if (task == current_task) {
        TaskList_moveFrontActiveTaskToBack();
        task = TaskList_getFrontActiveTask();
    }

    if (task != NULL) {
        current_task = task;
    }
}

void Systick_Handler_Port() {
    disable_interrupt_reentrant();
    kernel_Tick_inc();

    if (scheduling_suspended == 0U) {
        Interrupt_CTRL_Reg = PendSV_SET_Bit;
    }
    enable_interrupt_reentrant();
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

__asm void SVC_Handler_Port() {
    /* 8 字节对齐 */
    PRESERVE8

    /* 获取current_task的第一位成员 */
    ldr r3, =current_task
    ldr r1, [r3]
    ldr r0, [r1]

    /* 加载current_task栈顶指针中对应的寄存器 */
    ldmia r0!, {r4-r11}

    /* 设置进程堆栈指针为第一个任务的栈顶指针 */
    msr psp, r0
    isb

    /* 恢复中断 */
    mov r0, #0
    msr basepri, r0

    /* 使用PSP作为堆栈指针，并切换为用户级 */
    orr r14, #0xD

    /* 从PSP中恢复寄存器 */
    bx r14
}

__asm void PendSV_Handler_Port() {
    extern current_task;
    extern Task_nextTask;

    /* 8 字节对齐 */
    PRESERVE8

    /* 获取进程堆栈指针 */
    mrs r0, psp
    /* 确保PSP为最新 */
    isb

    /* 获取最近任务指向的结构体 */
    ldr	r3, =current_task
    ldr	r1, [r3]

    /* 除PendSV自动保存的寄存器外，将R4~R11保存到当前任务的任务栈中 */
    stmdb r0!, {r4-r11}

    /* 将进程堆栈指针保存到最近任务的堆栈指针 */
    str r0, [r1]

    /* 将r3和r14保存到内核堆栈MSP中 */
    stmdb sp!, {r3, r14}

    /* 屏蔽低优先级中断 */
    mov r0, #MAX_SYSCALL_INTERRUPT_PRIORITY
    msr basepri, r0
    dsb
    isb

    /* 跳转到任务切换函数 */
    bl Task_nextTask

    /* 恢复中断 */
    mov r0, #0
    msr basepri, r0

    /* 从MSP中加载r3, r14寄存器 */
    /* 因为之前跳转过函数，寄存器的值会重置 */
    ldmia sp!, {r3, r14}

    /* 重新从最近任务中读取栈顶指针 */
    /* 此时为新的栈顶指针 */
    ldr r1, [r3]
    ldr r0, [r1]

    /* 从任务栈中加载对应寄存器 */
    ldmia r0!, {r4-r11}

    /* 将栈顶指针更新到PSP寄存器 */
    msr psp, r0
    isb

    /* 从PSP指向的堆栈加载寄存器，并跳转执行 */
    bx r14
    nop
}
