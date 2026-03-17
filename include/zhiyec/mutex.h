/**
 * @file mutex.h
 * @author Zhiyelah
 * @brief 互斥锁
 * @note 可选的模块, 依赖信号量模块
 */

#ifndef _ZHIYEC_MUTEX_H
#define _ZHIYEC_MUTEX_H

#include <stdbool.h>
#include <zhiyec/task.h>
#include <zhiyec/tick.h>

struct mutex;

#define MUTEX_BYTE 28

/**
 * @brief 初始化互斥锁
 * @param mutex_mem 对象内存指针
 * @param ceiling_priority 天花板优先级(请求该锁的任务中最高的任务优先级)
 * @return 对象指针
 */
struct mutex *mutex_init(void *const mutex_mem, const enum task_priority ceiling_priority);

/**
 * @brief 获取所有者
 * @return 所有者对象指针
 */
struct task_struct *mutex_get_owner(struct mutex *const mutex);

/**
 * @brief 获得锁
 * @param mutex 互斥锁对象
 */
void mutex_lock(struct mutex *const mutex);

/**
 * @brief 尝试获得锁
 * @param mutex 互斥锁对象
 * @param timeout 超时时间
 * @return 是否成功获得锁
 */
bool mutex_try_lock(struct mutex *const mutex, const tick_t timeout);

/**
 * @brief 释放锁
 * @param mutex 互斥锁对象
 */
void mutex_unlock(struct mutex *const mutex);

#endif /* _ZHIYEC_MUTEX_H */
