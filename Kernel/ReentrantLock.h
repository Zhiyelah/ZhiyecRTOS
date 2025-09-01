/**
 * @file ReentrantLock.h
 * @author Zhiyelah
 * @brief 可重入锁
 * @note 可选的模块, 依赖互斥锁模块
 */

#ifndef _ReentrantLock_h
#define _ReentrantLock_h

#include "Mutex.h"

struct ReentrantLock {
    /* 基于互斥锁 */
    struct Mutex mutex;
    /* 锁计数器 */
    volatile int state;
};

/**
 * @brief 初始化可重入锁
 * @param lock 可重入锁对象
 */
void ReentrantLock_init(struct ReentrantLock *const lock);

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
bool ReentrantLock_tryLock(struct ReentrantLock *const lock, const Tick_t timeout);

/**
 * @brief 释放锁
 * @param lock 可重入锁对象
 */
void ReentrantLock_unlock(struct ReentrantLock *const lock);

#endif /* _ReentrantLock_h */
