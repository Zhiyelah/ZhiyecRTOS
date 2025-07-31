#include "Port.h"
#include "Config.h"
#include "Tick.h"
#include "Task.h"
#include "Hook.h"

#define SysTick_Handler_Port CONFIG_SYSTICK_HANDLER_PORT
#define PendSV_Handler_Port CONFIG_PENDSV_HANDLER_PORT
#define SVC_Handler_Port CONFIG_SVC_HANDLER_PORT
#define MANAGED_INTERRUPT_MAX_PRIORITY (CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY)

__asm bool Port_isUsingPSP() {
    mov r0, sp
    mrs r1, msp
    cmp r0, r1
    moveq r0, #0
    movne r0, #1
    bx lr
}

void SysTick_Handler_Port() {
    /* 默认以特权模式执行 */

#ifdef Hook_SysTick
    Hook_SysTick();
#endif

    /* 屏蔽中断 */
    uint32_t new_basepri = MANAGED_INTERRUPT_MAX_PRIORITY;
    __asm {
        msr basepri, new_basepri
        dsb
        isb
    }

    kernel_Tick_inc();

    extern volatile const unsigned int scheduling_suspended;
    extern volatile const struct TaskStruct *const current_task;
    if (scheduling_suspended == 0U && current_task->sched_method == SCHED_RR) {
        Interrupt_CTRL_Reg = PendSV_SET_Bit;
    }

    /* 恢复中断 */
    __asm {
        msr basepri, #0
    }
}

__asm void SVC_Handler_Port() {
    /* 默认以特权模式执行 */

    /* 8 字节对齐 */
    PRESERVE8

    /* 获取current_task的第一位成员 */
    ldr r0, =current_task
    ldr r0, [r0]
    ldr r0, [r0]

    /* 加载current_task栈顶指针中对应的寄存器 */
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
    ldr	r2, =current_task
    ldr	r1, [r2]

    /* 将进程堆栈指针保存到最近任务的堆栈指针 */
    str r0, [r1]

    /* 将current_task的地址和lr保存到内核堆栈MSP中 */
    stmdb sp!, {r2, lr}

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

    /* 从MSP中加载current_task的地址和lr寄存器 */
    /* 因为之前跳转过函数，寄存器的值会重置 */
    ldmia sp!, {r2, lr}

    /* 重新从最近任务中读取栈顶指针 */
    /* 此时为新的栈顶指针 */
    ldr r0, [r2]
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
