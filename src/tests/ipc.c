#include "system.h"
#include "tests.h"
#include "pipes.h"
#include "task.h"

void ipc_tests()
{
    int pipefd = _open_pipe();
    assert_neq(pipefd, -1, "Created pipe");
    int child = kfork();
    if (child > 0) {
        //puts("In Child..\n");
	char out_buf[12] = "hello world\0";
	int ret = _write(pipefd, out_buf, 12);
	assert_eq(ret, 12, "Wrote 12 bytes to pipe in parent");
	char out_buf2[10] = "more data\0";
	ret = _write(pipefd, out_buf2, 10);
	assert_eq(ret, 10, "Wrote 10 bytes to pipe in parent");
	setpriority(1, 6); // lower parent priority and yield to ensure child is entered
    } else {
        // puts("In Parent..\n");
        char in_buf[12];
        int ret = _read(pipefd, in_buf, 12);
        assert_eq(ret, 12, "Read 12 bytes from pipe in child");
        assert_eq(strcmp(in_buf, "hello world"), 1, "Read bytes equal to written bytes1");
        char in_buf2[10];
        ret = _read(pipefd, in_buf2, 10);
        assert_eq(ret, 10, "Read 10 bytes from pipe in child");
        assert_eq(strcmp(in_buf2, "more data"), 1, "Read bytes equal to written bytes2");
    }
    
    //puts("After close pipe...\n");
    char buf[4] = "abcd";
    ////close pipe doesn't return -1 when invalid pipe passed in
    assert_eq(_close_pipe(100), -1, "Closing invalid pipe");
    assert_eq(_write(100, buf, 4), -1, "Writing to invalid pipe");
    assert_eq(_read(100, buf, 4), -1, "Reading from invalid pipe");
    
    /*
      int i;/
      while (_open_pipe() != -1);
      assert_eq(1, 0, "Returns INVALID_PIPE when no pipes left");
      print("Completed Communication Testing...\n\n");
    */
    if (getpid() == 0x1) {
	exit();
    }
}
