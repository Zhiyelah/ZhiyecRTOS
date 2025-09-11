#include "Port.h"
#include "Task.h"
#include "Hook.h"

#define SysTick_Handler_Port CONFIG_SYSTICK_HANDLER_PORT
#define SVC_Handler_Port CONFIG_SVC_HANDLER_PORT
#define PendSV_Handler_Port CONFIG_PENDSV_HANDLER_PORT
#define MANAGED_INTERRUPT_MAX_PRIORITY (CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY)

/* 初始化任务栈接口 */
Stack_t *InitTaskStack_Port(Stack_t *top_of_stack, void (*const fn)(void *), void *const arg) {
    /* xPSR寄存器 */
    *(--top_of_stack) = 0x01000000; /* 以Thumb指令模式执行(内存受限场景, Cortex-M平台要求) */
    /* PC寄存器 */
    *(--top_of_stack) = ((Stack_t)fn) & ((Stack_t)0xFFFFFFFEul); /* 将Thumb指令地址转为函数的实际地址 */
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

void SysTick_Handler_Port() {
    /* 默认以特权模式执行 */

#ifdef Hook_isrSysTickEntry
    Hook_isrSysTickEntry();
#endif

    uint32_t prev_basepri = Port_disableInterruptFromISR();

    ++kernel_ticks;

    if (Task_needSwitch()) {
        Interrupt_CTRL_Reg = PendSV_SET_Bit;
    }

    Port_enableInterruptFromISR(prev_basepri);
}

__asm void SVC_Handler_Port() {
    /* 默认以特权模式执行 */

    /* 8 字节对齐 */
    PRESERVE8

    /* 获取最近任务的栈顶指针 */
    ldr r0, =current_task
    ldr r0, [r0]
    ldr r0, [r0]

    /* 加载栈顶指针中对应的寄存器 */
    ldmia r0!, {r4-r11}

    /* 设置进程堆栈指针为第一个任务的栈顶指针 */
    msr psp, r0
    isb

    /* 恢复中断 */
    mov r0, #0
    msr basepri, r0

    /* 使用Thumb指令(Cortex-M仅支持Thumb指令) */
    /* 使用PSP作为堆栈指针，并切换为用户级 */
    orr lr, #0x0D

    /* 从PSP中恢复寄存器 */
    bx lr
}

__asm void PendSV_Handler_Port() {
    /* 默认以特权模式执行 */

    /* 8 字节对齐 */
    PRESERVE8

    /* 获取进程堆栈指针 */
    mrs r0, psp
    /* 确保PSP为最新 */
    isb

    /* 除PendSV自动保存的寄存器外，将r4~r11保存到当前任务的任务栈中 */
    stmdb r0!, {r4-r11}

    extern current_task;

    /* 获取最近任务指向的结构体 */
    ldr	r3, =current_task
    ldr	r1, [r3]

    /* 将进程堆栈指针保存到最近任务的堆栈指针 */
    str r0, [r1]

    /* 将最近任务的地址和lr保存到内核堆栈MSP中 */
    stmdb sp!, {r3, lr}

    /* 屏蔽受管理的中断 */
    mov r0, #MANAGED_INTERRUPT_MAX_PRIORITY
    msr basepri, r0
    dsb
    isb

    extern Task_switchNextTask;

    /* 跳转到任务切换函数 */
    bl Task_switchNextTask

    /* 恢复中断 */
    mov r0, #0
    msr basepri, r0

    /* 从MSP中加载之前保存的最近任务的地址和lr寄存器 */
    /* 因为之前跳转过函数，寄存器的值会重置 */
    ldmia sp!, {r3, lr}

    /* 重新从最近任务中读取栈顶指针 */
    /* 此时为新的栈顶指针 */
    ldr r0, [r3]
    ldr r0, [r0]

    /* 从任务栈中加载对应寄存器 */
    ldmia r0!, {r4-r11}

    /* 将栈顶指针更新到PSP寄存器 */
    msr psp, r0
    isb

    /* 跳转执行 */
    bx lr
    nop
}
