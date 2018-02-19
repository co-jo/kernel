    int pid = fork();
    
    if (pid == 0) {
        monitor_write("In child 1\n");
        int i = 0;
        int j = 0;
        for (i = 0; i < 100000000; i++) {
            j++;
        }
        monitor_write_dec(j);
        monitor_write("\n");
    } else {
        int pid2 = fork();
        if (pid2 == 0) {
            monitor_write("In child 2\n");
            int i = 0;
            int j = 30;
            for (i = 0; i < 100000000; i++) {
                j++;
            }
            monitor_write_dec(j);
            monitor_write("\n");
        } else {
            monitor_write("In parent\n");
            int i = 0;
            int j = 50;
            for (i = 0; i < 100000000; i++) {
                j++;
            }
            monitor_write_dec(j);
            monitor_write("\n");
        }
    }