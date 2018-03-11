// kheap.c -- Kernel heap functions, also provides
//            a placement malloc() for use before the heap is
//            initialised.
//            Written for JamesM's kernel development tutorials.

#include "kheap.h"
#include "paging.h"
#include "system.h"


// end is defined in the linker script.
extern unsigned int end;
unsigned int placement_address = (unsigned int)&end;
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
heap_t *kheap;

unsigned int kmalloc_int(unsigned int sz, int align, unsigned int *phys)
{
  if (kheap->initialized == 1)
  {
    unsigned int addr = alloc(sz, (unsigned char)align, kheap);
    if (phys != 0)
    {
      page_t *page = get_page(addr, kernel_directory, 1, 0);
      // Assumes frame has already been mapped (as heap exists)
      *phys = page->frame*FRAME_SIZE + (addr & 0xFFF);
    }
    return addr;
  }
  else
  {
    if (align && not_aligned(placement_address)) {
      placement_address = align_to_next(placement_address);
    }

    unsigned int current_pos = placement_address;
    if (phys) {
      *phys = current_pos;
    }
    placement_address += sz;
    // Not sure if this is risky..
    // memset(current_pos, 0, sz);
    return current_pos;
  }
}

int not_aligned(unsigned int address)
{
  return (address % FRAME_SIZE == 0) ? 0 : 1;
}

int align_to_next(unsigned int address)
{
  unsigned int offset = address & (FRAME_SIZE - 1);
  address += FRAME_SIZE - offset;
  return address;
}

int align_to_prev(unsigned int address)
{
  unsigned int offset = address & (FRAME_SIZE - 1);
  address -= offset;
  return address;
}

void kfree(void *p)
{
  free(p, kheap);
}

unsigned int kmalloc_a(unsigned int sz)
{
  return kmalloc_int(sz, 1, 0);
}

unsigned int kmalloc_p(unsigned int sz, unsigned int *phys)
{
  return kmalloc_int(sz, 0, phys);
}

unsigned int kmalloc_ap(unsigned int sz, unsigned int *phys)
{
  return kmalloc_int(sz, 1, phys);
}

unsigned int kmalloc(unsigned int sz)
{
  return kmalloc_int(sz, 0, 0);
}

static void expand(unsigned int new_size, heap_t *heap)
{
  // Get the nearest following page boundary.
  if (new_size&0xFFFFF000 != 0)
  {
    new_size &= 0xFFFFF000;
    new_size += 0x1000;
  }

  // This should always be on a page boundary.
  unsigned int old_size = heap->end_address-heap->start_address;

  unsigned int i = old_size;
  while (i < new_size)
  {
    get_page(heap->start_address+i, kernel_directory, 1, 0);
    i += FRAME_SIZE;
  }
  heap->end_address = heap->start_address+new_size;
}

static unsigned int contract(unsigned int new_size, heap_t *heap)
{
  // Get the nearest following page boundary.
  if (new_size&0x1000)
  {
    new_size &= 0x1000;
    new_size += 0x1000;
  }

  // Don't contract too far!
  if (new_size < HEAP_MIN_SIZE)
    new_size = HEAP_MIN_SIZE;

  unsigned int old_size = heap->end_address-heap->start_address;
  unsigned int i = old_size - 0x1000;
  while (new_size < i)
  {
    free_frame(get_page(heap->start_address+i, kernel_directory, 1, 0));
    i -= 0x1000;
  }

  heap->end_address = heap->start_address + new_size;
  return new_size;
}

static int find_smallest_hole(unsigned int size, unsigned char page_align, heap_t *heap)
{
  // Find the smallest hole that will fit.
  unsigned int iterator = 0;
  while (iterator < heap->index.size)
  {
    header_t *header = (header_t *)lookup_ordered_array(iterator, &heap->index);
    // If the user has requested the memory be page-aligned
    if (page_align > 0)
    {
      // Page-align the starting point of this header.
      unsigned int location = (unsigned int)header;
      int offset = 0;
      if ((location+sizeof(header_t) & 0xFFFFF000) != 0)
        offset = FRAME_SIZE  - (location+sizeof(header_t)) & FRAME_SIZE;

      int hole_size = (int)header->size - offset;
      // If still fits
      if (hole_size >= (int)size) {
        break;
      }
    }
    else if (header->size >= size) {
      break;
    }
    iterator++;
  }
  int i;

  // Why did the loop exit?
  if (iterator == heap->index.size) {
    return -1; // We got to the end and didn't find anything.
  }
  else {
    return iterator;
  }
}

static char header_t_less_than(void*a, void *b)
{
  return (((header_t*)a)->size < ((header_t*)b)->size)?1:0;
}

