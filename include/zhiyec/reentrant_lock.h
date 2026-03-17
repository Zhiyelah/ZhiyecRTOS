/**
 * @file reentrant_lock.h
 * @author Zhiyelah
 * @brief 可重入锁
 * @note 可选的模块, 依赖互斥锁模块
 */

#ifndef _ZHIYEC_REENTRANTLOCK_H
#define _ZHIYEC_REENTRANTLOCK_H

#include <stdbool.h>
#include <zhiyec/task.h>
#include <zhiyec/tick.h>

struct reentrantlock;

#define REENTRANTLOCK_BYTE 36

/**
 * @brief 初始化可重入锁
 * @param lock_mem 对象内存指针
 * @param ceiling_priority 天花板优先级(请求该锁的任务中最高的任务优先级)
 * @return 对象指针
 */
struct reentrantlock *reentrantlock_init(void *const lock_mem, const enum task_priority ceiling_priority);

/**
 * @brief 获得锁
 * @param lock 可重入锁对象
 */
void reentrantlock_lock(struct reentrantlock *const lock);

/**
 * @brief 尝试获得锁
 * @param lock 可重入锁对象
 * @param timeout 超时时间
 * @return 是否成功获得锁
 */
bool reentrantlock_try_lock(struct reentrantlock *const lock, const tick_t timeout);

/**
 * @brief 释放锁
 * @param lock 可重入锁对象
 */
void reentrantlock_unlock(struct reentrantlock *const lock);

#endif /* _ZHIYEC_REENTRANTLOCK_H */
