/**
 * @file Config.h
 * @author Zhiyelah
 * @brief 配置文件
 */

#ifndef _ZhiyembeConfig_h
#define _ZhiyembeConfig_h

/* 配置Tick中断频率 */
#define CONFIG_SYSTICK_RATE_HZ 1000

/* 配置CPU时钟频率 */
#define CONFIG_CPU_CLOCK_HZ 72000000

/* 配置SysTick中断入口 */
#define CONFIG_SYSTICK_HANDLER_PORT SysTick_Handler

/* 配置PenSV中断入口 */
#define CONFIG_PENDSV_HANDLER_PORT PendSV_Handler

/* 配置SVC中断入口 */
#define CONFIG_SVC_HANDLER_PORT SVC_Handler

/* 配置任务最大数量(仅在静态内存分配时有效) */
#define CONFIG_TASK_MAX_NUM 32

/* 配置受管理中断的最大优先级 */
#define CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY 5

/* 配置内核中断优先级(SysTick等) */
#define CONFIG_KERNEL_INTERRUPT_PRIORITY (CONFIG_MANAGED_INTERRUPT_MAX_PRIORITY - 1)

/* 是否使用动态内存分配 */
#define USE_DYNAMIC_MEMORY_ALLOCATION 0

/*      配置内存池大小(当启用动态内存分配时有效) */
/*   */ #define CONFIG_MEMORYPOOL_SIZE 4096

/*      配置默认的任务栈大小(启用动态内存分配且未指定任务栈大小时) */
/*   */ #define CONFIG_DEFAULT_TASK_STACK_SIZE 128

#endif /* _ZhiyembeConfig_h */
