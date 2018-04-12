#include "stdlib.h"

/* User Space Region */
int main() {
  int pipefd = open_pipe();
  int pid = fork();
  if (pid) {
    puts("Parent - Sleep\n");
    sleep(4);
    puts("Parent - Awake\n");
    char text[20] = "Hello world!\n\0";
    write(pipefd, text, 14);
    print("Wrote ");
    print(text);
    print("to pipe\n");
  } else {
    puts("In Child\n");
    char buffer[20];
    read(pipefd, buffer, 14);
    buffer[20] = '\0';
    print(buffer);
    exit();
  }
  return 0;
}
