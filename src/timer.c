// timer.c -- Initialises the PIT, and handles clock updates.
//            Written for JamesM's kernel development tutorials.

#include "timer.h"
#include "isr.h"
#include "monitor.h"

u32int tick = 0;
u32int quantum = 10;

static void timer_callback(registers_t regs)
{
    tick++;
    
    // monitor_write_hex(regs.ds);
    // monitor_write("\n");
    // monitor_write_hex(regs.edi);
    // monitor_write("\n");
    // monitor_write_hex(regs.esi);
    // monitor_write("\n");
    // monitor_write_hex(regs.ebp);
    // monitor_write("\n");
    // monitor_write_hex(regs.esp);
    // monitor_write("\n");
    // monitor_write_hex(regs.ebx);
    // monitor_write("\n");
    // monitor_write_hex(regs.edx);
    // monitor_write("\n");
    // monitor_write_hex(regs.ecx);
    // monitor_write("\n");
    // monitor_write_hex(regs.eax);
    // monitor_write("\n");
    // monitor_write_hex(regs.int_no);
    // monitor_write("\n");
    // monitor_write_hex(regs.err_code);
    // monitor_write("\n");
    // monitor_write_hex(regs.eip);
    // monitor_write("\n");
    // monitor_write_hex(regs.cs);
    // monitor_write("\n");
    // monitor_write_hex(regs.eflags);
    // monitor_write("\n");
    // monitor_write_hex(regs.useresp);
    // monitor_write("\n");
    // monitor_write_hex(regs.ss);
    // monitor_write("\n");

    if (tick >= quantum) {
        tick = 0;
        switch_task();
    }
}

void init_timer(u32int frequency)
{
    // Firstly, register our timer callback.
    register_interrupt_handler(IRQ0, &timer_callback);

    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency. Important to note is
    // that the divisor must be small enough to fit into 16-bits.
    u32int divisor = 1193180 / frequency;

    // Send the command byte.
    outb(0x43, 0x36);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    u8int l = (u8int)(divisor & 0xFF);
    u8int h = (u8int)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
    outb(0x40, l);
    outb(0x40, h);
}
