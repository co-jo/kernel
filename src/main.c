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
  // int pid = fork();
  // if (pid > 0) {
  //   int i;
  //   int sum = 0;
  //   for (i = 0; i < 100000001; i++) {
  //     sum++;
  //   }
  //   printf("PSUM: [%d]\n", sum);
  // } else {
  //   int i;
  //   int sum = 0;
  //   for (i = 0; i < 100000000; i++) {
  //     sum++;
  //   }
  //   printf("CSUM: [%d]\n", sum);
  // }

  assert_eq(0, 0, "EQ");
  assert_eq(0, 0, "EQ");
  assert_neq(1, 0, "NEQ");
  return 0;
}
