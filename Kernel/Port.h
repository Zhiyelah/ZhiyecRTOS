/**
 * @file Port.h
 * @author Zhiyelah
 * @brief 内核外设接口
 */

#ifndef _Port_h
#define _Port_h

#include "Config.h"
#include "Defines.h"
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
static __forceinline inline void Port_disableInterrupt() {
    extern volatile int interrupt_disabled_nesting;

    uint32_t new_basepri = MANAGED_INTERRUPT_MAX_PRIORITY;
    __asm {
        msr basepri, new_basepri
        dsb
        isb
    }

    ++interrupt_disabled_nesting;
}

/**
 * @brief 禁用中断
 * @note 中断安全的版本
 */
static __forceinline inline uint32_t Port_disableInterruptFromISR() {
    uint32_t prev_basepri;
    __asm {
        mrs prev_basepri, basepri
    }
    Port_disableInterrupt();
    return prev_basepri;
}

/**
 * @brief 恢复中断
 * @note 中断安全的版本
 */
static __forceinline inline void Port_enableInterruptFromISR(uint32_t prev_basepri) {
    extern volatile int interrupt_disabled_nesting;

    --interrupt_disabled_nesting;

    if (interrupt_disabled_nesting == 0) {
        __asm {
            msr basepri, prev_basepri
        }
    }
}

/**
 * @brief 恢复中断
 */
static __forceinline inline void Port_enableInterrupt() {
    Port_enableInterruptFromISR(0U);
}

/**
 * @brief 堆栈初始化接口
 */
Stack_t *InitTaskStack_Port(Stack_t *top_of_stack, void (*const fn)(void *), void *const arg);

/**
 * @brief 任务执行接口
 */
void StartFirstTask_Port(void);

#endif /* _Port_h */
