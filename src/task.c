//
// task.c - Implements the functionality needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#include "task.h"
#include "paging.h"
#include "kheap.h"

// The currently running task.
volatile task_t *current_task;

// The start of the task linked list.
volatile task_t *ready_queue;

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern void alloc_frame(page_t*,int,int);
extern u32int initial_esp;
extern u32int read_eip();
extern heap_t *gheap;

extern void perform_task_switch(u32int, u32int, u32int, u32int);

// The next available process ID.
u32int next_pid = 1;

void initialise_tasking()
{
    // Rather important stuff happening, no interrupts please!
    asm volatile("cli");

    // Relocate the stack so we know where it is.
    move_stack((void*)0xE0000000, 0x2000);

    // Initialise the first task (kernel task)
    current_task = ready_queue = (task_t*)kmalloc(sizeof(task_t));
    current_task->heap = 0;
    current_task->id = next_pid++;
    current_task->esp = current_task->ebp = 0;
    current_task->eip = 0;
    current_task->time_spent = 0;
    current_task->priority = 4;
    current_task->death_flag = 0;

    current_task->page_directory = current_directory;
    current_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    /* monitor_write("first_kstack: \n");
       monitor_write_hex(current_task->kernel_stack);*/
    current_task->parent = 0;
    current_task->next_sibling = 0;
    current_task->first_child = 0;
    current_task->next = 0;
    current_task->prev = 0;

    // Reenable interrupts.
    asm volatile("sti");
}

void move_stack(void *new_stack_start, u32int size)
{
  u32int i;
  // Allocate some space for the new stack.
  for( i = (u32int)new_stack_start;
       i >= ((u32int)new_stack_start-size);
       i -= 0x1000)
  {
    // General-purpose stack is in user-mode.
    alloc_frame( get_page(i, 1, current_directory), 0 /* User mode */, 1 /* Is writable */ );
  }

  // Flush the TLB by reading and writing the page directory address again.
  u32int pd_addr;
  asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
  asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

  // Old ESP and EBP, read from registers.
  u32int old_stack_pointer; asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  u32int old_base_pointer;  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

  // Offset to add to old stack addresses to get a new stack address.
  u32int offset            = (u32int)new_stack_start - initial_esp;
  // monitor_write("\n");
  // least significant byte = 54;
  // monitor_write_hex(offset + old_stack_pointer);

  // New ESP and EBP.
  u32int new_stack_pointer = old_stack_pointer + offset;
  u32int new_base_pointer  = old_base_pointer  + offset;

  // Copy the stack.
  memcpy((void*)new_stack_pointer, (void*)old_stack_pointer, initial_esp-old_stack_pointer);

  // Backtrace through the original stack, copying new values into
  // the new stack.
  for(i = (u32int)new_stack_start; i > (u32int)new_stack_start-size; i -= 4)
  {
    u32int tmp = * (u32int*)i;
    // If the value of tmp is inside the range of the old stack, assume it is a base pointer
    // and remap it. This will unfortunately remap ANY value in this range, whether they are
    // base pointers or not.
    if (( old_stack_pointer < tmp) && (tmp < initial_esp))
    {
      tmp = tmp + offset;
      u32int *tmp2 = (u32int*)i;
      *tmp2 = tmp;
    }
  }

  // Change stacks.
  asm volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
  asm volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}

