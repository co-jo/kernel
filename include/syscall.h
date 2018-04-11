
#ifndef SYSCALL
#define SYSCALL

#include "system.h"
void initialise_syscalls();
void syscall_handler(regs_t *regs);

#define DECL_SYSCALL0(fn) int syscall_##fn();
#define DECL_SYSCALL1(fn,p1) int syscall_##fn(p1);
#define DECL_SYSCALL2(fn,p1,p2) int syscall_##fn(p1,p2);
#define DECL_SYSCALL3(fn,p1,p2,p3) int syscall_##fn(p1,p2,p3);
#define DECL_SYSCALL4(fn,p1,p2,p3,p4) int syscall_##fn(p1,p2,p3,p4);
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) int syscall_##fn(p1,p2,p3,p4,p5);

#define DEFN_SYSCALL0(fn, num) \
int syscall_##fn() \
{ \
  int a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
  return a; \
}

#define DEFN_SYSCALL1(fn, num, P1) \
int syscall_##fn(P1 p1) \
{ \
  int a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1)); \
  return a; \
}

#define DEFN_SYSCALL2(fn, num, P1, P2) \
int syscall_##fn(P1 p1, P2 p2) \
{ \
  int a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
  return a; \
}

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
int syscall_##fn(P1 p1, P2 p2, P3 p3) \
{ \
  int a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3)); \
  return a; \
}

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) \
{ \
  int a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4)); \
  return a; \
}

#define DEFN_SYSCALL5(fn, num) \
int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
{ \
  int a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4), "D" ((int)p5)); \
  return a; \
}

/* Random */
DECL_SYSCALL0(halt);

/* Output Functions */
DECL_SYSCALL2(printf, const char *, int arg);

/* Memory */
DECL_SYSCALL2(_alloc, unsigned int size, unsigned char page_align);
DECL_SYSCALL1(_free, void *ptr);

/* Tasking */
DECL_SYSCALL0(initialise_tasking);
DECL_SYSCALL0(_pfork);
DECL_SYSCALL0(_exit);
DECL_SYSCALL0(_yield);
DECL_SYSCALL1(_sleep, unsigned int seconds);
DECL_SYSCALL0(_getpid);
DECL_SYSCALL2(_setpriority, int pid, int priority);

/* Syncronization */
DECL_SYSCALL1(_open_sem, int num_semaphores);
DECL_SYSCALL1(_wait, int semaphore_id);
DECL_SYSCALL1(_signal, int semaphore_id);
DECL_SYSCALL1(_close_sem, int semaphore_id);

/* IPC */
DECL_SYSCALL0(_open_pipe);
DECL_SYSCALL3(_write, int fildes, const void *buf, unsigned int num_bytes);
DECL_SYSCALL3(_read, int fildes, void *buf, unsigned int num_bytes);
DECL_SYSCALL1(_close_pipe, int fildes);

#endif
