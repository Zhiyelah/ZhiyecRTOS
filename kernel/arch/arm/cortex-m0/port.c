#include "port.h"
#include <zhiyec/task.h>
#include <../kernel/hook.h>

#define SysTick_Handler_Port CONFIG_SYSTICK_HANDLER_PORT
#define SVC_Handler_Port CONFIG_SVC_HANDLER_PORT
#define PendSV_Handler_Port CONFIG_PENDSV_HANDLER_PORT
#define SYSTICK_LOAD_REG_VALUE (CONFIG_CPU_CLOCK_HZ / CONFIG_SYSTICK_RATE_HZ)

/* SysTick寄存器 */
#define SysTick_CTRL_Reg (*((volatile uint32_t *)0xe000e010))

/* SysTick控制寄存器位定义 */
#define SysTick_ENABLE_Bit (1UL << 0UL)
#define SysTick_INT_Bit (1UL << 1UL)
#define SysTick_CLK_Bit (1UL << 2UL)

/* 优先级寄存器 */
#define SHPR3_Reg (*((volatile uint32_t *)0xe000ed20))
/* 最小优先级 */
#define MIN_Interrupt_Priority (0xFFul)

/* SysTick和PendSV的优先级 */
#define SHPR3_PENDSV_Priority (((uint32_t)MIN_Interrupt_Priority) << 16UL)
#define SHPR3_SYSTICK_Priority (((uint32_t)CONFIG_KERNEL_INTERRUPT_PRIORITY) << 24UL)

void InitSysTick_Port() {
    /* 配置SysTick及PendSV优先级 */
    SHPR3_Reg |= SHPR3_PENDSV_Priority;
    SHPR3_Reg |= SHPR3_SYSTICK_Priority;
    /* 配置SysTick */
    SysTick_CTRL_Reg = 0;
    SysTick_LOAD_Reg = (SYSTICK_LOAD_REG_VALUE & 0xFFFFFFul) - 1;
    SysTick_VALUE_Reg = 0;
    SysTick_CTRL_Reg = SysTick_ENABLE_Bit | SysTick_INT_Bit | SysTick_CLK_Bit;
}

/* 初始化任务栈接口 */
stack_t *InitTaskStack_Port(stack_t *top_of_stack, void (*const fn)(void *), void *const arg) {
    /* xPSR寄存器 */
    *(--top_of_stack) = 0x01000000; /* 以Thumb指令模式执行(内存受限场景, Cortex-M平台要求) */
    /* PC寄存器 */
    *(--top_of_stack) = (stack_t)fn;
    /* LR寄存器 */
    extern void TaskReturnHandler();
    *(--top_of_stack) = (stack_t)TaskReturnHandler;
    /* r12, r3, r2, r1寄存器 */
    top_of_stack -= 4;
    /* r0寄存器 */
    *(--top_of_stack) = (stack_t)arg;
    /* r11, r10, r9, r8, r7, r6, r5, r4寄存器 */
    top_of_stack -= 8;

    return top_of_stack;
}

__asm void StartFirstTask_Port() {
    PRESERVE8

    extern kernel_current_task;

    ldr r3, =kernel_current_task
    ldr r0, [r3]
    ldr r0, [r0]

    /* 跳过低位寄存器 */
    adds r0, #32

    msr psp, r0

    /* 使用PSP作为堆栈指针 */
    movs r0, #2
    msr CONTROL, r0
    isb

    /* 加载寄存器 */
    pop {r0-r5}
    /* 使用r5作为lr寄存器 */
    mov lr, r5
    /* 返回地址保存在r3 */
    pop {r3}
    /* 弹出XPSR寄存器, 丢弃 */
    pop {r2}

    /* 启用中断并返回 */
    cpsie i
    bx r3

    ALIGN
}

void SysTick_Handler_Port() {
#ifdef Hook_isrSysTickEntry
    Hook_isrSysTickEntry();
#endif

    uint32_t prev_basepri = Port_disableInterruptFromISR();

    if (Task_needSwitch()) {
        Interrupt_CTRL_Reg = PendSV_SET_Bit;
    }

    Port_enableInterruptFromISR(prev_basepri);
}

__asm void SVC_Handler_Port() {
}

__asm void PendSV_Handler_Port() {
    PRESERVE8

    /* 获取进程堆栈指针 */
    mrs r0, psp

    /* 为低位寄存器预留空间 */
    subs r0, #32

    /* 保存当前堆栈 */
    extern kernel_current_task;
    ldr r3, =kernel_current_task
    ldr r2, [r3]
    str r0, [r2]

    /* 保存低位寄存器 */
    stmia r0!, {r4-r7}
    /* 保存高位寄存器 */
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    stmia r0!, {r4-r7}

    /* 选择下一个任务 */
    push {r3, lr}
    cpsid i
    extern Task_switchNextTask;
    bl Task_switchNextTask
    cpsie i
    /* r2存放最近任务的地址, r3存放lr寄存器 */
    pop {r2, r3}

    /* 恢复下一个任务的堆栈 */
    ldr r0, [r2]
    ldr r0, [r0]
    adds r0, #16  /* 偏移到高位寄存器 */
    ldmia r0!, {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    msr psp, r0
    subs r0, #32  /* 偏移到低位寄存器 */
    ldmia r0!, {r4-r7}

    /* 跳转执行 */
    bx r3
    ALIGN
}
