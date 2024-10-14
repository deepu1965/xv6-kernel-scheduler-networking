#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}
uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;

  argint(0, &interval);
  argaddr(1, &handler);
  
  struct proc *p = myproc();
  p->alarm_interval = interval;
  p->alarm_handler = (void(*)())handler;
  p->ticks_count = 0;
  p->alarm_on = (interval > 0);
  p->alarm_tf = 0;

  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  if(p->alarm_tf){
    memmove(p->trapframe, p->alarm_tf, sizeof(struct trapframe));
    kfree(p->alarm_tf);
    p->alarm_tf = 0;
    p->ticks_count = 0;  // Reset the tick count
  }
  return 0;
}
uint64
sys_settickets(void)
{
  int number;
  struct proc *p = myproc();

  argint(0, &number);
  
  if(number <= 0)
    return -1;

  p->tickets = number;
  return number;
}
// System call to get the count of a specific syscall
uint64
sys_getSysCount(void)
{
  struct proc *p = myproc();
  int mask;

  // Get the mask from the arguments
  argint(0, &mask);

  int total_count = 0;

  // Count the syscalls for the current process
  for (int i = 0; i < NELEM(p->syscall_count); i++)
  {
    if ((1 << i) & mask)
    {
      total_count += p->syscall_count[i];
    }
  }

  // Recursively count the syscalls of child processes
  struct proc *child;

  // No need to acquire a global lock, but iterate through the process table
  for (child = proc; child < &proc[NPROC]; child++)
  {
    // Check if this process is a child of the current process
    if (child->parent == p)
    {
      // Acquire lock on the child process for safety
      acquire(&child->lock);

      // Count syscalls for the child process
      for (int i = 0; i < NELEM(child->syscall_count); i++)
      {
        if ((1 << i) & mask)
        {
          total_count += child->syscall_count[i];
        }
      }

      // Release the lock after reading the child's syscall count
      release(&child->lock);
    }
  }

  return total_count;
}
