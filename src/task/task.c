//
// task.c - Implements the functionality needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#include "task.h"
#include "paging.h"
#include "kheap.h"
#include "system.h"

#define SLEEP_MAGIC 85

// The currently running task.
volatile task_t *current_task;

// The start of the task linked list.
volatile task_t *ready_queue;

// list of sleeping tasks
volatile task_t *sleep_list;
// Number of active threads
volatile int nt = 0;

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern unsigned int initial_esp;

extern heap_t *kheap;

extern void perform_task_switch(unsigned int eip, unsigned int, unsigned int, unsigned int);

// The next available process ID.
int pid = 1;

void initialise_tasking()
{
  // Rather important stuff happening, no interrupts please!
  asm volatile("cli");
  // Relocate the stack so we know where it is.
  // Initialise the first task (kernel task)
  move_stack(0xE0000000, 0x2);
  current_task = create_init_task();

  ready_queue = current_task;

  // Reenable interrupts.
  asm volatile("sti");
}

void flush_tlb()
{
  unsigned int pd;
  asm volatile("mov %%cr3, %0" : "=r" (pd));
  asm volatile("mov %0, %%cr3" : : "r" (pd));
}

void move_stack(unsigned int base, unsigned int num_frames)
{
  unsigned int i;
  unsigned int size = num_frames * FRAME_SIZE;
  // Allocate some space for the new stack.
  int sflags = flags(1, 1, 1);
  for(i = base - size; i <= base; i += FRAME_SIZE) {
    get_page(i, current_directory, sflags);
  }

  // Flush the TLB by reading and writing the page directory address again.
  flush_tlb();

  // Old ESP and EBP, read from registers.
  unsigned int old_esp;
  unsigned int old_ebp;

  RD_EBP(old_ebp);
  RD_ESP(old_esp);

  // Offset to add to old stack addresses to get a new stack address.
  unsigned int offset = base - initial_esp;
  // New ESP and EBP.
  unsigned int new_esp = old_esp + offset;
  unsigned int new_ebp = old_ebp + offset;

  // Copy the stack.
  memcpy(new_esp, old_esp, initial_esp - old_esp);

  // Backtrace through the original stack, copying new values into the new stack.
  int step = sizeof(int);
  for(i = base; i > base - size; i -= step) {
    unsigned int tmp = * (unsigned int*)i;
    // If the value of tmp is inside the range of the old stack, assume it is a base pointer
    // and remap it. This will unfortunately remap ANY value in this range, whether they are
    // base pointers or not.
    if (( old_esp < tmp) && (tmp < initial_esp))
    {
      tmp = tmp + offset;
      unsigned int *tmp2 = (unsigned int*)i;
      *tmp2 = tmp;
    }
  }

  // Change stacks.
  WT_EBP(new_ebp);
  WT_ESP(new_esp);
}

// We enter this func as a thread, therefore we want to reclaim the stack and leave
// as if it was never switched. The eip is implict in the return;
void switch_task()
{
  // If we haven't initialised tasking yet, just return.
  if (!current_task)
    return;

  // EIP should be implicilty saved on the stack before entering this function
  RD_EBP(current_task->ebp);
  RD_ESP(current_task->esp);

  if (current_task->state == ZOMBIE) {
      cleanup_task(current_task);
  }

  // Getinitialise the next task to run.
  current_task = ready_queue;

  // No tasks left
  if (!nt) {
    set_window_title("No Tasks Left... Halting");
    while(1);
  }

  current_directory = current_task->page_directory;
  set_kernel_stack(current_task->kernel_stack);

  // Loads Page and looks at associated FRAME
  unsigned int physical = get_physical(current_directory->phys_tables);
  // If we just forked we want to JMP to new entry; Else switch by RET
  unsigned int state = current_task->state;

  if(current_task->state == FORKED) {
    current_task->state = READY;
  }

  perform_task_switch(state, physical, current_task->ebp, current_task->esp);

  // Hopefully by instead of JMPing on each switch (only during fork)
  // We enter switch save T1's execution and eventually pick back up
  // where we left off and RET - maintaining a clearer flow of execution
  // and keeping the stack 'clean'

  return 0;
}

