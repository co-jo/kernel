#include "system.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"

#include "scrn.h"
#include "timer.h"
#include "paging.h"
#include "task.h"

#include "syscall.h"
#include "debug.h"
unsigned int initial_esp;

int main(struct multiboot *mboot_ptr, unsigned int initial_stack)
{
  initial_esp = initial_stack;

  /* Descriptor Tables */
  gdt_install();
  idt_install();
  /* Interrupt Handlers */
  isr_install();
  irq_install();
  /* Video Output */
  init_video();
  /* Support Tasking */
  timer_install(18);

  initialise_paging();
  initialise_tasking();

  /* Support RING3 */
  initialise_syscalls();
  switch_to_user_mode();

  int pid = ufork();
  syscall_printf("PID : [%x]\n", pid);
  if (pid > 0) {
    int i = 0;
    int sum = 0;
    for (int i = 0; i < 1000; i++)
     sum++;
    syscall_printf("Final SUM (Panret): [%x]\n", sum);
  }
  else {
    int i = 0;
    int sum = 0;
    for (int i = 0; i < 1000; i++)
     sum++;
    syscall_printf("Final SUM (Child) : [%x]\n", sum);
    cli(">...<");
  }

  return 0;
}
