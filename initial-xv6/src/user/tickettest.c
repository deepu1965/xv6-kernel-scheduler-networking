#include "kernel/types.h"
#include "user/user.h"

int main() {
  for(int i = 0; i < 1000000000; i++) {
    if(i % 10000000 == 0) {
      printf("PID %d: %d\n", getpid(), i);
      sleep(1);  // Yield the CPU
    }
  }
  exit(0);
}