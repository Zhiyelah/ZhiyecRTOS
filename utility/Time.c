#include <../kernel/Port.h>
#include <Config.h>
#include <stdint.h>
#include <utility/Time.h>

#define CPU_CLOCK_HZ (CONFIG_CPU_CLOCK_HZ)

void Time_delayUs(const Time_t us) {
    extern uint32_t GetSysTickLoadReg_Port(void);
    extern uint32_t GetSysTickValueReg_Port(void);

    /* 使用SysTick实现微秒级延时 */
    const uint32_t ticks = us * (CPU_CLOCK_HZ / 1000000); // 计算需要的时钟周期数
    const uint32_t start = GetSysTickValueReg_Port();     // 当前计数值

    /* 等待指定的时钟周期数 */
    while ((start - GetSysTickValueReg_Port()) % GetSysTickLoadReg_Port() < ticks) {
    }
}
