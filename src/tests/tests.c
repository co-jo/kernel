#include "system.h"

void  test_kit() {
  memory_tests();
}

void assert_eq(int value, int expected, const char *msg)
{
  if (value == expected) {
    settextcolor(10, 0);
    puts("+ ");
    settextcolor(15, 0);
    puts(msg);
    putch('\n');
  }
}

void assert_neq(int value, int expected, const char *msg)
{
  if (value != expected) {
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
    puts("] OUT: [");
    settextcolor(12, 0);
    printf("%d", value);
    settextcolor(15, 0);
    puts("]\n");
  }
}
