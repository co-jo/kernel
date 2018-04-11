#include "system.h"
#include "tests.h"
#include "task.h"
#include "kheap.h"
#include "paging.h"

void grow_heap()
{
  set_window_title("Running Grow Heap");
  int i;
  for (i = 0; i < 884; i++) {
      kmalloc(FRAME_SIZE, 0, 0);
  }
}

void shrink_heap()
{
}

void memory_tests()
{
  grow_heap();
  shrink_heap();
}

