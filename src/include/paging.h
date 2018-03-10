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
    /**
       Array of pointers to pagetables.
    **/
    page_table_t *tables[1024];
    /**
       Array of pointers to the pagetables above, but gives their *physical*
       location, for loading into the CR3 register.
    **/
    unsigned int phys_tables[1024];
} page_directory_t;

/**
   Sets up the environment, page directories etc and
   enables paging.
**/
void initialise_paging();

/**
   Causes the specified page directory to be loaded into the
   CR3 register.
**/
void switch_page_directory(page_directory_t *directory);

/**
   Retrieves a pointer to the page required.
   If make == 1, if the page-table in which this page should
   reside isn't created, create it!
**/
page_t *get_page(unsigned int address, page_directory_t *dir, int rw, int user);

/**
   Handler for page faults.
**/
void page_fault(regs *regs);

/**
   Makes a copy of a page directory.
**/
page_directory_t *clone_directory(page_directory_t *src);

page_directory_t *create_initial_directory();

unsigned int *create_frame_index();

void direct_map(page_t *page, unsigned int address, int writeable);

void alloc_page(page_t *page, int rw, int user);

void alloc_table(page_directory_t *dir, int tid, int rw, int user);

unsigned int *create_frame_index();

void direct_memory_map();

void map_first_table();

void enable_paging(unsigned int table_base);

unsigned int get_physical(unsigned int *address);

unsigned int first_frame();

#endif
