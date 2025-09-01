/**
 * @file Semaphore.h
 * @author Zhiyelah
 * @brief 信号量
 * @note 可选的模块
 */

#ifndef _Semaphore_h
#define _Semaphore_h

#include "QueueList.h"
#include "Tick.h"
#include <stdbool.h>

struct Semaphore {
    /* 信号量状态 */
    volatile int state;
    /* 等待获得信号量的任务 */
    struct QueueList tasks_waiting_to_acquire;
};

/**
 * @brief 初始化为二值信号量
 * @param sem 信号量对象
 */
void Semaphore_initBinary(struct Semaphore *const sem);

/**
 * @brief 初始化为计数信号量
 * @param sem 信号量对象
 * @param count 初始计数值
 */
void Semaphore_initCounting(struct Semaphore *const sem, const unsigned int count);

/**
 * @brief 获得信号量
 * @param sem 信号量对象
 */
void Semaphore_acquire(struct Semaphore *const sem);

/**
 * @brief 尝试获得信号量, 超时后返回
 * @param sem 信号量对象
 * @param timeout 超时时间
 * @return 是否成功获得信号量
 */
bool Semaphore_tryAcquire(struct Semaphore *const sem, const Tick_t timeout);

/**
 * @brief 释放信号量
 * @param sem 信号量对象
 * @note 可在ISR中调用
 */
void Semaphore_release(struct Semaphore *const sem);

#endif /* _Semaphore_h */
