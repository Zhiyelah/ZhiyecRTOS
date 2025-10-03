/**
 * @file Semaphore.h
 * @author Zhiyelah
 * @brief 信号量
 * @note 可选的模块
 */

#ifndef _Semaphore_h
#define _Semaphore_h

#include <stdbool.h>
#include <zhiyec/Tick.h>

struct Semaphore;

#define Semaphore_byte 16

/**
 * @brief 初始化为二值信号量
 * @param sem_mem 对象内存指针
 * @return 对象指针
 */
struct Semaphore *Semaphore_initBinary(void *const sem_mem);

/**
 * @brief 初始化为计数信号量
 * @param sem_mem 对象内存指针
 * @param max_value 最大计数值
 * @param init_value 初始计数值
 * @return 对象指针
 */
struct Semaphore *Semaphore_initCounting(void *const sem_mem,
                                         const int max_value, const unsigned int init_value);

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
bool Semaphore_tryAcquire(struct Semaphore *const sem, tick_t timeout);

/**
 * @brief 释放信号量
 * @param sem 信号量对象
 * @note 可在ISR中调用
 */
void Semaphore_release(struct Semaphore *const sem);

#endif /* _Semaphore_h */
