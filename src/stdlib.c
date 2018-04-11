#include "syscall.h"

/***************************** Output Functions *******************************/

//void print(const char *string, int args)
//{
//}
//
//void print_hex(unsigned int num)
//{
//}
//
//void print_dec(unsigned int num)
//{
//}

/***************************** Memory Managment *******************************/

void *alloc(unsigned int size, unsigned char page_align)
{
  syscall__alloc(size, page_align);
}

void free(void *ptr)
{
  syscall__free(ptr);
}

/***************************** Process Managment ******************************/

// void initialise_tasking()
// {
//   syscall_initialise_tasking();
// }

//int fork()
//{
//
//}

void exit()
{
  syscall__exit();
}

void yield()
{
  syscall__yield();
}

int sleep(unsigned int secs)
{
  return syscall__sleep(secs);
}

int getpid()
{
  return syscall__getpid();
}

int setpriority(int pid, int priority)
{
  return syscall__setpriority(pid, priority);
}

/******************************* Syncronization *******************************/

int open_sem(int num_semaphores)
{
  return syscall__open_sem(num_semaphores);
}

int wait(int semaphore_id)
{
  return syscall__wait(semaphore_id);
}

int signal(int semaphore_id)
{
  return syscall__signal(semaphore_id);
}

int close_sem(int semaphore_id)
{
  return syscall__close_sem(semaphore_id);
}

/************************************** IPC ***********************************/

int open_pipe()
{
  return syscall__open_pipe();
}

unsigned int write(int fildes, const void *buf, unsigned int num_bytes)
{
  return syscall__write(fildes, buf, num_bytes);
}

unsigned int read(int fildes, void *buf, unsigned int num_bytes)
{
  return syscall__read(fildes, buf, num_bytes);
}

int close__pipe(int fildes)
{
  return close__pipe(fildes);
}
