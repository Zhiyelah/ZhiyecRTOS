#include "Tick.h"
#include "Config.h"
#include "Port.h"

#define SYSTICK_RATE_HZ (CONFIG_SYSTICK_RATE_HZ)

static volatile Tick_t tick_count = 0;

Tick_t Tick_currentTicks() {
    return tick_count;
}

Tick_t Tick_fromMsec(const unsigned int ms) {
    return (ms * SYSTICK_RATE_HZ + (1000 - 1)) / 1000;
}

unsigned int Tick_toMsec(const Tick_t ticks) {
    return (ticks * 1000UL + (SYSTICK_RATE_HZ - 1)) / SYSTICK_RATE_HZ;
}

bool Tick_after(const Tick_t current_ticks, const Tick_t target_ticks) {
    return (STick_t)(target_ticks - current_ticks) < 0;
}

void kernel_Tick_inc() {
    ++tick_count;
}
