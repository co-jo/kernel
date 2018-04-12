#include "system.h"
#include "tests.h"
#include "task.h"
#include "kheap.h"
#include "paging.h"

void memory_tests()
{
    int *a = _alloc(4, 1);
    int pid = kfork();
    if (pid != 0) {
        *a = 4;
        assert_eq(*a, 4, "assigning to alloc'd memory in parent");
    } else {
        *a = 2;
        assert_eq(*a, 2, "assigning to alloc'd memory in child");
    }
    kfree(a);
    if (getpid() != 1) _exit();
}

