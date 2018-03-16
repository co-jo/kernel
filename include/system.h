#ifndef SYSTEM
#define SYSTEM

// FRAMES
#define MEMORY              0x01000000
#define MAX_ADDRESS         0xFFFFFFFF
#define MIN_ADDRESS         0x00000000

#define FRAME_SIZE          0x00001000
#define TABLE_SIZE          0x00000400
#define DIRECTORY_SIZE      0x00000400
#define NUM_FRAMES          MEMORY/FRAME_SIZE

// KHEAP
#define KHEAP_START         0xC0000000
#define KHEAP_INITIAL_SIZE  0x00100000
#define KHEAP_END           KHEAP_START + KHEAP_INITIAL_SIZE
#define KHEAP_MAX           0xCFFFF000

// HEAP INDEX
#define HEAP_INDEX_SIZE     0x00020000
#define HEAP_MAGIC          0x123890AB
#define HEAP_MIN_SIZE       0x00070000

// KSTACK
#define KERNEL_STACK_SIZE   0x500

typedef int size_t;

/* This defines what the stack looks like after an ISR was running */
typedef struct regs
{
  unsigned int gs, fs, es, ds;
  unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
  unsigned int int_no, err_code;
  unsigned int eip, cs, eflags, useresp, ss;
} regs_t;

typedef void (*isr_t)(regs_t*);

/* MAIN.C */
extern void *memcpy(void *dest, const void *src, size_t count);
extern void *memset(void *dest, char val, size_t count);
extern unsigned short *memsetw(unsigned short *dest, unsigned short val, size_t count);
extern size_t strlen(const char *str);
extern unsigned char inportb (unsigned short _port);
extern void outportb (unsigned short _port, unsigned char _data);

/* KEYBOARD.C */
// extern void keyboard_install();

/* HALT */
void halt(char *message);

#endif
