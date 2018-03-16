// paging.h -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#ifndef PAGING
#define PAGING

#include "system.h"
#include "isr.h"

typedef struct page
{
    unsigned int present    : 1;   // Page present in memory
    unsigned int rw         : 1;   // Read-only if clear, readwrite if set
    unsigned int user       : 1;   // Supervisor level only if clear
    unsigned int accessed   : 1;   // Has the page been accessed since last refresh?
    unsigned int dirty      : 1;   // Has the page been written to since last refresh?
    unsigned int unused     : 7;   // Amalgamation of unused and reserved bits
    unsigned int frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table
{
    page_t pages[1024];
} page_table_t;

typedef struct page_directory
{
    page_table_t *tables[1024];
    unsigned int phys_tables[1024];
} page_directory_t;


int flags(int present, int rw, int user);
int read_flags(page_t *page);

void identity_map();

void map_first_table();

/* Load Page Directory into CR3 */
void switch_page_directory(page_directory_t *directory);

page_directory_t *clone_directory(page_directory_t *src);

page_directory_t *create_initial_directory();

/* Init table if DNE, then go back and init requested page if DNE  */
page_t *get_page(unsigned int address, page_directory_t *dir, int flags);

void alloc_page(page_t *page, int flags);

void page_fault(regs_t *regs);

void clone_table(page_directory_t *dest, page_table_t *src, int tid);

void alloc_table(page_directory_t *dest, int tid, int flags);

void enable_paging(page_directory_t *directory);

unsigned int get_physical(unsigned int *address);

unsigned int first_frame();

unsigned int *create_frame_index();

#endif
