#include "Task.h"
#include "Hook.h"

#define SysTick_Handler_Port CONFIG_SYSTICK_HANDLER_PORT
#define PendSV_Handler_Port CONFIG_PENDSV_HANDLER_PORT
#define SVC_Handler_Port CONFIG_SVC_HANDLER_PORT

__asm bool Port_isUsingPSP() {
    /* 判断当前sp指针是msp还是psp */
    mov r0, sp
    mrs r1, msp
    cmp r0, r1
    moveq r0, #0
    movne r0, #1
    bx lr
}

/* 初始化任务栈接口 */
Stack_t *InitTaskStack_Port(Stack_t *top_of_stack, void (*const fn)(void *), void *const arg) {
    /* xPSR寄存器 */
    *(--top_of_stack) = 0x01000000; /* 以Thumb指令模式执行(内存受限场景, Cortex-M平台要求) */
    /* PC寄存器 */
    *(--top_of_stack) = (Stack_t)fn;
    /* LR寄存器 */
    extern void TaskReturnHandler();
    *(--top_of_stack) = (Stack_t)TaskReturnHandler;
    /* r12, r3, r2, r1寄存器 */
    top_of_stack -= 4;
    /* r0寄存器 */
    *(--top_of_stack) = (Stack_t)arg;
    /* r11, r10, r9, r8, r7, r6, r5, r4寄存器 */
    top_of_stack -= 8;

    return top_of_stack;
}

__asm void StartFirstTask_Port() {
    /* 8 字节对齐 */
    PRESERVE8

    extern current_task;

    ldr r3, =current_task
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
    /* 默认以特权模式执行 */

#ifdef Hook_isrSysTickEntry
    Hook_isrSysTickEntry();
#endif

    Port_disableInterrupt();

    kernel_Tick_inc();

    extern bool Task_needSwitch();
    if (Task_needSwitch()) {
        Interrupt_CTRL_Reg = PendSV_SET_Bit;
    }

    Port_enableInterrupt();
}

__asm void SVC_Handler_Port() {
    /* 默认以特权模式执行 */
}

__asm void PendSV_Handler_Port() {
    /* 默认以特权模式执行 */

    /* 8 字节对齐 */
    PRESERVE8

    mrs r0, psp

    extern current_task;

    ldr r3, =current_task
    ldr r2, [r3]

    /* 为低位寄存器预留空间 */
    subs r0, #32

    /* 将进程堆栈指针保存到最近任务的堆栈指针 */
    str r0, [r2]

    /* 保存低位寄存器 */
    stmia r0!, {r4-r7}
    /* 保存高位寄存器 */
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    stmia r0!, {r4-r7}

    /* 将最近任务的地址和lr保存到内核堆栈MSP中 */
    push {r3, lr}

    /* 屏蔽中断 */
    cpsid i
    extern Task_switchNextTask;
    /* 跳转到任务切换函数 */
    bl Task_switchNextTask
    /* 恢复中断 */
    cpsie i

    /* r2存放最近任务的地址, r3存放lr寄存器 */
    pop {r2, r3}

    /* 重新从最近任务中读取栈顶指针 */
    ldr r0, [r2]
    ldr r0, [r0]

    /* 偏移到高位寄存器 */
    adds r0, #16
    /* 从任务栈中加载对应寄存器 */
    ldmia r0!, {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7

    /* 将栈顶指针更新到PSP寄存器 */
    msr psp, r0

    /* 加载低位寄存器 */
    subs r0, #32
    ldmia r0!, {r4-r7}

    /* 跳转执行 */
    bx r3
    ALIGN
}
