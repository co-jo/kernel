// syscall.c -- Defines the implementation of a system call system.
//              Written for JamesM's kernel development tutorials.

#include "syscall.h"
#include "isr.h"
#include "monitor.h"
#include "task.h"
#include "descriptor_tables.h"

static void syscall_handler(registers_t *regs);

// Monitor
DEFN_SYSCALL1(monitor_write, 0, const char*);
DEFN_SYSCALL1(monitor_write_hex, 1, const char*);
DEFN_SYSCALL1(monitor_write_dec, 2, const char*);
// Memory
DEFN_SYSCALL2(alloc, 3, const u32int, const u8int);
DEFN_SYSCALL1(free, 4, const void*);
// Process
DEFN_SYSCALL0(fork, 5);
DEFN_SYSCALL0(isr0, 6);
DEFN_SYSCALL0(irq0, 7);

static void *syscalls[8] =
{
    &monitor_write,
    &monitor_write_hex,
    &monitor_write_dec,
    &alloc,
    &free,
    &fork,
    &isr0,
    &irq0
};
u32int num_syscalls = 8;

void initialise_syscalls()
{
    // Register our syscall handler.
    register_interrupt_handler (0x80, &syscall_handler);
}

void syscall_handler(registers_t *regs)
{
    // Firstly, check if the requested syscall number is valid.
    // The syscall number is found in EAX.
    if (regs->eax >= num_syscalls)
        return;

    // Get the required syscall locatioallocn.
    void *location = syscalls[regs->eax];

    // We don't know how many parameters the function wants, so we just
    // push them all onto the stack in the correct order. The function will
    // use all the parameters it wants, and we can pop them all back off afterwards.
    int ret;
    asm volatile (" \
      push %1; \
      push %2; \
      push %3; \
      push %4; \
      push %5; \
      call *%6; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
    " : "=a" (ret) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (location));
    regs->eax = ret;
}
