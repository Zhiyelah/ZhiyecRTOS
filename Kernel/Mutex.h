/**
 * @file Mutex.h
 * @author Zhiyelah
 * @brief 互斥锁
 * @note 可选的模块, 依赖信号量模块
 */

#ifndef _Mutex_h
#define _Mutex_h

#include "Semaphore.h"
#include "Task.h"

struct Mutex {
    /* 基于信号量 */
    struct Semaphore sem;
    /* 持有锁的任务 */
    struct TaskStruct *owner;
    /* 持有锁任务的类型 */
    enum TaskType owner_type;
};

/**
 * @brief 初始化互斥锁
 * @param mutex 互斥锁对象
 */
void Mutex_init(struct Mutex *const mutex);

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
bool Mutex_tryLock(struct Mutex *const mutex, const Tick_t timeout);

/**
 * @brief 释放锁
 * @param mutex 互斥锁对象
 */
void Mutex_unlock(struct Mutex *const mutex);

#endif /* _Mutex_h */
