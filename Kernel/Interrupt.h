/**
 * @file Interrupt.h
 * @author Zhiyelah
 * @brief 中断管理
 */

#ifndef _Interrupt_h
#define _Interrupt_h

#include "Config.h"
#include <stdint.h>

#define MAX_SYSCALL_INTERRUPT_PRIORITY (CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY)

extern unsigned int __reentry_count;

/**
 * @brief 禁用中断
 */
static __forceinline void __disable_interrupt() {
    uint32_t new_basepri = MAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm {
        msr basepri, new_basepri
        dsb
        isb
    }
}

/**
 * @brief 恢复中断
 */
static __forceinline void __enable_interrupt() {
    __asm {
        msr basepri, #0
    }
}

/**
 * @brief 可重入的禁用中断
 */
static __forceinline void disable_interrupt_reentrant() {
    /* 屏蔽中断 */
    __disable_interrupt();

    __reentry_count++;
}

/**
 * @brief 可重入的恢复中断
 */
static __forceinline void enable_interrupt_reentrant() {
    if (__reentry_count > 0) {
        __reentry_count--;
    }

    if (__reentry_count == 0) {
        /* 恢复中断 */
        __enable_interrupt();
    }
}

#endif /* _Interrupt_h */