task_t *create_init_task()
{
  ++nt;
  task_t *task = (task_t*)kmalloc(sizeof(task_t), 0, 0);
  task->page_directory = current_directory;
  task->id = pid;
  task->stack = STACK_START;
  task->kernel_stack = kmalloc(KERNEL_STACK_SIZE, 1, 0) + KERNEL_STACK_SIZE;
  task->state = READY;
  task->priority = DEFAULT_PRIORITY;
  task->sleep_time = 0;
  task->esp = task->ebp = task->eip = task->next = task->user = 0;
  return task;
}

task_t *create_task()
{
  ++nt;
  task_t *task = (task_t*)kmalloc(sizeof(task_t), 0, 0);
  task->page_directory = clone_directory(current_directory);
  task->stack = kmalloc(2 * FRAME_SIZE, 1, 0) + 2 * FRAME_SIZE;
  task->kernel_stack = kmalloc(KERNEL_STACK_SIZE, 1, 0) + KERNEL_STACK_SIZE;
  task->id = ++pid;
  task->esp = task->ebp = task->eip = task->next = task->user = 0;
  task->state = FORKED;
  task->priority = DEFAULT_PRIORITY;
  task->sleep_time = 0;
  task->esp = task->ebp = task->eip = task->next = task->user = 0;
  return task;
}

int pfork()
{
  // Clone the address space.
  asm volatile("cli");
  task_t *parent = (task_t*)current_task;
  task_t *child = create_task();

  enqueue_task(child);

  child->ebp = parent->ebp;
  child->esp = parent->esp;
  child->eip = parent->eip;
  child->eax = 0x0;

  asm volatile("sti");
  return child->id;
}

void exit() {
    task_t *dying = dequeue_task();
    if (dying) {
      dying->state = ZOMBIE;
    }
    yield();
}

void yield() {
    switch_task();
}

void reprioritize()
{
    // 1 is the 'highest' priority, 10 is the 'lowest'
    // meaning a greater priority value is actually lower priority :)
    // return immediately if the ready queue only has one task
    if (!ready_queue) return;

    if (!ready_queue->next) {
        return;
    }

    task_t *first_task = (task_t*)ready_queue;
    task_t *tmp_task = (task_t*)ready_queue;

    // first, check if the first task is priority 10
    // in this case, all tasks have decayed to the lowest priority, so we
    // switch to a round-robin scheduling
    if (first_task->priority == 10) {
        ready_queue = ready_queue->next;
        ready_queue->prev = 0;
        while (tmp_task->next)
            tmp_task = tmp_task->next;
        tmp_task->next = first_task;
        first_task->prev = tmp_task;
        first_task->next = 0;
    }
    else {
        first_task->priority++;
        if (first_task->priority > first_task->next->priority) {
            ready_queue = ready_queue->next;
            ready_queue->prev = 0;
            tmp_task = tmp_task->next;
            // while tmp_task still has a higher priority than first_task
            while (first_task->priority > tmp_task->priority) {
                if (tmp_task->next)
                    tmp_task = tmp_task->next;
                else
                    break;
            }
            if (tmp_task->next) {
                // tmp_task now has a lesser or equal priority to first_task
                tmp_task->prev->next = first_task;
                first_task->prev = tmp_task->prev;
                first_task->next = tmp_task;
                tmp_task->prev = first_task;
            } else {
                tmp_task->next = first_task;
                first_task->prev = tmp_task;
                first_task->next = 0;
            }
        }
    }
    return;
}

int setpriority(int pid, int new_priority)
{
    task_t *iter = (task_t*)ready_queue;
    // edge case: first task matches pid
    if (iter->id == pid) {
        ready_queue = ready_queue->next;
        ready_queue->prev = 0;
        iter->next = 0;
        iter->priority = new_priority;
        enqueue_task(iter);
    } else {
        while(iter && (pid != iter->id))
            iter = iter->next;
        // if we found a task with a matching pid
        if (iter) {
            // set the new priority, and remove the task from the ready queue
            iter->priority = new_priority;
            iter->prev->next = iter->next;
            iter->next->prev = iter->prev;
            iter->next = 0;
            iter->prev = 0;
            // then add the task back into the queue
            enqueue_task(iter);
        }
    }
    return iter ? new_priority : 0;
}

/**
 * inserts a task into the ready queue
 * ordered by task priority
 */
