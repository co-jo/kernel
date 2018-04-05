#include "task.h"
#include "paging.h"
#include "kheap.h"
#include "scrn.h"
#include "pipes.h"

extern volatile task_t *current_task;

pipe_t pipes[MAX_PIPES];
char used_pipes[MAX_PIPES] = { 0 };

// static void set_bit(int index)
// {
//     if ((index < 0) || (index >= MAX_PIPES)) return;
//     int arr_index = index >> 5; // divide by 32
//     int byte_index = index & 0x1F; // mask out bottom five bits
//     unsigned int mask = 1 << byte_index;
//     pipes_bitset[arr_index] &= mask;
// }
// 
// static void clear_bit(int index)
// {
//     if ((index < 0) || (index >= MAX_PIPES)) return;
//     int arr_index = index >> 5; // divide by 32
//     int byte_index = index & 0x1F; // mask out bottom five bits
//     char mask = 1 << byte_index;
//     pipes_bitset[arr_index] &= ~mask;
// }
// 
// static int is_bit_set(int index)
// {
//     if ((index < 0) || (index >= MAX_PIPES)) return 0;
//     int arr_index = index >> 5; // divide by 32
//     int byte_index = index & 0x1F; // mask out bottom five bits
//     unsigned int mask = 1 << byte_index;
//     return (pipes_bitset[arr_index] & mask) ? 1 : 0;
// }
// 
// static int first_free_pipe()
// {
//     int i;
//     for (i = 0; i < MAX_PIPES/32; ++i) {
// 	if (pipes_bitset[i] == 0xFFFFFFFF) // all bits set, nothing free
// 	    continue;
// 	else {
// 	    int j;
// 	    for (j = 0; j < 32; ++j) {
// 		if (!(pipes_bitset[i] & (1<<j))) {
// 		    return 32*i + j;
// 		}
// 	    }
// 	}
//     }
//     // no pipes free
//     return INVALID_PIPE;
// }

static int first_free_pipe()
{
    int i;
    for (i = 0; i < MAX_PIPES; ++i) {
	if (used_pipes[i]) {
	    continue;
	} else {
	    return i;
	}
    }
    return -1;
}

int open_pipe()
{
    int fildes = first_free_pipe();
    if (fildes == INVALID_PIPE) { return fildes; }
    used_pipes[fildes] = 1;
    memset(pipes[fildes].buffer, 0, 1024);
    pipes[fildes].read_offset = 0;
    pipes[fildes].write_offset = 0;
    return fildes;
}

unsigned int write(int fildes, const void *buf, unsigned int nbytes)
{
    asm volatile("cli");
    printf("Writing to: pipe [%d]\n", fildes);
    if (!used_pipes[fildes]) {
	asm volatile("sti");
	return 0;
    }
    
    pipe_t pipe = pipes[fildes];
    char *wbuf = buf;
    
    int enough_space = 0;
    // checks to make sure we have enough space in the buffer
    if ((pipe.read_offset <= pipe.write_offset)
	&& ((pipe.write_offset + nbytes) % BUF_SIZE) < pipe.read_offset) {
	enough_space = 1;
    } else if ((pipe.read_offset > pipe.write_offset)
	       && (pipe.write_offset + nbytes < pipe.read_offset)) {
	enough_space = 1;
    }

    if (!enough_space) {
	asm volatile("sti");
	return 0;
    }

    int i, write_index = pipe.write_offset;
    for (i = 0; i < nbytes; i++) {
	pipe.buffer[write_index] = wbuf[i];
	write_index = (write_index + 1) % BUF_SIZE;
    }
    pipe.write_offset = write_index;

    asm volatile("sti");
    return nbytes;
}

unsigned int read(int fildes, void *buf, unsigned int nbytes)
{
    printf("Reading from: pipe [%d]\n", fildes);
    if (!used_pipes[fildes]) {
	asm volatile("sti");
	return 0;
    }
   
    pipe_t pipe = pipes[fildes];
    char *rbuf = buf;
    
    int i, bytes_read = 0, read_index = pipe.read_offset;
    for (i = 0; i < nbytes; i++) {
	rbuf[i] = pipe.buffer[read_index];
	read_index = (read_index + 1) % BUF_SIZE;
	bytes_read++;
	if (read_index == pipe.write_offset) {
	    asm volatile("sti");
	    return bytes_read;
	}
    }
    pipe.read_offset = read_index;

    asm volatile("sti");
    return bytes_read;
}

int close_pipe(int fildes)
{
    if (!used_pipes[fildes])
	return INVALID_PIPE;

    // clear
    used_pipes[fildes] = 0;

    return fildes;
}
