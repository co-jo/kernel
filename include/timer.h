#ifndef TIMER
#define TIMER

#include "system.h"

void timer_handler(regs_t *regs);
void timer_phase(int frequency);
void timer_install(int frequency);

#endif
