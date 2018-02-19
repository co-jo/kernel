// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "task.h"
#include "syscall.h"

extern u32int placement_address;
u32int initial_esp;

int main(struct multiboot *mboot_ptr, u32int initial_stack)
{
    initial_esp = initial_stack;
    // Initialise all the ISRs and segmentation
    init_descriptor_tables();
    // Initialise the screen (by clearing it)
    monitor_clear();
    // Initialise the PIT to 100Hz
    asm volatile("sti");
    init_timer(50);

    // Start paging.
    initialise_paging();

    // Start multitasking.
    initialise_tasking();

    // Create a new process in a new address space which is a clone of this.
    initialise_syscalls();

    // int process = fork();

    // if (process == 2) {
    //     monitor_write("PID == 1\n");
    // } else if (process == 0) {
    //     monitor_write("PID == 0\n");
    // }

    // monitor_write("Process: ");
    // monitor_write_hex(process);
    // monitor_write("\n");

    switch_to_user_mode();

    int process = syscall_fork();

    syscall_monitor_write("\nProcess: ");
    syscall_monitor_write_hex(process);
    syscall_monitor_write("\n");

    if (process == 2) {
        // syscall_monitor_write("\n PID == 2");

    } else {
        // syscall_monitor_write("\n PID == 0");
    }

    return 0;
}
