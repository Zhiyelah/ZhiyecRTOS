#include <zhiyec/types.h>

/* 初始化任务栈接口 */
stack_t *port_init_task_stack(stack_t *top_of_stack, void (*const fn)(void *), void *const arg,
                              void (*return_handler)(void)) {
    /* xPSR寄存器 */
    *(--top_of_stack) = 0x01000000; /* 以Thumb指令模式执行(内存受限场景, Cortex-M平台要求) */
    /* PC寄存器 */
    *(--top_of_stack) = (stack_t)fn;
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
