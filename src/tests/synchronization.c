#include "system.h"
#include "tests.h"
#include "task.h"
#include "semaphores.h"
#include "pipes.h"

void invalid()
{
  assert_eq(close_sem(1), 0, "Close Invalid Sem");
  assert_eq(signal(1), 0,  "Signal Invalid Sem");
  assert_eq(wait(1), 0, "Wait Invalid Sem");
}

void concurrent()
{
  int asem = open_sem(1);
  int bsem = open_sem(1);
  assert_neq(bsem, 0, "Open Binary Semaphore.");
  int child = kfork();
  if (child != 0) {
    assert_neq(wait(bsem), 0, "Parent acquiring Semaphore");
    setpriority(1,7);
    setpriority(2,1);
    printf("Yieling Parent [%x]\n", getpid());
    yield();
    assert_eq(signal(bsem), bsem, "Releasing Binary Semaphore");
  } else {
    puts("In Child..\n");
    assert_neq(wait(bsem), 0, "Child waiting on Semaphore");
    assert_eq(1, 1, "Child holds Binary Semaphore");
  }
}

void sync_tests()
{
  set_window_title("Running Synchronization Tests..");
  invalid();
  concurrent();
}

