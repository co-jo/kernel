#if 0
/***************************** Output Functions *******************************/

void print(const char *string, int args)
{
}

void *alloc(unsigned int size, unsigned char page_align)
{
}

void free(void *ptr)
{
}

/***************************** Process Managment ******************************/

void initialise_tasking()
{
}

int fork()
{
  puts("Fork.,.\n");
}

int exit()
{
}

int yield()
{
}

int sleep()
{
}

int getpid()
{
}

int setpriority(int pid, int priority)
{
}

/******************************* Syncronization *******************************/

int open_sem(int num_semaphores)
{
}

int wait(int semaphore_id)
{
}

int close_sem(int semaphore_id)
{
}

/************************************** IPC ***********************************/

int open_pipe()
{
}

unsigned int write(int fildes, const void *buf, unsigned int num_bytes)
{
}

unsigned int read(int fildes, void *buf, unsigned int num_bytes)
{
}

int close_pipe(int fildes)
{

}
#endif
