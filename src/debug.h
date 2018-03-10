#ifndef DEBUG
#define DEBUG

#include "isr.h"

void debug_regs(regs *regs);
void dump_regs(regs *regs);

void print_user_flag();

#endif
