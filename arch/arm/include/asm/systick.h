/**
 * @file systick.h
 * @author Zhiyelah
 * @brief 系统滴答定时器
 */

#ifndef _ZHIYEC_SYSTICK_H
#define _ZHIYEC_SYSTICK_H

#include <config.h>
#include <stdint.h>

#define SYSTICK_LOAD_REG_VALUE (CONFIG_CPU_CLOCK_HZ / CONFIG_SYSTICK_RATE_HZ)

/* SysTick寄存器 */
#define SYSTICK_CTRL_REG (*((volatile uint32_t *)0xe000e010))
#define SYSTICK_LOAD_REG (*((volatile uint32_t *)0xe000e014))
#define SYSTICK_VALUE_REG (*((volatile uint32_t *)0xe000e018))

/* SysTick控制寄存器位定义 */
#define SYSTICK_ENABLE_BIT (1UL << 0UL)
#define SYSTICK_INT_BIT (1UL << 1UL)
#define SYSTICK_CLK_BIT (1UL << 2UL)

/* 优先级寄存器 */
#define SHPR3_REG (*((volatile uint32_t *)0xe000ed20))
/* 最小优先级 */
#define MIN_INTERRUPT_PRIORITY (0xFFul)

/* SysTick和PendSV的优先级 */
#define SHPR3_PENDSV_PRIORITY (((uint32_t)MIN_INTERRUPT_PRIORITY) << 16UL)
#define SHPR3_SYSTICK_PRIORITY (((uint32_t)CONFIG_KERNEL_INTERRUPT_PRIORITY) << 24UL)

static inline void systick_init() {
    /* 配置SysTick及PendSV优先级 */
    SHPR3_REG |= SHPR3_PENDSV_PRIORITY;
    SHPR3_REG |= SHPR3_SYSTICK_PRIORITY;
    /* 配置SysTick */
    SYSTICK_CTRL_REG = 0;
    SYSTICK_LOAD_REG = (SYSTICK_LOAD_REG_VALUE & 0xFFFFFFul) - 1;
    SYSTICK_VALUE_REG = 0;
    SYSTICK_CTRL_REG = SYSTICK_ENABLE_BIT | SYSTICK_INT_BIT | SYSTICK_CLK_BIT;
}

#endif /* _ZHIYEC_SYSTICK_H */
