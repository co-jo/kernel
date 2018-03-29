// task.h - Defines the structures and prototypes needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#ifndef TASK
#define TASK

#include "system.h"
#include "paging.h"

#define ZOMBIE -1
#define FORKED 0
#define READY 1
#define WAITING 2
#define RUNNING 3
#define SLEEPING 4
#define MASKABLE 5
#define NOT_MASKABLE 6

#define DEFAULT_PRIORITY 4

// This structure defines a 'task' - a process.
// Do not change order of ESP/EBP/EIP/EAX! - Refer to save_frame
typedef struct task
{
    unsigned int esp;                     // Base Pointer
    unsigned int ebp;                     // Stack Pointer
    unsigned int eip;                     // Instruction pointer
    unsigned int eax;                     // Possible return value
    unsigned int kernel_stack;            // SYSCall stack
    unsigned int stack;                   // Main Stack
    unsigned int state;                   // State the process can be in
    unsigned int user;                    // Is this a user or kernel task
    struct task *next;                    // The next task in a linked list
    struct task *prev;
    int id;                               // Process ID
    int priority;                         // Task priority
    unsigned int sleep_time;           // How much longer this task needs to sleep
    page_directory_t *page_directory;     // Page directory
} task_t;

// Initialises the tasking system.
void initialise_tasking();

// Called by the timer hook, this changes the running process.
void switch_task();

// Forks the current process, spawning a new one with a different
// memory space.
int pfork();

// int fork();

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

// insert a task into the ready queue
void enqueue_task(task_t *task);

// remove the first task
task_t *dequeue_task();

int sleep(unsigned int secs);

#endif
