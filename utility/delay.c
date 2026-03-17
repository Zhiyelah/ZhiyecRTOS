#include <asm/systick.h>
#include <config.h>
#include <utility/delay.h>

#define CPU_CLOCK_HZ (CONFIG_CPU_CLOCK_HZ)

void delay_us(const unsigned long us) {
    /* 指定的时钟周期数 */
    const unsigned long ticks = us * (CPU_CLOCK_HZ / 1000000);
    unsigned long count = 0;

    const uint32_t reload = SYSTICK_LOAD_REG;
    uint32_t prev_value = SYSTICK_VALUE_REG;

    /* 等待指定的时钟周期数 */
    for (;;) {
        const uint32_t curr_value = SYSTICK_VALUE_REG;

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
