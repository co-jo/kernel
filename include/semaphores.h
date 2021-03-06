#ifndef SEM_H_
#define SEM_H_

#include "task.h"

struct sem_t {
    int id;
    unsigned int size;
    unsigned int num_held;
    int closing;
    task_t *wait_list;
    task_t *wait_list_end;
    int *tasks_held;
    struct sem_t *next;
    struct sem_t *prev;
};
typedef struct sem_t sem_t;

//   A semaphore must be initialized by open_sem() before it can be used.
//   Processes waiting on a semaphore are resumed on a first-come first-served
//   basis.

//  n is the number of processes that can be granted access to the critical
//  region for this semaphore simultaneously. The return value is a semaphore
//  identifier to be used by signal and wait. 0 indicates open_sem failed.
int _open_sem(int n);

// The invoking process is requesting to acquire the semaphore, s. If the
//   internal counter allows, the process will continue executing after acquiring
//   the semaphore. If not, the calling process will block and release the
//   processor to the scheduler. Returns semaphore id on success of acquiring
//   the semaphore, 0 on failure.
int _wait(int s);

// The invoking process will release the semaphore, if and only if the process
//   is currently holding the semaphore. If a process is waiting on
//   the semaphore, the process will be granted the semaphore and if appropriate
//   the process will be given control of the processor, i.e. the waking process
//   has a higher scheduling precedence than the current process. The return value
//   is the seamphore id on success, 0 on failure.
int _signal(int s);

// Close the semaphore s and release any associated resources. If s is invalid then
//   return 0, otherwise the semaphore id.
int _close_sem(int s);

// Gets the semaphore with identifier s
static sem_t *find_sem(int s);

#endif
