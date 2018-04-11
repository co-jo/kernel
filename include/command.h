typedef struct cmd {
  char *key;
  char *msg;
  void (*func)();
} cmd_t;

int is_cmd(char *key);
void exec_cmd(char *key);
void cmd_err(char *key);

