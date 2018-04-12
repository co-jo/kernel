#include "system.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"

#include "scrn.h"
#include "timer.h"
#include "paging.h"
#include "task.h"
#include "pipes.h"

#include "syscall.h"
#include "debug.h"

unsigned int initial_esp;

int kmain(struct multiboot *mboot_ptr, unsigned int initial_stack)
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
  window_install();
  /* Keyboard */
  keyboard_install();
  /* Support Tasking */
  timer_install();

  initialise_paging();
  initialise_tasking();

  set_window_title("Colin & Joshua's Kernel");

  /* Support RING3 */
  initialise_syscalls();

  /* Run Tests */
  test_kit();

  switch_to_user_mode();

  /* User Program */
  //main();
  
  /* Idle Task */
  idle();
  return 0;
}
