#include <../kernel/port.h>
#include <config.h>
#include <stdint.h>
#include <utility/time.h>

#define CPU_CLOCK_HZ (CONFIG_CPU_CLOCK_HZ)

void Time_delayUs(const Time_t us) {
    /* 使用SysTick实现微秒级延时 */
    const uint32_t ticks = us * (CPU_CLOCK_HZ / 1000000); // 计算需要的时钟周期数
    const uint32_t start = SysTick_VALUE_Reg;             // 当前计数值

    /* 等待指定的时钟周期数 */
    while ((start - SysTick_VALUE_Reg) % SysTick_LOAD_Reg < ticks) {
    }
}
