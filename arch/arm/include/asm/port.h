/**
 * @file port.h
 * @author Zhiyelah
 * @brief 内核外设接口
 */

#ifndef _ZHIYEC_PORT_H
#define _ZHIYEC_PORT_H

#include <stdint.h>
#include <zhiyec/types.h>

/* 中断控制与状态寄存器 */
#define INTERRUPT_CTRL_REG (*((volatile uint32_t *)0xe000ed04))

/* PendSV中断位 */
#define PENDSV_SET_BIT (1UL << 28UL)

#define port_yield()                         \
    do {                                     \
        INTERRUPT_CTRL_REG = PENDSV_SET_BIT; \
        DSB();                               \
        ISB();                               \
    } while (0)

/**
 * @brief 任务栈初始化接口
 */
stack_t *port_init_task_stack(stack_t *top_of_stack, void (*const fn)(void *), void *const arg,
                              void (*return_handler)(void));

/**
 * @brief 任务跳转接口
 */
void port_start_first_task(void);

#endif /* _ZHIYEC_PORT_H */
