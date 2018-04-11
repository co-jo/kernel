#include "syscall.h"
#include "isr.h"
#include "debug.h"
#include "scrn.h"
#include "system.h"

/* Syscall Includes */
#include "task.h"
#include "semaphores.h"
#include "pipes.h"
#include "kheap.h"
#include "scrn.h"

DEFN_SYSCALL0(pfork, 0);

/* Memory */
DEFN_SYSCALL2(_alloc, 1, unsigned int, unsigned char);
DEFN_SYSCALL1(_free, 2, void*);

/* Tasking */
DEFN_SYSCALL0(initialise_tasking, 3);
DEFN_SYSCALL0(_exit, 4);
DEFN_SYSCALL0(_yield, 5);
DEFN_SYSCALL1(_sleep, 6, unsigned int);
DEFN_SYSCALL0(_getpid, 7);
DEFN_SYSCALL2(_setpriority, 8, int, int);

/* Syncronization */
DEFN_SYSCALL1(_open_sem, 9, int);
DEFN_SYSCALL1(_wait, 10, int);
DEFN_SYSCALL1(_signal, 11, int);
DEFN_SYSCALL1(_close_sem, 12, int);

/* IPC */
DEFN_SYSCALL0(_open_pipe, 13);
DEFN_SYSCALL3(_write, 14, int, const void*, unsigned int);
DEFN_SYSCALL3(_read, 15, int, void*, unsigned int);
DEFN_SYSCALL1(_close_pipe, 16, int);

/* Output Functions */
DEFN_SYSCALL2(printf, 17, const char*, int);


static void *syscalls[18] =
{
    &pfork,
    &_alloc,
    &_free,
    &initialise_tasking,
    &_exit,
    &_yield,
    &_sleep,
    &_getpid,
    &_setpriority,
    &_open_sem,
    &_wait,
    &_signal,
    &_close_sem,
    &_open_pipe,
    &_write,
    &_read,
    &_close_pipe,
    &printf
};
unsigned int num_syscalls = 18;

void initialise_syscalls()
{
    // Register our syscall handler.
    register_interrupt_handler (0x80, &syscall_handler);
}

void syscall_handler(regs_t *regs)
{
    // Firstly, check if the requested syscall number is valid.
    // The syscall number is found in EAX.
    if (regs->eax >= num_syscalls)
        return;
    // Get the required syscall location.
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
