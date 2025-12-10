/**
 * @file mutex.h
 * @author Zhiyelah
 * @brief 互斥锁
 * @note 可选的模块, 依赖信号量模块
 */

#ifndef _ZHIYEC_MUTEX_H
#define _ZHIYEC_MUTEX_H

#include <stdbool.h>
#include <zhiyec/tick.h>

struct Mutex;

#define Mutex_byte 28

/**
 * @brief 初始化互斥锁
 * @param mutex_mem 对象内存指针
 * @return 对象指针
 */
struct Mutex *Mutex_init(void *const mutex_mem);

/**
 * @brief 获取所有者
 * @return 所有者对象指针
 */
struct TaskStruct *Mutex_getOwner(struct Mutex *const mutex);

/**
 * @brief 获得锁
 * @param mutex 互斥锁对象
 */
void Mutex_lock(struct Mutex *const mutex);

/**
 * @brief 尝试获得锁
 * @param mutex 互斥锁对象
 * @param timeout 超时时间
 * @return 是否成功获得锁
 */
bool Mutex_tryLock(struct Mutex *const mutex, const tick_t timeout);

/**
 * @brief 释放锁
 * @param mutex 互斥锁对象
 */
void Mutex_unlock(struct Mutex *const mutex);

#endif /* _ZHIYEC_MUTEX_H */
