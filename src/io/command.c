#include "system.h"
#include "command.h"

#define NUM_CMDS 2

cmd_t cmds[NUM_CMDS] = {
  { "halt", "Halting...", &halt },
  { "shutdown", "Shutting Down...", &shutdown }
};

void cmd_err(char *key)
{
  int i;
  char msg[32] = "Invalid Command : ";
  for (i = 0; i < strlen(key); i++)
    msg[18 + i] = key[i];

  set_window_title(msg);
}

int is_cmd(char *key)
{
  int i;
  for (i = 0; i < NUM_CMDS; i++) {
    cmd_t cmd = cmds[i];
    if (strcmp(cmd.key, key))
        return i;
  }
  return -1;
}

void exec_cmd(char *key)
{
  int id = is_cmd(key);
  if (id != -1) {
    set_window_title(cmds[id].msg);
    cmds[id].func();
  } else {
    cmd_err(key);
  }
}
