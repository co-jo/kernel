//
// task.h - Defines the structures and prototypes needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "paging.h"

#define KERNEL_STACK_SIZE 2048

// wrapper around heap_t for tasks
typedef struct heap_u
{
  void *ptr;
  struct heap_u *next;
} heap_u;

// This structure defines a 'task' - a process.
typedef struct task
{
    int id;                // Process ID.
    u32int esp, ebp;       // Stack and base pointers.
    u32int eip;            // Instruction pointer.
    u32int priority;
    u32int time_spent;     // time quantums spent using the CPU
    heap_u *heap;
    page_directory_t *page_directory; // Page directory.
    u32int kernel_stack;
    struct task *parent;              // Who forked this task?
    struct task *next_sibling;        // The siblings of this task
    struct task *first_child;         // The first child
    struct task *next;     // The next task in the ready queue
    struct task *prev;     // The previous task in the ready queue
} task_t;

// Initialises the tasking system.
void initialise_tasking();

// Called by the timer hook, this changes the running process.
void switch_task();

// Forks the current process, spawning a new one with a different
// memory space.
int fork();

// Causes the current process' stack to be forcibly moved to a new location.
void move_stack(void *new_stack_start, u32int size);

// Returns the pid of the current process.
int getpid();

// Switch to user mode
void switch_to_user_mode();

// free memory allocated by this task
void free(void *p);

// allocate memory for use by this task
void *alloc(u32int size, u8int page_align);

// print current contents of the task heap
void print_user_heap();

#endif
