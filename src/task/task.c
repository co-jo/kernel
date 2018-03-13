//
// task.c - Implements the functionality needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#include "task.h"
#include "paging.h"
#include "kheap.h"
#include "system.h"

// The currently running task.
volatile task_t *current_task;

// The start of the task linked list.
volatile task_t *ready_queue;

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern unsigned int initial_esp;
extern unsigned int read_eip();
extern heap_t *kheap;

extern void perform_task_switch(unsigned int, unsigned int, unsigned int, unsigned int);

// The next available process ID.
int process_count = 1;

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
  for(i = base - size; i <= base; i += FRAME_SIZE) {
    get_page(i, current_directory, 1, 0);
  }

  // Flush the TLB by reading and writing the page directory address again.
  flush_tlb();

  // Old ESP and EBP, read from registers.
  unsigned int old_esp;
  unsigned int old_ebp;
  asm volatile("mov %%ebp, %0" : "=r" (old_ebp));
  asm volatile("mov %%esp, %0" : "=r" (old_esp));

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
  asm volatile("mov %0, %%esp" : : "r" (new_esp));
  asm volatile("mov %0, %%ebp" : : "r" (new_ebp));
}

void switch_task()
{
  // If we haven't initialised tasking yet, just return.
  if (!current_task)
    return;

  // Read esp, ebp now for saving later on.
  unsigned int esp, ebp, eip;

  asm volatile("mov %%esp, %0" : "=r"(esp));
  asm volatile("mov %%ebp, %0" : "=r"(ebp));

  eip = read_eip();
  //Have we just switched tasks?
  if (eip == 0x12345) {
    return;
  }
  // No, we didn't switch tasks. Let's save some register values and switch.
  current_task->eip = eip;
  current_task->esp = esp;
  current_task->ebp = ebp;

  // Getinitialise the next task to run.
  current_task = current_task->next;

  // If we fell off the end of the linked list start again at the beginning.
  if (!current_task) current_task = ready_queue;

  eip = current_task->eip;
  esp = current_task->esp;
  ebp = current_task->ebp;
  // Make sure the memory manager knows we've changed page directory.
  current_directory = current_task->page_directory;

  // Change our kernel stack over.
  set_kernel_stack(current_task->kernel_stack);

  unsigned int physical = get_physical((unsigned int *)current_directory->phys_tables); 
  perform_task_switch(eip, physical, ebp, esp);
}

task_t *create_init_task()
{
  task_t *task = (task_t*)kmalloc(sizeof(task_t));
  task->page_directory = clone_directory(current_directory);
  task->id = process_count++;
  task->esp = task->ebp = task->eip = task->next = 0;
  task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
  return task;
}

task_t *create_task()
{
  task_t *task = (task_t*)kmalloc(sizeof(task_t));
  task->page_directory = clone_directory(current_directory);
  task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
  task->id = process_count++;
  task->esp = task->ebp = task->eip = task->next = 0;
  return task;
}

int fork()
{
  // We are modifying kernel structures, and so cannot
  asm volatile("cli");
  // Take a pointer to this process' task struct for later reference.
  task_t *parent_task = (task_t*)current_task;

  // Clone the address space.
  task_t *child = create_task();

  // Add it to the end of the ready queue.
  task_t *tmp_task = (task_t*)ready_queue;
  while (tmp_task->next) {
    tmp_task = tmp_task->next;
  }
  tmp_task->next = child;
  // printf("TMP TSAK? %x\n", tmp_task);
  // This will be the entry point for the new process.
  unsigned int eip = read_eip();

  // We could be the parent or the child here - check.
  if (current_task == parent_task)
  {
    // We are the parent, so set up the esp/ebp/eip for our child.
    unsigned int esp; asm volatile("mov %%esp, %0" : "=r"(esp));
    unsigned int ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
    child->esp = esp;
    child->ebp = ebp;
    child->eip = eip;

    asm volatile("sti");
    return child->id;
  }
  else
  {
    return 0;
  }
}

int getpid()
{
  return current_task->id;
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
