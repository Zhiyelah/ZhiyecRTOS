/**
 * @file Tick.h
 * @author Zhiyelah
 * @brief 内核滴答支持
 */

#ifndef _ZHIYEC_TICK_H
#define _ZHIYEC_TICK_H

#include <Config.h>
#include <stdbool.h>
#include <zhiyec/Types.h>

#define SYSTICK_RATE_HZ (CONFIG_SYSTICK_RATE_HZ)

extern volatile tick_t kernel_ticks;

/**
 * @brief 获取当前ticks
 */
#define Tick_currentTicks() ((tick_t)kernel_ticks)

/**
 * @brief 毫秒转ticks
 */
#define Tick_fromMsec(ms) ((tick_t)(((ms) * SYSTICK_RATE_HZ + (1000 - 1)) / 1000))

/**
 * @brief ticks转毫秒
 */
#define Tick_toMsec(ticks) ((unsigned int)(((ticks) * 1000UL + (SYSTICK_RATE_HZ - 1)) / SYSTICK_RATE_HZ))

/**
 * @brief 检测是否超时, 溢出处理
 */
#define Tick_after(current_ticks, target_ticks) ((bool)((stick_t)((target_ticks) - (current_ticks)) < 0))

#endif /* _ZHIYEC_TICK_H */
