#ifndef DEBUG
#define DEBUG

#include "isr.h"

void debug_regs(regs_t *regs);
void dump_regs(regs_t *regs);

void print_user_flag();

#endif
