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

// Wrappers for syscalls - not actual C stdlib
#include "stdlib.h"

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
  window_install();
  /* Keyboard */
  keyboard_install();
  /* Support Tasking */
  timer_install(18);

  initialise_paging();
  initialise_tasking();

  set_window_title("Colin & Joshua's Kernel");

  /* Support RING3 */
  initialise_syscalls();
  //switch_to_user_mode();

  int p = kfork();
  if (p) { // in parent
      print_ready_queue();
      setpriority(1, 7);
      print_ready_queue();
      yield();
      puts("In parent\n");
  } else {
      int p2 = kfork();
      if (p2) {
          print_ready_queue();
          puts("In child 1\n");
          exit();
      } else {
          puts("In child 2\n");
          print_ready_queue();
          exit();
      }
  }
  // exit();
  return 0;
}
