/**
 * @file port.h
 * @author Zhiyelah
 * @brief 内核外设接口
 */

#ifndef _ZHIYEC_PORT_H
#define _ZHIYEC_PORT_H

#include <config.h>
#include <stdint.h>
#include <zhiyec/types.h>

/**
 * @brief 禁用中断
 */
static inline void Port_disableInterrupt() {
    extern volatile int interrupt_disabled_nesting;

    uint32_t new_basepri = CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY;
    __asm {
        msr basepri, new_basepri
        dsb
        isb
    }

    ++interrupt_disabled_nesting;
}

/**
 * @brief 禁用中断
 * @return 禁用前的中断屏蔽优先级
 * @note 中断安全的版本
 */
static inline uint32_t Port_disableInterruptFromISR() {
    uint32_t prev_basepri;
    __asm {
        mrs prev_basepri, basepri
    }
    Port_disableInterrupt();
    return prev_basepri;
}

/**
 * @brief 恢复中断
 * @param prev_basepri 禁用前的中断屏蔽优先级
 * @note 中断安全的版本
 */
static inline void Port_enableInterruptFromISR(uint32_t prev_basepri) {
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
static inline void Port_enableInterrupt() {
    Port_enableInterruptFromISR(0U);
}

/* 中断控制与状态寄存器 */
#define Interrupt_CTRL_Reg (*((volatile uint32_t *)0xe000ed04))

/* PendSV中断位 */
#define PendSV_SET_Bit (1UL << 28UL)

#define Port_yield()                         \
    do {                                     \
        Interrupt_CTRL_Reg = PendSV_SET_Bit; \
        __dsb(0U);                           \
        __isb(0U);                           \
    } while (0)

#define SysTick_LOAD_Reg (*((volatile uint32_t *)0xe000e014))
#define SysTick_VALUE_Reg (*((volatile uint32_t *)0xe000e018))

/**
 * @brief Tick初始化接口
 */
void InitSysTick_Port(void);

/**
 * @brief 堆栈初始化接口
 */
stack_t *InitTaskStack_Port(stack_t *top_of_stack, void (*const fn)(void *), void *const arg);

/**
 * @brief 任务跳转接口
 */
void StartFirstTask_Port(void);

#endif /* _ZHIYEC_PORT_H */
