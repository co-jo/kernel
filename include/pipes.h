#ifndef PIPE_H_
#define PIPE_H_

#include "semaphores.h"

// INTERPROCESS COMMUNICATION:
// 
// pipes are first-in-first-out bounded buffers. Elements are read in the same
//   order as they were written. When writes overtake reads, the first unread
//   element will be dropped. Thus, ordering is always preserved.
//  "read" and "write" on pipes are atomic, i.e., they are indivisible, and
//   they are non-blocking. All pipes are of the same size.

#define INVALID_PIPE -1
#define BUF_SIZE 1024
#define MAX_PIPES 256

typedef struct pipe {
    char buffer[BUF_SIZE];   // a pipe can store up to 1KB of unread data
    int read_offset;
    int write_offset;
} pipe_t;

// Initialize a new pipe and returns a descriptor. It returns INVALID_PIPE
//   when none is available.
int open_pipe();

// Write the first nbyte of bytes from buf into the pipe fildes. The return value is the
//   number of bytes written to the pipe. If the pipe is full, no bytes are written.
//   Only write to the pipe if all nbytes can be written.
unsigned int write(int fildes, const void *buf, unsigned int nbyte);

// Read the first nbyte of bytes from the pipe fildes and store them in buf. The
//   return value is the number of bytes successfully read. If the pipe is 
//   invalid, it returns -1.
unsigned int read(int fildes, void *buf, unsigned int nbyte);

// Close the pipe specified by fildes. It returns INVALID_PIPE if the fildes
//   is not valid.
int close_pipe(int fildes);

#endif

