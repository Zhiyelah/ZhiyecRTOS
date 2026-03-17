#include <zhiyec/types.h>

/* 初始化任务栈接口 */
stack_t *port_init_task_stack(stack_t *top_of_stack, void (*const fn)(void *), void *const arg,
                              void (*return_handler)(void)) {
    /* xPSR寄存器 */
    *(--top_of_stack) = 0x01000000; /* 以Thumb指令模式执行(内存受限场景, Cortex-M平台要求) */
    /* PC寄存器 */
    *(--top_of_stack) = ((stack_t)fn) & ((stack_t)0xFFFFFFFEul); /* 将Thumb指令地址转为函数的实际地址 */
    /* LR寄存器 */
    *(--top_of_stack) = (stack_t)return_handler;
    /* r12, r3, r2, r1寄存器 */
    top_of_stack -= 4;
    /* r0寄存器 */
    *(--top_of_stack) = (stack_t)arg;
    /* r11, r10, r9, r8, r7, r6, r5, r4寄存器 */
    top_of_stack -= 8;

    return top_of_stack;
}

__asm void port_start_first_task() {
    PRESERVE8

    /* 将MSP设置为初始值 */
    ldr r0, =0xE000ED08
    ldr r0, [r0]
    ldr r0, [r0]
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
