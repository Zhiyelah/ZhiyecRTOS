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
    PRESERVE8

    /* 加载第一个任务 */
    ldr r0, =kernel_current_task
    ldr r0, [r0]
    ldr r0, [r0]
    ldmia r0!, {r4-r11}
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

__asm void PENDSV_HANDLER_PORT() {
    PRESERVE8

    /* 获取进程堆栈指针 */
    mrs r0, psp
    isb

    /* 保存寄存器 */
    stmdb r0!, {r4-r11}

    /* 保存当前堆栈 */
    extern kernel_current_task;
    ldr	r3, =kernel_current_task
    ldr	r1, [r3]
    str r0, [r1]

    /* 选择下一个任务 */
    stmdb sp!, {r3, lr}
    mov r0, #SHIELDABLE_INTERRUPT_MAX_PRIORITY
    msr basepri, r0
    dsb
    isb
    extern task_switch_next;
    bl task_switch_next
    mov r0, #0
    msr basepri, r0
    ldmia sp!, {r3, lr}

    /* 恢复下一个任务的堆栈 */
    ldr r0, [r3]
    ldr r0, [r0]
    ldmia r0!, {r4-r11}
    msr psp, r0
    isb

    /* 跳转执行 */
    bx lr
    nop
}