// grab the next task in the priority queue and change priorities
void schedule()
{
    // monitor_write("Reprioritizing...\n");
    // print_ready_queue();

    // 1 is the 'highest' priority, 10 is the 'lowest'
    // meaning a greater priority value is actually lower priority :)
    // return immediately if the ready queue only has one task
    if (!ready_queue->next) {
        return;
    }
    current_task = ready_queue;

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
    } else {
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

// creates a new heap, copying over all of the data in the old one
heap_u *clone_heap(heap_u *old_heap)
{   
    if (!old_heap) return 0;
    heap_u *new_heap = (heap_u*)kmalloc(sizeof(heap_u));
    heap_u *tmp = new_heap;
    while (old_heap) {
        if (!tmp) 
            tmp = (heap_u*)kmalloc(sizeof(heap_u));
        tmp->ptr = alloc(sizeof(old_heap->size), old_heap->align);
        memset(tmp->ptr, old_heap->ptr, old_heap->size);
        tmp->size = old_heap->size;
        tmp->align = old_heap->align;
        tmp = tmp->next;
        old_heap = old_heap->next;
    }
    return new_heap;
}

void switch_task()
{    
    // monitor_write("\n====== enter_switch ======\n");
    // If we haven't initialised tasking yet, just return.
    if (!current_task)
        return;

    // Read esp, ebp now for saving later on.
    u32int esp, ebp, eip;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%ebp, %0" : "=r"(ebp));

    // Read the instruction pointer. We do some cunning logic here:
    // One of two things could have happened when this function exits -
    //   (a) We called the function and it returned the EIP as requested.
    //   (b) We have just switched tasks, and because the saved EIP is essentially
    //       the instruction after read_eip(), it will seem as if read_eip has just
    //       returned.
    // In the second case we need to return immediately. To detect it we put a dummy
    // value in EAX further down at the end of this function. As C returns values in EAX,
    // it will look like the return value is this dummy value! (0x12345).
    
    // monitor_write("pre_eip: ");
    // monitor_write_hex(current_task->eip);
    // monitor_write("\n");
    eip = read_eip();
    // monitor_write("\n eip: "); monitor_write_hex(eip);
    
    // monitor_write("post_eip: ");
    // monitor_write_hex(eip);
    // monitor_write("\n");
    // Have we just switched tasks?
    if (eip == 0x12345)
        return;

    // monitor_write("\n current_pid: ");
    // monitor_write_hex(current_task->id);
    // No, we didn't switch tasks. Let's save some register values and switch.
    current_task->eip = eip;
    current_task->esp = esp;
    current_task->ebp = ebp;

    // Get the next task to run.
    schedule();

    // If we fell off the end of the linked list start again at the beginning.
    // !!!! won't happen with current implementation
    // if (!current_task) current_task = ready_queue;

    eip = current_task->eip;
    esp = current_task->esp;
    ebp = current_task->ebp;
    
    // Make sure the memory manager knows we've changed page directory.
    current_directory = current_task->page_directory;

    // Change our kernel stack over.


    /*  monitor_write("\n==== current_kstack: ===== \n");
    monitor_write("pid: ");
    monitor_write_hex(current_task->id);
    monitor_write("\n");

    monitor_write_hex(current_task->kernel_stack);*/
    set_kernel_stack(current_task->kernel_stack+KERNEL_STACK_SIZE);

    // Here we:
    // * Stop interrupts so we don't get interrupted.
    // * Temporarily puts the new EIP location in ECX.
    // * Loads the stack and base pointers from the new task struct.
    // * Changes page directory to the physical address (physicalAddr) of the new directory.
    // * Puts a dummy value (0x12345) in EAX so that above we can recognise that we've just
    //   switched task.
    // * Restarts interrupts. The STI instruction has a delay - it doesn't take effect until after
    //   the next instruction.
    // * Jumps to the location in ECX (remember we put the new EIP in there).
    // monitor_write("task_switch_eip - "); monitor_write_hex(eip);
    // monitor_write("\n");
    
    // monitor_write("\neip - :");
    // monitor_write_hex(eip);

    perform_task_switch(eip, current_directory->physicalAddr, ebp, esp);

   // ANYTHING BELOW NEVER GETS CALLED
}

int fork()
{
    // We are modifying kernel structures, and so cannot
    asm volatile("cli");

    // Take a pointer to this process' task struct for later reference.
    task_t *parent_task = (task_t*)current_task;

    // Clone the address space.
    page_directory_t *directory = clone_directory(current_directory);
    // Clone the user heap
    heap_u *heap = clone_heap(parent_task->heap);

    // Create a new process.
    task_t *new_task = (task_t*)kmalloc(sizeof(task_t));

    new_task->id = next_pid++;
    new_task->esp = new_task->ebp = 0;
    new_task->eip = 0;
    new_task->priority = 4;
    new_task->death_flag = 0;
    new_task->time_spent = 0;
    new_task->page_directory = directory;
    new_task->heap = heap;

    current_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    //monitor_write("\nnew_kstack: \n");
    //monitor_write_hex(current_task->kernel_stack);

    new_task->parent = parent_task;
    new_task->first_child = 0;
    new_task->next = 0;
    new_task->next_sibling= 0;

    // Add it to the end of the linked list of children tasks
    task_t *child = parent_task->first_child;
    if (!child) {
        parent_task->first_child = new_task;
    } else {
        while (child->next_sibling)
            child = child->next_sibling;
        child->next_sibling = new_task;
    }   

    // Add it into the ready queue.
    add_to_ready_queue(new_task);

    // This will be the entry point for the new process.

    u32int eip = read_eip();
    // monitor_write("\n(fork) - eip: ");monitor_write_hex(eip);

    // We could be the parent or the child here - check.
    if (current_task == parent_task)
    {   
        //monitor_write("\nParent: (fork) \n eip:"); monitor_write_hex(eip);
        // We are the parent, so set up the esp/ebp/eip for our child.
        u32int esp; asm volatile("mov %%esp, %0" : "=r"(esp));
        u32int ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
        new_task->esp = esp;
        new_task->ebp = ebp;
        new_task->eip = eip;

        asm volatile("sti");

        return new_task->id;
    }
    else
    {
        // We are the child.
        return 0;
    }
}

void yield()
{
    switch_task();
}

void exit()
{
    task_t *dying = (task_t*)current_task;
    monitor_write("Exiting from Task ");
    monitor_write_dec(dying->id);
    monitor_write("\n");
    // set the death flag to 1
    dying->death_flag = 1;
    // if the task has no children, we can free all resources immediately
    if (!dying->first_child) {
        monitor_write("No children\n");
        free_heap(dying->heap);
        kfree(dying->page_directory);
        dying->prev = dying->next;
        dying->next = dying->prev;
        task_t *parent = dying->parent;
        if (parent->first_child->id == dying->id) {
            parent->first_child = 0;
        } else {
            task_t *sibling = parent->first_child;
            while (sibling->next)
                if (sibling->next->id == dying->id)
                    sibling->next = dying->next;
        }
        monitor_write("Dying...\n");
        kfree(dying);
        schedule(ready_queue);
    } else {
        // the dying task has children, which need to die first before it can die
        task_t *iterator = dying->first_child; 
        while(iterator) {
            monitor_write_dec(iterator->id);
            monitor_write("\n");
            if (iterator->death_flag != 1)
                iterator->death_flag = 1;
            if (iterator->priority < dying->priority)
                setpriority(iterator->id, dying->priority);
            iterator = iterator->next_sibling;
        }
        setpriority(dying->id, dying->priority + 1);
    }
    print_ready_queue();
    schedule();
}

// frees all resources allocated to a user heap
void free_heap(heap_u *heap) {
    if (!heap) return;
    heap_u *iterator = heap;
    while (iterator->next) {
        free(iterator->ptr);
        iterator = iterator->next;
        kfree(heap);
        heap = iterator;
    }
    free(heap->ptr);
    kfree(heap);
    return;
}

int setpriority(int pid, int new_priority) 
{
    /*
    monitor_write("Setting priority of ");
    monitor_write_dec(pid);
    monitor_write("\n");
    */
    task_t *iter = (task_t*)ready_queue;
    if (iter->id == pid) {
        ready_queue = ready_queue->next;
        ready_queue->prev = 0;
        iter->next = 0;
        iter->priority = new_priority;
        add_to_ready_queue(iter);
        return new_priority;
    }
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
        add_to_ready_queue(iter);
        return new_priority;
    } else {
        return 0;
    }
}

