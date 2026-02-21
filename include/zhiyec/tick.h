/**
 * @file tick.h
 * @author Zhiyelah
 * @brief 内核Tick支持
 */

#ifndef _ZHIYEC_TICK_H
#define _ZHIYEC_TICK_H

#include <config.h>
#include <stdbool.h>
#include <zhiyec/types.h>

#define SYSTICK_RATE_HZ (CONFIG_SYSTICK_RATE_HZ)

extern volatile tick_t kernel_ticks;

/**
 * @brief 获取当前Tick
 */
#define Tick_currentTick() ((tick_t)kernel_ticks)

/**
 * @brief 毫秒转Tick
 */
#define Tick_fromMsec(ms) ((tick_t)(((ms) * SYSTICK_RATE_HZ + (1000 - 1)) / 1000))

/**
 * @brief Tick转毫秒
 */
#define Tick_toMsec(ticks) ((unsigned int)(((ticks) * 1000UL + (SYSTICK_RATE_HZ - 1)) / SYSTICK_RATE_HZ))

/**
 * @brief 检测是否超时(含溢出处理)
 * @return 超时返回true
 */
#define Tick_after(current_ticks, target_ticks) ((bool)((stick_t)((target_ticks) - (current_ticks)) < 0))

#endif /* _ZHIYEC_TICK_H */
