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

void test_synch();
void assert_not_equal(unsigned int value, unsigned int expected, const char *msg);
void assert_equal(unsigned int value, unsigned int expected, const char *msg);

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

/*
  int sem = open_sem(1);
  int p = kfork();
  if (p) { // in parent
      wait(sem);
      setpriority(1, 7);
      yield();
      puts("In parent\n");
      signal(sem);
  } else {
      int p2 = kfork();
      if (p2) {
          wait(sem);
          puts("In child 1\n");
          signal(sem);
          exit();
      } else {
          wait(sem);
          puts("In child 2\n");
          signal(sem);
          exit();
      }
  }
  close_sem(sem);
  // exit();
  */
  test_synch();
  return 0;
}

void test_synch() 
{
    // assumes that no other forks have taken place earlier
    print("Testing synchronization...\n");

    // testing invalid semaphores
    assert_equal(close_sem(3), 0, "Closing invalid semaphore");
    assert_equal(signal(3), 0, "Signalling invalid semaphore");
    assert_equal(wait(3), 0, "Waiting on invalid semaphore");

    // testing non binary semaphore
    int sem = open_sem(1);
    int finish_sem = open_sem(1);
    assert_not_equal(sem, 0, "Opened new binary semaphore");
    int child = kfork();
    if (child != 0) {
        assert_not_equal(wait(sem), 0, "Aquiring semaphore in parent");
        setpriority(1,7);
        setpriority(2,1);
        yield();
        print("(Parent) This message should come first\n");
        assert_not_equal(signal(sem), 0, "Releasing semaphore in parent");
    } else {
        assert_not_equal(wait(finish_sem), 0, "Aquiring finish_sem in child");
        assert_not_equal(wait(sem), 0, "Aquiring semaphore in child");
        print("(Child) This message should come second\n");
        assert_not_equal(signal(sem), 0, "Releasing semaphore in child");
        assert_not_equal(signal(finish_sem), 0, "Releasing finish_sem in child");
        exit();
    }

    assert_not_equal(wait(finish_sem), 0, "Aquiring finish_sem in parent");
    assert_not_equal(signal(finish_sem), 0, "Releasing finish_sem in parent");

    assert_not_equal(close_sem(sem), 0, "Closing semaphore");

    // testing non-binary semaphore
    //sem = open_sem(3);
    //assert_not_equal(sem, 0, "Opened new non-binary semaphore");
    //int child1 = kfork();
    // int child2 = kfork();
    //int pid = getpid();
/*
    switch (pid) {
    case 1:
        wait(finish_sem);
        wait(sem);
        print("Entered critical region on process 1\n");
        setpriority(1, 9);
        setpriority(5, 8);
        yield();
        signal(sem);
        print("This should come second\n");
        signal(finish_sem);
        break;
    case 3:
        wait(sem);
        print("Entered critical region on process 2\n");
        break;
    case 4:
        wait(sem);
        print("Entered critical region on process 3\n");
        break;
    case 5:
        wait(sem);
        print("Entered critical region on process 4\n");
        print("This should come first\n");
        break;
    }
    if (pid != 1) {
        signal(sem);
        wait(finish_sem);
        signal(finish_sem);
        exit();
    }
    wait(finish_sem);
    signal(finish_sem);
    close_sem(finish_sem);
    assert_not_equal(close_sem(sem), 0, "Closing non-binary semaphore");

    print("Completed synchronization testing...\n\n");
*/
}


void assert_not_equal(unsigned int value, unsigned int expected, const char *msg)
{
    if (value != expected) {
        print("PASS: ");
        print(msg);
    } else {
        print("FAIL: ");
        print(msg);
        print("\nExpected: ");
        print_dec(expected);
        print("\tActual: ");
        print_dec(value);
    }
    print("\n");
}

void assert_equal(unsigned int value, unsigned int expected, const char *msg)
{
    if (value == expected) {
        print("PASS: ");
        print(msg);
    } else {
        print("FAIL: ");
        print(msg);
        print("\nExpected: ");
        print_dec(expected);
        print("\tActual: ");
        print_dec(value);
    }
    print("\n");
}

