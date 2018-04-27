# CS4405 - Real-Time OS Project

This is a UNIX like, Real-Time OS for our Advanced Operating Systems course (CS4405) at UNB. Developed by me and [@jdgunter](https://github.com/jdgunter). It was initially based off of James Molloy's tutorial(which was based off of Bran's) but highly recommend only using it as a rough guide due to its poor design and volitile nature.


We implement the following mostly successfully with the expection of Heap memory managment
due to time restrictions (somewhat works but highly volitile).

+ IPC/Pipes (write, read)
+ Paging/Virtual Memory (alloc, free)
+ Pre-emptive Tasking (yield, sleep)
+ Priority Based Scheduling (setpriority)
+ Multi-processing (fork, exit)
+ Synchronization/Semaphores (wait, signal, open, close)
+ Console Output (Including scrolling up/down)
+ Keyboard Support (Command Inputs)
+ Test Cases

#### Running
`make && make run`

### Compiling/Requisites
+ QEMU (i386)
+ GCC Cross Compilier (i386)
+ NASM
