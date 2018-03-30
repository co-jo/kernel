#include "semaphores.h"
#include "task.h"
#include "kheap.h"

extern volatile task_t *current_task;

volatile sem_t *sem_list = 0;

unsigned int sem_count = 0;

int open_sem(int n) 
{
    sem_t *new_sem = kmalloc(sizeof(sem_t));
    new_sem->id = ++sem_count;
    new_sem->size = n;
    new_sem->num_held = 0;
    new_sem->wait_list = 0;
    new_sem->wait_list_end = 0;
    new_sem->closing = 0;
    new_sem->tasks_held = kmalloc(n * sizeof(int));
    int i;
    for (i = 0; i < n; ++i) {
        new_sem->tasks_held[i] = 0;
    }
    new_sem->prev = 0;
    if (!sem_list) {
        sem_list = new_sem;
        new_sem->next = 0;
    } else {
        sem_list->prev = new_sem;
        new_sem->next = sem_list;
        sem_list = new_sem;
    }
    return new_sem->id;
}

sem_t *find_sem(int s)
{
    sem_t *temp = (sem_t*)sem_list;
    while (temp) {
        if (temp->id == s) {
            return temp;
        } else {
            temp = temp->next;
        }
    }
    return 0;
}

int wait(int s)
{
    sem_t *sem = find_sem(s);
    if (!sem) return 0;

    // disable interrupts 
    asm volatile("cli");
    
    // first check whether the current task is already holding the semaphore
    int already_held = 0, i;
    for (i = 0; i < sem->size; ++i) {
        if (sem->tasks_held[i] == current_task->id) {
            already_held = 1;
            break;
        }
    }
    if (already_held) {
        asm volatile("sti");
        return 0;
    }

    // this task isn't already holding this semaphore
    // check if the semaphore has any room left
    if (sem->num_held < sem->size) {
        // the semaphore has room left, add this task id to the array of tasks holding this
        // semaphore and return the id
        for (i = 0; i < sem->size; ++i) {
            if (!sem->tasks_held[i]) {
                sem->tasks_held[i] = current_task->id;
                break;
            }
        }
        sem->num_held++;
        return sem->id;
    } 
    else {
        // this semaphore does not have room left:
        // remove it from the ready queue, add the task to the end of the wait queue, 
        // and yield the processor
        task_t *waiting_task = dequeue_task();
        sem->wait_list_end->next = waiting_task;
        waiting_task->prev = sem->wait_list_end;
        sem->wait_list_end = waiting_task;
        if (!sem->wait_list) 
            sem->wait_list = sem->wait_list_end;

        asm volatile("sti");
        yield();
        
        // we'll return here after being put back in the ready queue
        // by the signal function signalling that there's free space in the semaphore
        return sem->id;
    }

    // this shouldn't be reachable
    asm volatile("sti");
    return 0;
}

int signal(int s)
{
    sem_t *sem = find_sem(s);
    if (!sem) return 0;

    // check whether this task is actually holding the semaphore
    int held = 0, i, index;
    for (i = 0; i < sem->size; ++i) {
        if (sem->tasks_held[i] == current_task->id) {
            held = 1;
            index = i;
            break;
        }
    }
    if (!held) return 0;

    // the task is holding the semaphore, so we can release it
    sem->tasks_held[index] = 0;

    // if there's a task waiting to use the semaphore, add it to the array of
    // tasks holding the semaphore
    if (sem->wait_list) {
        task_t *next_task = sem->wait_list;
        sem->wait_list = sem->wait_list->next;
        if (!sem->wait_list) {
            sem->wait_list_end = 0; // set the end of the list to null if there's nothing left
        }
        sem->tasks_held[index] = next_task->id;
        enqueue_task(next_task);
        // yield if the new task has a higher priority
        if (next_task->priority > current_task->priority) {
            yield();
        }
    } else {
        sem->num_held--;
    }
    
    int id = sem->id;
    if ((sem->num_held==0) && sem->closing) {
        close_sem(s);
    }

    return id;
}

int close_sem(int s)
{
    sem_t *sem = find_sem(s);
    if (!sem) 
        return 0;

    int id = sem->id;
    if (sem->wait_list) { // some tasks are still using this semaphore
        sem->closing = 1; // let them clean up when they're done
        return id;
    }
    
    sem->wait_list_end = 0;
    kfree(sem->tasks_held);
    sem->prev = sem->next;
    sem->next = sem->prev;
    sem->next = 0;
    sem->prev = 0;
    kfree(sem);

    return id;
}
