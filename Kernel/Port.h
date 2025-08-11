/**
 * @file Port.h
 * @author Zhiyelah
 * @brief 内核外设接口
 */

#ifndef _Port_h
#define _Port_h

#include "Config.h"
#include <stdbool.h>
#include <stdint.h>

/* 内存字节对齐位 */
#define BYTE_ALIGNMENT 8

/* ARM-CM0 to ARM-CM7 */
/* SysTick寄存器 */
#define SysTick_CTRL_Reg (*((volatile uint32_t *)0xe000e010))
#define SysTick_LOAD_Reg (*((volatile uint32_t *)0xe000e014))
#define SysTick_VALUE_Reg (*((volatile uint32_t *)0xe000e018))

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

/* 中断控制与状态寄存器 */
#define Interrupt_CTRL_Reg (*((volatile uint32_t *)0xe000ed04))

/* PendSV中断位 */
#define PendSV_SET_Bit (1UL << 28UL)

#define MANAGED_INTERRUPT_MAX_PRIORITY (CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY)

/**
 * @brief 禁用中断
 */
static __forceinline void Port_disableInterrupt() {
    uint32_t new_basepri = MANAGED_INTERRUPT_MAX_PRIORITY;
    __asm {
        msr basepri, new_basepri
        dsb
        isb
    }
}

/**
 * @brief 恢复中断
 */
static __forceinline void Port_enableInterrupt() {
    __asm {
        msr basepri, #0
    }
}

/**
 * @brief 判断当前堆栈指针是否为PSP
 * @return 如果当前堆栈指针为PSP则返回true, 否则返回false
 */
bool Port_isUsingPSP(void);

#endif /* _Port_h */
