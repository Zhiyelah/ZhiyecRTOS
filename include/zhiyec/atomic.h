/**
 * @file atomic.h
 * @author Zhiyelah
 * @brief 原子操作支持
 */

#ifndef _ZHIYEC_ATOMIC_H
#define _ZHIYEC_ATOMIC_H

#include <asm/irq.h>

/**
 * @brief 开始原子操作
 */
#define atomic_begin() irq_disable()

/**
 * @brief 结束原子操作
 */
#define atomic_end() irq_enable()

/**
 * @brief 原子操作块
 */
#define atomic(code_block)         \
    do {                           \
        atomic_begin();            \
        {code_block} atomic_end(); \
    } while (0)

#endif /* _ZHIYEC_ATOMIC_H */
