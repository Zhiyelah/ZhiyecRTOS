/**
 * @file semaphore.h
 * @author Zhiyelah
 * @brief 信号量
 * @note 可选的模块
 */

#ifndef _ZHIYEC_SEMAPHORE_H
#define _ZHIYEC_SEMAPHORE_H

#include <stdbool.h>
#include <zhiyec/tick.h>

struct semaphore;

#define SEMAPHORE_BYTE 16

/**
 * @brief 初始化为二值信号量
 * @param sem_mem 对象内存指针
 * @return 对象指针
 */
struct semaphore *semaphore_init_binary(void *const sem_mem);

/**
 * @brief 初始化为计数信号量
 * @param sem_mem 对象内存指针
 * @param max_value 最大计数值
 * @param init_value 初始计数值
 * @return 对象指针
 */
struct semaphore *semaphore_init_counting(void *const sem_mem,
                                          const int max_value, const unsigned int init_value);

/**
 * @brief 获得信号量
 * @param sem 信号量对象
 */
void semaphore_acquire(struct semaphore *const sem);

/**
 * @brief 尝试获得信号量, 超时后返回
 * @param sem 信号量对象
 * @param timeout 超时时间
 * @return 是否成功获得信号量
 */
bool semaphore_try_acquire(struct semaphore *const sem, tick_t timeout);

/**
 * @brief 释放信号量
 * @param sem 信号量对象
 */
void semaphore_release(struct semaphore *const sem);

/**
 * @brief 获得信号量
 * @param sem 信号量对象
 * @note 中断安全的版本
 */
bool semaphore_acquire_from_isr(struct semaphore *const sem);

/**
 * @brief 释放信号量
 * @param sem 信号量对象
 * @note 中断安全的版本
 */
void semaphore_release_from_isr(struct semaphore *const sem);

#endif /* _ZHIYEC_SEMAPHORE_H */
