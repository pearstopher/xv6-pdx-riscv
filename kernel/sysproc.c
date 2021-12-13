#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h" //lab 2
//#include "stdlib.h" //lab 2 - malloc the struct

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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

  if(argint(0, &pid) < 0)
    return -1;
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

//lab 2
uint64
sys_trace(void)
{
  int num;

  //get the first arg like this
  if (argint(0, &num) < 0)
    return -1;

  //add the syscall num to the newly creeated proc int
  struct proc *p = myproc();
  p->trace = num;

  //I dont think I need to zero out the trace at any point
  //the whole proc will dissapear when the procedure is done

  //return -1 on failure I think
  //and 0 or + on success?
  return 0; //for now

  //does this need to call trace?
  //return trace(num);

}

//lab 2
uint64
sys_sysinfo(void)
{
  //this is a struct pointer
  uint64 structptr;
  if (argaddr(0, &structptr) < 0)
    return -1;

  struct proc *p = myproc();


  //struct sysinfo * s = (struct sysinfo *) malloc(sizeof(struct sysinfo));
  struct sysinfo s; //copying out, I don't need to malloc it

  //copy in he amount of free memory
  //kernel/kalloc.c
  s.freemem = amt_mem_free();

  //copy in the number of processes
  //kernel/proc.c
  s.nproc = num_procs();

  //use copyout() to copy the struct back to user space
  if (copyout(p->pagetable, structptr, (char *)&s, sizeof(s)) < 0)
    return -1;

  return 0;

}
