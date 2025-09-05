#include "Tick.h"

volatile Tick_t kernel_ticks = 0U;

volatile int interrupt_disabled_nesting = 0;
