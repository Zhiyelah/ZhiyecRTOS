/**
 * @file Hook.h
 * @author Zhiyelah
 * @brief 内核钩子(可选的)
 */

#ifndef _Hook_h
#define _Hook_h

/* 注册钩子 */

/* SysTick中断钩子 */
#define Hook_enterSysTickISR() ((void)0)

/* 空闲任务钩子 */
#define Hook_runIdleTask() ((void)0)

/* 任务删除钩子 */
#define Hook_deleteTask(task) ((void)0)

#endif /* _Hook_h */