void *create_heap(heap_t *heap, unsigned int start, unsigned int end,
                  unsigned int max, unsigned char supervisor,
                  unsigned char readonly)
{
  // Initialise the index.
  heap->index = place_ordered_array((void*)start, HEAP_INDEX_SIZE, &header_t_less_than);
  // Shift the start address forward to resemble where we can start putting data.
  start += sizeof(type_t) * HEAP_INDEX_SIZE;
  // Make sure the start address is page-aligned.
  start = not_aligned(start) ? align_to_next(start) : start;
  // This may or may not have implications if heap index gets full.
  start -= sizeof(header_t);

  header_t *hole = (header_t *)(start);
  // Write the start, end and max addresses into the heap structure.
  heap->start_address = start;
  heap->end_address = end;
  heap->max_address = max;
  heap->supervisor = supervisor;
  heap->readonly = readonly;
  heap->initialized = 1;

  // We start off with one large hole in the index.
  hole->size = end - start;
  hole->magic = HEAP_MAGIC;
  hole->is_hole = 1;

  insert_ordered_array((void*)hole, &heap->index);
}

unsigned int alloc(unsigned int size, unsigned char page_align, heap_t *heap)
{
  // Make sure we take the size of header/footer into account.
  unsigned int new_size = size + sizeof(header_t) + sizeof(footer_t);
  int iterator = find_smallest_hole(new_size, page_align, heap);

  if (iterator == -1) // If we didn't find a suitable hole
  {
    puts("Iteratior - 1");
    // Save some previous data.
    unsigned int old_length = heap->end_address - heap->start_address;
    unsigned int old_end_address = heap->end_address;

    // We need to allocate some more space.
    expand(old_length+new_size, heap);
    unsigned int new_length = heap->end_address-heap->start_address;

    // Find the endmost header. (Not endmost in size, but in location).
    iterator = 0;
    // Vars to hold the index of, and value of, the endmost header found so far.
    unsigned int idx = -1; unsigned int value = 0x0;
    while (iterator < heap->index.size)
    {
      unsigned int tmp = (unsigned int)lookup_ordered_array(iterator, &heap->index);
      if (tmp > value)
      {
        value = tmp;
        idx = iterator;
      }
      iterator++;
    }

    // If we didn't find ANY headers, we need to add one.
    if (idx == -1)
    {
      header_t *header = (header_t *)old_end_address;
      header->magic = HEAP_MAGIC;
      header->size = new_length - old_length;
      header->is_hole = 1;
      footer_t *footer = (footer_t *) (old_end_address + header->size - sizeof(footer_t));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
      insert_ordered_array((void*)header, &heap->index);
    }
    else
    {
      // The last header needs adjusting.
      header_t *header = lookup_ordered_array(idx, &heap->index);
      header->size += new_length - old_length;
      // Rewrite the footer.
      footer_t *footer = (footer_t *) ( (unsigned int)header + header->size - sizeof(footer_t) );
      footer->header = header;
      footer->magic = HEAP_MAGIC;
    }
    // We now have enough space. Recurse, and call the function again.
    return alloc(size, page_align, heap);
  }

  header_t *orig_hole_header = (header_t *)lookup_ordered_array(iterator, &heap->index);

  unsigned int orig_hole_pos = (unsigned int)orig_hole_header;
  unsigned int orig_hole_size = orig_hole_header->size;
  // Here we work out if we should split the hole we found into two parts.
  // Is the original hole size - requested hole size less than the overhead for adding a new hole?
  // ?
  if (orig_hole_size-new_size < sizeof(header_t)+sizeof(footer_t))
  {
    // Then just increase the requested size to the size of the hole we found.
    size += orig_hole_size-new_size;
    new_size = orig_hole_size;
  }

  int was_aligned = 0;
  int aligned_pos;
  int aligned_size;
  header_t *aligned_block;

  // Want to add hole in front of block but don't add it
  // if (page_align && orig_hole_pos&0xFFFFF000)
  if (page_align && not_aligned(orig_hole_pos + sizeof(header_t)));
  {
    // Data starts at page beginning
    was_aligned = 1;
    // Offet from the front of the hole (including the header) to the next page;
    int offset = (0x1000 /* page size */ - (orig_hole_pos&0xFFF)) - sizeof(header_t);
    // Where we want our block to be placed (Starts at header)
    unsigned int aligned_location   = orig_hole_pos + offset;

    // Note: Might have to adjust find_smallest_hole;
    // Data isnt already aligned, therefore we must shift the entry.
    aligned_block = (header_t *)(aligned_location);
    // Shifted by offset, Assumes find_smallest_hole accouns for shifting
    // We lose space in this step
    aligned_block->size = new_size;
    aligned_block->magic = HEAP_MAGIC;
    aligned_block->is_hole = 0;

    footer_t *block_footer = (footer_t *) ( (unsigned int)aligned_location - sizeof(footer_t) );
    block_footer->magic    = HEAP_MAGIC;
    block_footer->header   = aligned_block;

    aligned_pos = aligned_location;
  }

  // Overwrite the original header... ??

  header_t *block_header;

  if (was_aligned) {
    block_header = aligned_block;
  } else {
    block_header = (header_t *)orig_hole_pos;
    block_header->magic     = HEAP_MAGIC;
    block_header->is_hole   = 0;
    block_header->size      = new_size;

    // ...And the footer
    footer_t *block_footer  = (footer_t *) (orig_hole_pos + sizeof(header_t) + size);
    block_footer->magic     = HEAP_MAGIC;
    block_footer->header    = block_header;
  }

  // We may need to write a new hole after the allocated block.
  // We do this only if the new hole would have positive size...

  header_t *hole;
  if (orig_hole_size - new_size > (sizeof(header_t) + sizeof(footer_t)))
  {
    // Account for shifted header in placing new header below
    // Diff + (header + size + footer) = new_header_pos

    if (was_aligned) {
      new_size = ((unsigned int)aligned_block - orig_hole_pos) + new_size;
    }

    // Kinda confusing ^
    header_t *hole_header = (header_t *)(orig_hole_pos + new_size);

    hole_header->magic    = HEAP_MAGIC;
    hole_header->is_hole  = 1;
    hole_header->size     = orig_hole_size - new_size;

    footer_t *hole_footer = (footer_t *) ( (unsigned int)hole_header + orig_hole_size - new_size - sizeof(footer_t) );

    if ((unsigned int)hole_footer < heap->end_address)
    {
      hole_footer->magic = HEAP_MAGIC;
      hole_footer->header = hole_header;
    }

    hole = hole_header;

    // Remove hole we are occupying, then add new hole
    remove_ordered_array(iterator, &heap->index);
    // Inserting shrunken hole
    insert_ordered_array((void*)hole_header, &heap->index);
  }

  // Where you can place your data
  int block_ptr = (void *)((unsigned int)block_header + sizeof(header_t));
  return block_ptr;
}

