/**
 * @file atomic.h
 * @author Zhiyelah
 * @brief 原子操作支持
 */

#ifndef _ZHIYEC_ATOMIC_H
#define _ZHIYEC_ATOMIC_H

#include <../kernel/port.h>

/**
 * @brief 开始原子操作
 */
#define Atomic_begin() Port_disableInterrupt()

/**
 * @brief 结束原子操作
 */
#define Atomic_end() Port_enableInterrupt()

/**
 * @brief 原子操作块
 */
#define atomic(code_block) \
    do {                   \
        Atomic_begin();    \
        {code_block}       \
        Atomic_end();      \
    } while (0)

#endif /* _ZHIYEC_ATOMIC_H */
