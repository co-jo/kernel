//            Written for JamesM's kernel development tutorials.

#ifndef KHEAP
#define KHEAP

#include "system.h"
#include "ordered_array.h"

typedef struct
{
    unsigned int magic;         // Magic number, used for error checking and identification.
    unsigned char is_hole;      // 1 if this is a hole. 0 if this is a block.
    unsigned int size;          // size of the block, including the end footer.
} header_t;

typedef struct
{
    unsigned int magic;         // Magic number, same as in header_t.
    header_t *header;           // Pointer to the block header.
} footer_t;

typedef struct
{
    ordered_array_t index;
    unsigned int start_address;
    unsigned int end_address;
    unsigned int max_address;
    unsigned short supervisor;
    unsigned char readonly;
    unsigned char initialized;
} heap_t;

void *create_heap(heap_t *heap, unsigned int start, unsigned int end, unsigned int max,
                  unsigned char supervisor, unsigned char readonly);

void *_alloc(unsigned int size, unsigned char page_align);

void _free(void *p, heap_t *heap);

unsigned int kmalloc(unsigned int size, unsigned int align, unsigned int *phys);

void kfree(void *p);

void heap_state(heap_t *heap);

#endif