void free(void *p, heap_t *heap)
{
  // Exit gracefully for null pointers.
  if (p == 0)
    return;

  // Get the header and footer associated with this pointer.
  header_t *header = (header_t*) ( (unsigned int)p - sizeof(header_t) );
  footer_t *footer = (footer_t*) ( (unsigned int)header + header->size - sizeof(footer_t) );

  // Make us a hole.
  header->is_hole = 1;

  // Do we want to add this header into the 'free holes' index?
  char do_add = 1;

  // Unify left
  // If the thing immediately to the left of us is a footer...
  footer_t *test_footer = (footer_t*) ( (unsigned int)header - sizeof(footer_t) );
  if (test_footer->magic == HEAP_MAGIC &&
      test_footer->header->is_hole == 1)
  {
    unsigned int cache_size = header->size; // Cache our current size.
    header = test_footer->header;     // Rewrite our header with the new one.
    footer->header = header;          // Rewrite our footer to point to the new header.
    header->size += cache_size;       // Change the size.
    do_add = 0;                       // Since this header is already in the index, we don't want to add it again.
  }

  // Unify right
  // If the thing immediately to the right of us is a header...
  header_t *test_header = (header_t*) ( (unsigned int)footer + sizeof(footer_t) );
  if (test_header->magic == HEAP_MAGIC &&
      test_header->is_hole)
  {
    header->size += test_header->size; // Increase our size.
    test_footer = (footer_t*) ( (unsigned int)test_header + // Rewrite it's footer to point to our header.
        test_header->size - sizeof(footer_t) );
    footer = test_footer;
    // Find and remove this header from the index.
    unsigned int iterator = 0;
    while ( (iterator < heap->index.size) &&
        (lookup_ordered_array(iterator, &heap->index) != (void*)test_header) )
      iterator++;
      
    // Remove it.
    remove_ordered_array(iterator, &heap->index);
  }

  // If the footer location is the end address, we can contract.
  if ( (unsigned int)footer+sizeof(footer_t) == heap->end_address)
  {
    unsigned int old_length = heap->end_address-heap->start_address;
    unsigned int new_length = contract( (unsigned int)header - heap->start_address, heap);
    // Check how big we will be after resizing.
    if (header->size - (old_length-new_length) > 0)
    {
      // We will still exist, so resize us.
      header->size -= old_length-new_length;
      footer = (footer_t*) ( (unsigned int)header + header->size - sizeof(footer_t) );
      footer->magic = HEAP_MAGIC;
      footer->header = header;
    }
    else
    {
      // We will no longer exist :(. Remove us from the index.
      unsigned int iterator = 0;
      while ( (iterator < heap->index.size) &&
          (lookup_ordered_array(iterator, &heap->index) != (void*)test_header) )
        iterator++;
      // If we didn't find ourselves, we have nothing to remove.
      if (iterator < heap->index.size)
        remove_ordered_array(iterator, &heap->index);
    }
  }

  // If required, add us to the index.
  if (do_add == 1)
    insert_ordered_array((void*)header, &heap->index);
}
