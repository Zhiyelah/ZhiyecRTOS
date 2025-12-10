/**
 * @file reentrant_lock.h
 * @author Zhiyelah
 * @brief 可重入锁
 * @note 可选的模块, 依赖互斥锁模块
 */

#ifndef _ZHIYEC_REENTRANTLOCK_H
#define _ZHIYEC_REENTRANTLOCK_H

#include <stdbool.h>
#include <zhiyec/tick.h>

struct ReentrantLock;

#define ReentrantLock_byte 36

/**
 * @brief 初始化可重入锁
 * @param lock_mem 对象内存指针
 * @return 对象指针
 */
struct ReentrantLock *ReentrantLock_init(void *const lock_mem);

/**
 * @brief 获得锁
 * @param lock 可重入锁对象
 */
void ReentrantLock_lock(struct ReentrantLock *const lock);

/**
 * @brief 尝试获得锁
 * @param lock 可重入锁对象
 * @param timeout 超时时间
 * @return 是否成功获得锁
 */
bool ReentrantLock_tryLock(struct ReentrantLock *const lock, const tick_t timeout);

/**
 * @brief 释放锁
 * @param lock 可重入锁对象
 */
void ReentrantLock_unlock(struct ReentrantLock *const lock);

#endif /* _ZHIYEC_REENTRANTLOCK_H */
