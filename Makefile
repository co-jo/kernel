# Makefile for JamesM's kernel tutorials.
# The C and C++ rules are already setup by default.
# The only one that needs changing is the assembler
# rule, as we use nasm instead of GNU as.

CSRC 	= $(shell find ./ -name '*.c')
ASRC 	= src/boot.s $(wildcard src/*/*.s)
OSRC 	= $(patsubst %.s, %.o, $(ASRC))
#OUT	= kernel
SOURCES = $(OSRC) $(CSRC)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

CFLAGS = -m32 -nostdlib -nostdinc -Wall -fno-builtin -fno-stack-protector -Iinclude -Tlink.ld
LDFLAGS= -melf_i386 -Tlink.ld
ASFLAGS= -felf
OUTPUT=kernel

all: clean $(OBJECTS) link

clean:
	-rm src/*.o src/*/*.o kernel

link:
	ld $(LDFLAGS) -o kernel $(OBJECTS)

run:
	fcs-qemu-run-kernel -kernel kernel -bootdisk floppy.img

print-%: ; @echo $* = $($*)

.s.o:
	nasm $(ASFLAGS) $<
