#ifndef STDLIB
#define STDLIB
/***************************** Output Functions *******************************/

//void print(const char *string, int args);
//void print_hex(unsigned int num);
//void print_dec(unsigned int num);

/***************************** Memory Managment *******************************/

void *alloc(unsigned int size, unsigned char page_align);
void free(void *ptr);

/***************************** Process Managment ******************************/

void initialise_tasking();
int fork();
void exit();
void yield();
int sleep(unsigned int secs);
int getpid();
int setpriority(int pid, int priority);

/******************************* Syncronization *******************************/

int open_sem(int num_semaphores);
int wait(int semaphore_id);
int signal(int semaphore_id);
int close_sem(int semaphore_id);

/************************************** IPC ***********************************/

int open_pipe();
unsigned int write(int fildes, const void *buf, unsigned int num_bytes);
unsigned int read(int fildes, void *buf, unsigned int num_bytes);
int close_pipe(int fildes);

#endif
