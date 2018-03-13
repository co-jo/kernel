// paging.c -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#include "paging.h"
#include "kheap.h"
#include "debug.h"
#include "system.h"

// The kernel's page directory
page_directory_t *kernel_directory;

// The current page directory;
page_directory_t *current_directory;

// A bitset of frames - used or free.
unsigned int *frames;

// Defined in kheap.c
extern unsigned placement_address;
extern heap_t *kheap;

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(32))
#define OFFSET_FROM_BIT(a) (a%(32))

// Static function to set a bit in the frames bitset
static void set_frame(int frame)
{
  unsigned int idx = INDEX_FROM_BIT(frame);
  unsigned int off = OFFSET_FROM_BIT(frame);
  frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(int frame)
{
  unsigned int idx = INDEX_FROM_BIT(frame);
  unsigned int off = OFFSET_FROM_BIT(frame);
  frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static unsigned int test_frame(unsigned int frame_addr)
{
  unsigned int frame = frame_addr/0x1000;
  unsigned int idx = INDEX_FROM_BIT(frame);
  unsigned int off = OFFSET_FROM_BIT(frame);
  return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
unsigned int first_frame()
{
  int i, j;
  for (i = 0; i < INDEX_FROM_BIT(NUM_FRAMES); i++)
  {
    if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
    {
      // at least one bit is free here.
      for (j = 0; j < 32; j++)
      {
        unsigned int toTest = 0x1 << j;
        if ( !(frames[i]&toTest) )
        {
          return i*4*8+j;
        }
      }
    }
  }
}

// Function to allocate a frame.
void alloc_frame(page_t *page, int user, int writeable)
{
  if (page->frame != 0) return;
  else
  {
    unsigned int frame = first_frame();
    set_frame(frame);
    page->present = 1;
    page->rw = writeable;
    page->user = user;
    page->frame = frame;
  }
}

// Function to deallocate a frame.
void free_frame(page_t *page)
{
  int frame = page->frame;
  if (!frame) return;
  else
  {
    clear_frame(frame);
    page->frame = 0x0;
  }
}

page_directory_t *create_initial_directory()
{
  int size = sizeof(page_directory_t);
  kernel_directory = (page_directory_t*)kmalloc_a(size);
  memset(kernel_directory, 0, size);
}

unsigned int* create_frame_index()
{
  int size = NUM_FRAMES * sizeof(int) / 32;
  unsigned int *address = kmalloc_a(size);
  memset(address, 0, size);
  return address;
}

void direct_memory_map()
{
  int i;
  for (i = 0; i < placement_address + FRAME_SIZE; i += FRAME_SIZE) {
    page_t *page =  get_page(i, kernel_directory, 1, 0);
    if (i/FRAME_SIZE != page->frame) {
      printf("[%x] - ", i);
      printf("[%x]\n", page->frame);
      halt("DMM Failed.");
    }
  }
}

void map_first_table()
{
  unsigned int phys;
  page_table_t *table = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
  // Set page pointers to 0
  memset(table, 0, FRAME_SIZE);
  kernel_directory->tables[0] = phys;
  kernel_directory->phys_tables[0] = phys | 0x07;

  unsigned int frame = phys >> 12;    // Bits [32 - 12]
  unsigned int tid = frame >> 10;        // Bits [32 - 22]
  unsigned int pid = frame & 0x3FF;      // Bits [22 - 12]

  page_t *page = &kernel_directory->tables[tid]->pages[pid];

  page->accessed = 1;
  page->present = 1;
  page->rw = 0;
  page->user = 0;
  page->frame = frame;

  set_frame(page->frame);
}


void initialise_paging()
{
  // Frame Index
  frames = create_frame_index();
  // KHEAP Pointer
  kheap = kmalloc(sizeof(heap_t));
  memset(kheap, 0, sizeof(heap_t));

  // Allocate our first directory
  create_initial_directory();

  // Manual allocation for first table
  map_first_table();
  // 1:1 Mapping
  direct_memory_map();

  int i = 0;
  for (i = KHEAP_START; i < KHEAP_END; i += FRAME_SIZE) {
    page_t *page = get_page(i, kernel_directory, 1, 0);
  }
  enable_paging(kernel_directory);

  register_interrupt_handler(14, page_fault);

  create_heap(kheap, KHEAP_START, KHEAP_END, KHEAP_MAX, 0, 0);

  current_directory = clone_directory(kernel_directory);
  switch_page_directory(current_directory);
}

void switch_page_directory(page_directory_t *dir)
{
  current_directory = dir;
  unsigned int physical = get_physical(dir->phys_tables);
  asm volatile("mov %0, %%cr3":: "r"(physical));
}
// Loads the base of our physical table, and set bit 32
void enable_paging(page_directory_t *dir)
{
  current_directory = dir;
  unsigned int pg;
  unsigned int physical = dir->phys_tables;
  asm volatile("mov %0, %%cr3":: "r"(physical));
  asm volatile("mov %%cr0, %0": "=r"(pg));
  asm volatile("mov %0, %%cr0":: "r"(pg|0x80000000));
}

page_t *get_page(unsigned int address, page_directory_t *dir, int rw, int user)
{
  unsigned int offset = address & 0xFFF; // Bits [12 - 0]
  unsigned int frame = address >> 12;    // Bits [32 - 12]
  unsigned int tid = frame >> 10;        // Bits [32 - 22]
  unsigned int pid = frame & 0x3FF;      // Bits [22 - 12]
  // Address of table pointer

  // Table DNE
  if (!dir->tables[tid]) {
    // We need one frame & page to map the table to memoryk
    alloc_table(dir, tid, rw, user);
  }

  page_t *page = &dir->tables[tid]->pages[pid];

  if (page->present == 0) {
    alloc_page(page, rw, user);
    return page;
  } else {
    return page;
  }
}

void alloc_table(page_directory_t *dir, int tid, int rw, int user)
{
  unsigned int phys;
  page_table_t *table = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
  // Set page pointers to 0
  memset(table, 0, FRAME_SIZE);
  // This table also needs to be mapped to a frame;
  dir->tables[tid] = table;
  dir->phys_tables[tid] = phys | 0x7; // PRESENT, RW, US. //

  page_t *page = get_page(table, dir, rw, user);
}

// This assumes that a table exists
void alloc_page(page_t *page, int rw, int user)
{
  if (page->frame) {
    return;
  }
  else {
    int frame = first_frame();
    set_frame(frame);
    page->frame = frame;
    page->present = 1;
    page->rw = rw;
    page->user = user;
    page->accessed = 1;
  }
}

void page_fault(regs *regs)
{
  // The faulting address is stored in the CR2 register.
  unsigned int faulting_address;
  asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

  int present = (regs->err_code & 0x1) ? 1 : 0;     // Page not present
  int rw = (regs->err_code & 0x2);                  // Write operation?
  int us = (regs->err_code & 0x4) ? 1 : 0;          // Processor was in user-mode?
  int reserved = (regs->err_code & 0x8) ? 1 : 0;    // Overwritten CPU-reserved bits of page entry?
  int id = (regs->err_code & 0x10) ? 1 : 0;         // Caused by an instruction fetch?

  printf("Page Fault @ [%x]\n", faulting_address);
  printf("PR?: [%x] ", present);
  printf("RW?: [%x] ", rw);
  printf("US?: [%x] ", us);
  printf("RS?: [%x] ", reserved);
  printf("IF?: [%x]\n", id);
  halt("...");
}

void clone_table(page_directory_t *dest, page_table_t *src, int tid)
{
  alloc_table(dest, tid, 1, 1);
  page_table_t *table = dest->tables[tid];
  // For every entry in the table...
  int i;
  for (i = 0; i < TABLE_SIZE; i++) {
    // If the source entry has a frame associated with it...
    if (src->pages[i].frame) {
      // If not linked, is user page
      alloc_page(&table->pages[i], 1, 1);
      // Clone the flags from source to destination.
      if (src->pages[i].present) table->pages[i].present = 1;
      if (src->pages[i].rw) table->pages[i].rw = 1;
      if (src->pages[i].user) table->pages[i].user = 1;
      if (src->pages[i].accessed) table->pages[i].accessed = 1;
      if (src->pages[i].dirty) table->pages[i].dirty = 1;
      // Physically copy the data across. This function is in process.s.
      copy_page_physical(src->pages[i].frame*0x1000, table->pages[i].frame*0x1000);
    }
  }
}

void link_table(page_directory_t *dest, page_directory_t *src, int tid)
{
  dest->tables[tid] = src->tables[tid];
  dest->phys_tables[tid] = src->phys_tables[tid];
}

page_directory_t *clone_directory(page_directory_t *src)
{
  unsigned int phys;
  // Frame of where the page directory is placed
  int size = sizeof(page_directory_t);
  // Make a new page directory and obtain its physical address.
  page_directory_t *dir = (page_directory_t*)kmalloc_ap(size, &phys);
  memset(dir, 0, size);

  /* Link Kernel Tables, else copy */
  int i;
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (!src->tables[i])
      continue;
    if (kernel_directory->tables[i] == src->tables[i])
      link_table(dir, src, i);
    else
      clone_table(dir, src->tables[i], i);
  }
  return dir;
}

unsigned int get_physical(unsigned int *virtual_address)
{
  page_t *page = get_page(virtual_address, current_directory, 0, 0);
  return page->frame * FRAME_SIZE;
}

void print_table(int table_id)
{
  int i;
  for (i = 0; i < 1024; i++) {
    page_t page = current_directory->tables[table_id]->pages[i];
    if (page.frame)
      printf("Page: %x\n", current_directory->tables[table_id]->pages[i]);
  }
}
