#include "task.h"
#include "paging.h"
#include "kheap.h"
#include "scrn.h"
#include "pipes.h"
#include "semaphores.h"

extern volatile task_t *current_task;

volatile pipe_t *_pipe_list = 0;
int _pipe_count = 1;

static void print_pipe(pipe_t *p)
{
    printf("Pipe ID: %d\n", p->id);
    printf("Pipe Addr: %x\n", p);
    return;
}


int open_pipe()
{
    pipe_t *new_pipe = (pipe_t*)kmalloc(sizeof(pipe_t));
    if (!new_pipe) {
        return INVALID_PIPE;
    }
    new_pipe->id = _pipe_count++;
    new_pipe->read_offset = 0;
    new_pipe->write_offset = 0;
    new_pipe->free_space = BUF_SIZE;
    new_pipe->next = _pipe_list;
    new_pipe->prev = 0;
    new_pipe->dirty = 0;
    new_pipe->sem = open_sem(1);
    _pipe_list = new_pipe;

    print_pipe(new_pipe);
    return new_pipe->id;
}

unsigned int write(int fd, const void *buf, unsigned int nbyte)
{
    pipe_t *pipe = find_pipe(fd);
    char *b = (char*)buf;
    if (!pipe) {
        return INVALID_PIPE;
    }
    // lock access to the pipe while we write to it
    wait(pipe->sem);
    int bytes_written = 0;
    if (pipe->free_space >= nbyte) { // there's enough space to write to the pipe
        int i;
        for (i = 0; i < nbyte; ++i) {
            pipe->buffer[pipe->write_offset] = b[i];
            pipe->write_offset = (pipe->write_offset+1) % BUF_SIZE;
        }
        bytes_written = nbyte;
        pipe->free_space -= nbyte;
        pipe->dirty = 1;
    }
    signal(pipe->sem); // signal that we're done
    return bytes_written;
}

unsigned int read(int fd, void *buf, unsigned int nbyte)
{
    pipe_t *pipe = find_pipe(fd);
    if (!pipe) {
        return INVALID_PIPE;
    }
    wait(pipe->sem);
    int bytes_read = 0;
    while (!pipe->dirty); // wait for the pipe to be written to
    
    int i;
    for (i = 0; (i < nbyte) && (((pipe->read_offset+i) % BUF_SIZE) != pipe->write_offset + 1); ++i) {
        ((char*)buf)[i] = pipe->buffer[pipe->read_offset];
        pipe->read_offset = (pipe->read_offset+1) % BUF_SIZE;
        ++bytes_read;
    }
    pipe->free_space += bytes_read;
    pipe->dirty = 0;
    signal(pipe->sem);
    return bytes_read;
}

int close_pipe(int fd)
{
    pipe_t *pipe = find_pipe(fd);
    if (!pipe) return INVALID_PIPE;

    if (pipe == _pipe_list) {
        _pipe_list = _pipe_list->next;
        _pipe_list->prev = 0;
    } else {
        pipe->prev = pipe->next;
    }
    if (pipe->next) {
        pipe->next = pipe->prev;
    }

    pipe->next = 0;
    pipe->prev = 0;
    kfree(pipe);
    return 0;
}

static pipe_t *find_pipe(int fd)
{
    pipe_t *temp = (pipe_t*)_pipe_list;
    while (temp) {
        if (temp->id == fd) {
            printf("Found pipe: %d\n", temp->id);
            return temp;
        } else {
            temp = temp->next;
        }
    }
    return 0;
}

