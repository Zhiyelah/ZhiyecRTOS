#include <zhiyec/task.h>
#include <zhiyec/hook.h>
#include <asm/irq.h>

#define SYSTICK_HANDLER_PORT CONFIG_SYSTICK_HANDLER_PORT
#define SVC_HANDLER_PORT CONFIG_SVC_HANDLER_PORT
#define PENDSV_HANDLER_PORT CONFIG_PENDSV_HANDLER_PORT

void SYSTICK_HANDLER_PORT() {
#ifdef hook_isr_systick_entry
    hook_isr_systick_entry();
#endif

    uint32_t prev_basepri = irq_disable_from_isr();

    if (task_need_switch()) {
        INTERRUPT_CTRL_REG = PENDSV_SET_BIT;
    }

    irq_enable_from_isr(prev_basepri);
}

__asm void SVC_HANDLER_PORT() {
}

__asm void PENDSV_HANDLER_PORT() {
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
    extern task_switch_next;
    bl task_switch_next
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
