#include <../kernel/port.h>
#include <config.h>
#include <utility/delay.h>

#define CPU_CLOCK_HZ (CONFIG_CPU_CLOCK_HZ)

void Delay_us(const unsigned long us) {
    /* 指定的时钟周期数 */
    const unsigned long ticks = us * (CPU_CLOCK_HZ / 1000000);
    unsigned long count = 0;

    const uint32_t reload = SysTick_LOAD_Reg;
    uint32_t prev_value = SysTick_VALUE_Reg;

    /* 等待指定的时钟周期数 */
    while (true) {
        const uint32_t curr_value = SysTick_VALUE_Reg;

        if (curr_value != prev_value) {
            if (curr_value < prev_value) {
                count += prev_value - curr_value;
            } else {
                count += reload + prev_value - curr_value;
            }

            prev_value = curr_value;

            if (count >= ticks) {
                break;
            }
        }
    }
}
