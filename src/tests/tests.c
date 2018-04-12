#include "system.h"

void  test_kit() {
  memory_tests();
  // sync_tests();
  ipc_tests();
  if (getpid() != 1) exit();
}

int success(const char *msg)
{
    settextcolor(10, 0);
    puts("+ ");
    settextcolor(15, 0);
    puts(msg);
    putch('\n');
}

void failure(int value, int expected, const char *msg)
{
    settextcolor(12, 0);
    puts("! ");
    settextcolor(15, 0);
    puts(msg);
    putch('\n');
    puts("  ");
    putch(28);
    puts(" EXP: [");
    settextcolor(10, 0);
    printf("%d", expected);
    settextcolor(15, 0);
    puts("] ACT: [");
    settextcolor(12, 0);
    printf("%d", value);
    settextcolor(15, 0);
    puts("]\n");
}

void assert_eq(int value, int expected, const char *msg)
{
  if (value == expected) {
    success(msg);
  } else {
    failure(value, expected, msg);
  }
}

void assert_neq(int value, int expected, const char *msg)
{
  if (value != expected) {
    success(msg);
  } else {
    failure(value, expected, msg);
  }
}
