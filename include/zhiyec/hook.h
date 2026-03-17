/**
 * @file hook.h
 * @author Zhiyelah
 * @brief 内核钩子
 * @note 可选的模块
 */

#ifndef _ZHIYEC_HOOK_H
#define _ZHIYEC_HOOK_H

/* 注册钩子 */

/* SysTick中断钩子 */
#define hook_isr_systick_entry() ((void)0)

/* 空闲任务钩子 */
#define hook_idle_task_running() ((void)0)

#endif /* _ZHIYEC_HOOK_H */
