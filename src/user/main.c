#include "stdlib.h"

/* User Space Region */
int main() {
  int pid = fork();
  if (pid) {
    puts("Parent - Sleep\n");
    sleep(10);
    puts("Parent - Awake");
  } else {
    puts("In Child\n");
  }
  return 0;
}
