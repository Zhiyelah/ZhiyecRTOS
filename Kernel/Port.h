/**
 * @file Port.h
 * @author Zhiyelah
 * @brief 内核外设接口
 */

#ifndef _Port_h
#define _Port_h

#include <stdint.h>

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
#define Min_Interrupt_Priority (0xFFul)

/* SysTick和PendSV的优先级 */
#define SHPR3_PENDSV_Priority (((uint32_t)Min_Interrupt_Priority) << 16UL)
#define SHPR3_SYSTICK_Priority (((uint32_t)(CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY - 1)) << 24UL)

/* 中断控制与状态寄存器 */
#define Interrupt_CTRL_Reg (*((volatile uint32_t *)0xe000ed04))

/* PendSV中断位 */
#define PendSV_SET_Bit (1UL << 28UL)

#endif /* _Port_h */
