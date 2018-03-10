# Makefile for JamesM's kernel tutorials.
# The C and C++ rules are already setup by default.
# The only one that needs changing is the assembler 
# rule, as we use nasm instead of GNU as.

SOURCES=boot.o 								\
				system.o 							\
				main.o								\
				irq.o									\
				isr.o 								\
				dt.o									\
				gdt.o 								\
				idt.o								  \
				timer.o								\
        kheap.o 							\
				paging.o 							\
				ordered_array.o 			\
				task.o 								\
				process.o 						\
				syscall.o							\
				debug.o 							\
				scrn.o								\

CFLAGS = -I ./include/ -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector
LDFLAGS= -melf_i386 -Tlink.ld
ASFLAGS= -felf

all: $(SOURCES) link run

clean:
	-rm *.o kernel

link:
	ld $(LDFLAGS) -o kernel $(SOURCES)

run:
	fcs-qemu-run-kernel -kernel kernel -bootdisk ../floppy.img

.s.o:
	nasm $(ASFLAGS) $<
