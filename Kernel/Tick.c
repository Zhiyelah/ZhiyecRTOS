#include "Tick.h"
#include "Config.h"
#include "Port.h"

#define INTERRUPT_HZ (CONFIG_INTERRUPT_HZ)

static volatile Tick_t tick_count = 0;

Tick_t Tick_getCurrentTicks() {
    return tick_count;
}

Tick_t Tick_fromMsec(const unsigned int ms) {
    return (ms * INTERRUPT_HZ + (1000 - 1)) / 1000;
}

unsigned int Tick_toMsec(const Tick_t ticks) {
    return (ticks * 1000UL + (INTERRUPT_HZ - 1)) / INTERRUPT_HZ;
}

bool Tick_after(const Tick_t current_ticks, const Tick_t target_ticks) {
    return (STick_t)(target_ticks - current_ticks) < 0;
}

void kernel_Tick_inc() {
    ++tick_count;
}
