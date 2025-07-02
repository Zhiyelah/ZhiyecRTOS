/**
 * @file Config.h
 * @author Zhiyelah
 * @brief 配置文件
 */

#ifndef _ZhiyembeConfig_h
#define _ZhiyembeConfig_h

/* 配置中断频率 */
#define CONFIG_INTERRUPT_HZ 1000

/* 配置CPU时钟频率 */
#define CONFIG_CPU_CLOCK_HZ 72000000

/* 配置SysTick中断入口 */
#define CONFIG_SYSTICK_HANDLER_PORT SysTick_Handler

/* 配置PenSV中断入口 */
#define CONFIG_PENDSV_HANDLER_PORT PendSV_Handler

/* 配置SVC中断入口 */
#define CONFIG_SVC_HANDLER_PORT SVC_Handler

/* 配置任务最大数量 */
#define CONFIG_TASK_MAX_NUM 32

/* 配置中断系统调用的最大优先级 */
#define CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* 配置内核中断优先级(SysTick等) */
#define CONFIG_KERNEL_INTERRUPT_PRIORITY (CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY - 1)

#endif /* _ZhiyembeConfig_h */
