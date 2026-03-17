/**
 * @file irq.h
 * @author Zhiyelah
 * @brief 中断请求
 */

#ifndef _ZHIYEC_IRQ_H
#define _ZHIYEC_IRQ_H

#include <config.h>
#include <stdint.h>
#include <zhiyec/compiler.h>

#define SHIELDABLE_INTERRUPT_MAX_PRIORITY (CONFIG_SHIELDABLE_INTERRUPT_MAX_PRIORITY)

/**
 * @brief 禁用中断
 * @note 在保证不会嵌套调用该函数时使用
 */
static always_inline void irq_disable_without_nesting() {
    uint32_t new_basepri = SHIELDABLE_INTERRUPT_MAX_PRIORITY;
    __asm {
        msr basepri, new_basepri
        dsb
        isb
    }
}

/**
 * @brief 禁用中断
 * @note 含嵌套计数
 */
static always_inline void irq_disable() {
    extern volatile int interrupt_disabled_nesting;

    irq_disable_without_nesting();

    ++interrupt_disabled_nesting;
}

/**
 * @brief 禁用中断
 * @return 禁用前的中断屏蔽优先级
 * @note 中断安全的版本
 */
static always_inline uint32_t irq_disable_from_isr() {
    uint32_t prev_basepri;
    __asm {
        mrs prev_basepri, basepri
    }

    irq_disable();

    return prev_basepri;
}

/**
 * @brief 恢复中断
 * @param prev_basepri 禁用前的中断屏蔽优先级
 * @note 中断安全的版本
 */
static always_inline void irq_enable_from_isr(uint32_t prev_basepri) {
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
static always_inline void irq_enable() {
    irq_enable_from_isr(0U);
}

#endif /* _ZHIYEC_IRQ_H */
