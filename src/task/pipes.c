#include "task.h"
#include "paging.h"
#include "kheap.h"
#include "scrn.h"
#include "pipes.h"

extern volatile task_t *current_task;

volatile pipe_t *_pipes = 0;
int _pipe_count = 1;

int open_pipe()
{
    pipe_t *new_pipe = kmalloc(sizeof(pipe_t));
    new_pipe->id = _pipe_count++;
    new_pipe->read_offset = 0;
    new_pipe->write_offset = 0;
    new_pipe->dirty = 0;
    new_pipe->next = _pipes;
    new_pipe->prev = 0;
}
