#include <zhiyec/Tick.h>

volatile tick_t kernel_ticks = 0U;

volatile int interrupt_disabled_nesting = 0;
