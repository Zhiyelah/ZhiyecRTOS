/**
 * @file Time.h
 * @author Zhiyelah
 * @brief 提供时间处理函数
 */

#ifndef _Time_h
#define _Time_h

#include "Tick.h"

typedef Tick_t Time_t;

void Time_delayUs(const Time_t us);

#endif /* _Time_h */
