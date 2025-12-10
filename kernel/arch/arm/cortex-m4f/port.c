#include "port.h"
#include <zhiyec/task.h>
#include <../kernel/hook.h>

#define SysTick_Handler_Port CONFIG_SYSTICK_HANDLER_PORT
#define SVC_Handler_Port CONFIG_SVC_HANDLER_PORT
#define PendSV_Handler_Port CONFIG_PENDSV_HANDLER_PORT
#define SHIELDABLE_INTERRUPT_MAX_PRIORITY (CONFIG_SHIELDABLE_INTERRUPT_MAX_PRIORITY)
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
    *(--top_of_stack) = ((stack_t)fn) & ((stack_t)0xFFFFFFFEul); /* 将Thumb指令地址转为函数的实际地址 */
    /* LR寄存器 */
    extern void TaskReturnHandler();
    *(--top_of_stack) = (stack_t)TaskReturnHandler;
    /* r12, r3, r2, r1寄存器 */
    top_of_stack -= 4;
    /* r0寄存器 */
    *(--top_of_stack) = (stack_t)arg;
    /* EXC_RETURN */
    *(--top_of_stack) = 0xFFFFFFFD;
    /* r11, r10, r9, r8, r7, r6, r5, r4寄存器 */
    top_of_stack -= 8;

    return top_of_stack;
}

__asm void StartFirstTask_Port() {
    PRESERVE8

    /* 将MSP设置为初始值 */
    ldr r0, =0xE000ED08
    ldr r0, [r0]
    ldr r0, [r0]
    msr msp, r0

    /* 清除表示FPU正在使用的标志位, 防止在SVC栈中预留不必要的空间 */
    mov r0, #0
    msr control, r0

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

__asm void PendSV_Handler_Port() {
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
    extern Task_switchNextTask;
    bl Task_switchNextTask
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
