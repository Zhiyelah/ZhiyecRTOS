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
    ldmia r0!, {r4-r11, lr}
    msr psp, r0
    isb

    /* 恢复中断 */
    mov r0, #0
    msr basepri, r0

    /* 从PSP中恢复寄存器 */
    bx lr
}

__asm void PENDSV_HANDLER_PORT() {
    PRESERVE8

    /* 获取进程堆栈指针 */
    mrs r0, psp
    isb

    /* 如果使用FPU, 保存浮点寄存器 */
    tst lr, #0x10
    it eq
    vstmdbeq r0!, {s16-s31}

    /* 保存寄存器 */
    stmdb r0!, {r4-r11, lr}

    /* 保存当前堆栈 */
    extern kernel_current_task;
    ldr	r3, =kernel_current_task
    ldr	r1, [r3]
    str r0, [r1]

    /* 选择下一个任务 */
    stmdb sp!, {r3}
    mov r0, #SHIELDABLE_INTERRUPT_MAX_PRIORITY
    msr basepri, r0
    dsb
    isb
    extern task_switch_next;
    bl task_switch_next
    mov r0, #0
    msr basepri, r0
    ldmia sp!, {r3}

    /* 恢复下一个任务的堆栈 */
    ldr r0, [r3]
    ldr r0, [r0]
    ldmia r0!, {r4-r11, lr}

    /* 如果使用了FPU, 恢复浮点寄存器 */
    tst lr, #0x10
    it eq
    vldmiaeq r0!, {s16-s31}

    /* 将栈顶指针更新到PSP寄存器 */
    msr psp, r0
    isb

    /* 跳转执行 */
    bx lr
    nop
    ALIGN
}
