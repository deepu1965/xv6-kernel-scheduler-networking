#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/syscall.h"
char* syscallname(int mask);
char *syscall_names[] = {
    [SYS_fork]      = "fork",
    [SYS_exit]      = "exit",
    [SYS_wait]      = "wait",
    [SYS_pipe]      = "pipe",
    [SYS_read]      = "read",
    [SYS_kill]      = "kill",
    [SYS_exec]      = "exec",
    [SYS_fstat]     = "fstat",
    [SYS_chdir]     = "chdir",
    [SYS_dup]       = "dup",
    [SYS_getpid]    = "getpid",
    [SYS_sbrk]      = "sbrk",
    [SYS_sleep]     = "sleep",
    [SYS_uptime]    = "uptime",
    [SYS_open]      = "open",
    [SYS_write]     = "write",
    [SYS_mknod]     = "mknod",
    [SYS_unlink]    = "unlink",
    [SYS_link]      = "link",
    [SYS_mkdir]     = "mkdir",
    [SYS_close]     = "close",
    [SYS_getSysCount] = "getSysCount"
    // Add more syscalls as needed
};

// syscount <mask> command [args]
int
main(int argc, char *argv[])
{
  if(argc < 3) {
    fprintf(2, "Usage: syscount <mask> command [args]\n");
    exit(1);
  }

  int mask = atoi(argv[1]);
  if(mask == 0) {
    fprintf(2, "Invalid mask\n");
    exit(1);
  }

  int pid = fork();
  if(pid < 0) {
    fprintf(2, "fork failed\n");
    exit(1);
  }
    if (pid == 0) {
    // Child process: run the specified command
    exec(argv[2], argv + 2);  // Correctly pass command and arguments
    fprintf(2, "exec %s failed\n", argv[2]);
    exit(1);
    }
  else {
    // Parent process: wait for the child to finish
    wait(0);

    // Get the count of the specified syscall using getSysCount
    int count = getSysCount(mask);
    if(count < 0) {
      fprintf(2, "getSysCount failed\n");
      exit(1);
    }

    // Print the result
    printf("PID %d called %s %d times.\n", pid, syscallname(mask), count);
  }

  exit(0);
}
char* syscallname(int mask) {
    for (int i = 0; i < sizeof(syscall_names) / sizeof(char*); i++) {
        if (mask == (1 << i)) {
            return syscall_names[i] ? syscall_names[i] : "unknown";
        }
    }
    return "unknown";
}