void add_to_ready_queue(task_t *task)
{
    /*
    monitor_write("Adding Task ");
    monitor_write_dec(task->id);
    monitor_write("\n");
    */
    if (!task) return;
    task_t *iterator = (task_t*)ready_queue;

    // edge case, one element queue
    if (!iterator->next) {
        if (task->priority <= iterator->priority) {
            task->next = iterator;
            iterator->prev = task;
            ready_queue = task;
        } else {
            iterator->next = task;
            task->prev = iterator;
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
        task->next = task;
    } else {
        iterator->next = task;
        task->prev = iterator;
        task->next = 0;
    }
    // print_ready_queue();
    // the task is now in the correct place in the queue
}

int getpid()
{
    return current_task->id;
}

void switch_to_user_mode()
{
   // Set up our kernel stack
   set_kernel_stack(current_task->kernel_stack+KERNEL_STACK_SIZE);

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
     pushl $0x1B; \
     push $1f; \
     iret; \
   1: \
     ");
}

void free(void *p)
{
  if (p == 0) {
    return;
  }
  heap_u *heap = current_task->heap;
  // check edge case where the pointer is in the first node
  if (heap->ptr == p) {
    heap_free(p, gheap);
    current_task->heap = current_task->heap->next;
    kfree(heap);
    return;
  }
  // continue on, knowing we have at least one node
  heap_u *prev = heap;
  heap = heap->next;
  while (heap) {
    if (heap->ptr == p) {
      heap_free(p, gheap);
      prev->next = heap->next;
      kfree(heap);
      return;
    }
    heap = heap->next;
    prev = prev->next;
  }
  // didn't find the pointer in our task's "heap", silently fail
  return;
}

void *alloc(u32int size, u8int page_align)
{
  // check if nothing has been allocated yet
  if (!current_task->heap) {
    current_task->heap = (heap_u*)kmalloc(sizeof(heap_u));
    current_task->heap->ptr = heap_alloc(size, page_align, gheap);
    current_task->heap->next = 0;
    return current_task->heap->ptr;
  }
  // otherwise, we've already allocated at least one thing
  heap_u *heap = current_task->heap;
  while (heap->next) {
    heap = heap->next;
  }
  heap->next = (heap_u*)kmalloc(sizeof(heap_u));
  heap->next->ptr = heap_alloc(size, page_align, gheap);
  heap->next->next = 0;
  return heap->next->ptr;
}

// u32int gmalloc(u32int sz, u8int align)
// {
//     if (gheap) {
//         void *addr = heap_alloc(sz, (u8int)align, gheap);
//         if (phys != 0)
//         {
//             page_t *page = get_page((u32int)addr, 0, current_directory);
//             *phys = page->frame*0x1000 + ((u32int)addr&0xFFF);
//         }
//         return (u32int)addr;
//     }
// }

void print_user_heap()
{
  heap_u *heap = current_task->heap;
  while (heap) {
    monitor_write("Heap: ");
    monitor_write_hex(heap);
    monitor_write("   Heap ptr: ");
    monitor_write_hex(heap->ptr);
    monitor_write("\n");
    heap = heap->next;
  }
  return;
}

void print_ready_queue()
{
    task_t *task = (task_t*)ready_queue;
    while (task) {
        monitor_write("Task ID: ");
        monitor_write_dec(task->id);
        monitor_write(" Priority: ");
        monitor_write_dec(task->priority);
        monitor_write("\n");
        task = task->next;
    }
}

// void heap_free(void *p)
// {
//   heap_u heap = current_task->heap;
//   while (heap) {
//     if (heap->ptr == p) {
//       heap_free(p, gheap);
//     }
//     heap = heap->next;
//   }
// }
