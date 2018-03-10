// task.h - Defines the structures and prototypes needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#ifndef TASK_H
#define TASK_H

#include "system.h"
#include "paging.h"

// This structure defines a 'task' - a process.
typedef struct task
{
    int id;                // Process ID.
    unsigned int esp, ebp;       // Stack and base pointers.
    unsigned int eip;            // Instruction pointer.
    page_directory_t *page_directory; // Page directory.
    unsigned int kernel_stack;
    struct task *next;     // The next task in a linked list.
} task_t;

// Initialises the tasking system.
void initialise_tasking();

// Called by the timer hook, this changes the running process.
void switch_task();

// Forks the current process, spawning a new one with a different
// memory space.
int fork();

// Causes the current process' stack to be forcibly moved to a new location.
void move_stack(unsigned int base, unsigned int num_frames);

// Returns the pid of the current process.
int getpid();

// Switch to user mode
void switch_to_user_mode();

// Create Task
task_t *create_task();

// Initial Kernel Task
task_t *create_init_task();

#endif
