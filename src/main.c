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
    monitor_write_hex(placement_address);

    monitor_write("\n");
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

    // switch_to_user_mode();

    int pid = fork();
    
    if (pid == 0) {
        monitor_write("In child 1\n");
        int i = 0;
        int j = 0;
        for (i = 0; i < 100000000; i++) {
            j++;
        }
        monitor_write_dec(j);
        monitor_write("\n");
    } else {
        int pid2 = fork();
        if (pid2 == 0) {
            monitor_write("In child 2\n");
            int i = 0;
            int j = 30;
            for (i = 0; i < 100000000; i++) {
                j++;
            }
            monitor_write_dec(j);
            monitor_write("\n");
        } else {
            monitor_write("In parent\n");
            int i = 0;
            int j = 50;
            for (i = 0; i < 100000000; i++) {
                j++;
            }
            monitor_write_dec(j);
            monitor_write("\n");
        }
    }
    // syscall_monitor_write("Hello, user world!\n");

    return 0;
}
