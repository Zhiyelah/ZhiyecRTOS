/**
 * @file Tick.h
 * @author Zhiyelah
 * @brief 系统时基
 */

#ifndef _Tick_h
#define _Tick_h

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t Tick_t;
typedef int32_t STick_t;

/**
 * @brief 获取当前ticks
 */
Tick_t Tick_currentTicks(void);

/**
 * @brief 毫秒转ticks
 */
Tick_t Tick_fromMsec(const unsigned int ms);

/**
 * @brief ticks转毫秒
 */
unsigned int Tick_toMsec(const Tick_t ticks);

/**
 * @brief 检测是否超时, 溢出处理
 */
bool Tick_after(const Tick_t current_ticks, const Tick_t target_ticks);

/**
 * 增加tick, 内核调用
 */
void kernel_Tick_inc(void);

#endif /* _Tick_h */