void enqueue_task(task_t *task)
{
    if (!task) return;
    task_t *iterator = (task_t*)ready_queue;

    if (!ready_queue) {
        ready_queue = task;
        return;
    }

    if (!iterator->next) {
      // Place in front
      if (iterator->priority > task->priority) {
        task->next = ready_queue;
        ready_queue->prev = task;
        ready_queue = task;

      } else {
        // Append
        ready_queue->next = task;
        task->prev = ready_queue;
        task->next = 0;
      }
      return;
    }

    while (iterator->next) {
        // if task is still a lower priority than the iterator
        if (task->priority > iterator->priority) {
            iterator = iterator->next;
        } else {
            break;
        }
    }

    // if we haven't reached the end of the queue
    if (iterator->next) {
        task->next = iterator->next;
        iterator->next = task;
        task->prev = iterator;
    } else {
        iterator->next = task;
        task->prev = iterator;
        task->next = 0;
    }
    // the task is now in the correct place in the queue
}

task_t *dequeue_task()
{
    // error check: current_task not set
    if (!current_task) return 0;

    // Not from a sleep call
    if (nt == 1) {
      ready_queue = 0;
      return current_task;
    }

    task_t  *task = current_task;
    if (task == ready_queue) {
      ready_queue = ready_queue->next;
    }

    if (task) {
      // Bridge
      if (task->next && task->prev) {
        task->next->prev = task->prev;
        task->prev->next = task->next;
      } else if (task->next) {
        task->next->prev = 0;
      } else if (task->prev) {
        task->prev->next = 0;
      }
      // Remove Link
      task->next = task->prev = 0;
    }

    return current_task;
}

void cleanup_task(task_t *task) {
  //printf("Cleaning Up Task [%x]\n", task->id);
  task->next = 0;
  task->prev = 0;
  --nt;
  kfree(task);
}

int sleep(unsigned int secs)
{
    task_t *sleeping = dequeue_task();
    sleeping->sleep_time = secs;
    sleeping->state = SLEEPING;

    sleeping->next = sleep_list;
    sleep_list->prev = sleeping;
    sleep_list = sleeping;

    yield();

    return sleeping->sleep_time;
}

void update_sleeping_tasks()
{
    task_t *temp = (task_t*)sleep_list;
    task_t *waking_task;

    while (temp) {
        --(temp->sleep_time);
        if (temp->sleep_time <= 0) {         // if we're done sleeping, remove from the list
            waking_task = temp;
            if (!waking_task->prev) {               // we're at the start of the list in this case
                sleep_list = waking_task->next;
                sleep_list->prev = 0;
            } else {                         // else we're in the middle of the list
                waking_task->prev->next = waking_task->next;
                waking_task->next->prev = waking_task->prev;
            }
            waking_task->prev = 0;
            waking_task->next = 0;
            waking_task->sleep_time = 0;
            enqueue_task(waking_task);
        }
        temp = temp->next;
    }
}

task_t *remove_task_from_list(int pid, task_t *list)
{
    task_t *temp;
    for (temp = list; temp && temp->id != pid; temp = temp->next);
    if (temp) {
        temp->prev->next = temp->next;
        temp->next->prev = temp->prev;
        temp->next = 0;
        temp->prev = 0;
    }
    return temp; // returns null if pid not found
}

int contains_task(int pid, task_t *list)
{
    task_t *temp;
    for (temp = list; temp && temp->id != pid; temp = temp->next);
    return temp ? 1 : 0;
}

int getpid()
{
  return current_task->id;
}

void print_ready_queue()
{
    task_t *temp = (task_t*)ready_queue;
    char *buf;

    if (!temp)
      puts("Ready Queue Empty!");

    while(temp) {
        printf("Task [%d]: ", temp->id);
        printf("Priority: [%d]\n", temp->priority);
        temp = temp->next;
    }
}

void switch_to_user_mode()
{
  // Set up our kernel stack
  set_kernel_stack(current_task->kernel_stack);

  // Set up a stack structure for switching to user mode.
  asm volatile("  \
      cli; \
      mov $0x23, %ax; \
      mov %ax, %ds; \
      mov %ax, %es; \
      mov %ax, %fs; \
      mov %ax, %gs; \
      \
      mov %esp, %eax; \
      pushl $0x23; \
      pushl %eax; \
      pushf; \
      \
      pop %eax ; \
      or $0x200, %eax; \
      push %eax ;  \
      \
      pushl $0x1B; \
      push $1f; \
      iret; \
      1: \
  ");
}
