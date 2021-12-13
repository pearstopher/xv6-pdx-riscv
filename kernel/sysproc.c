#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


//lab 4.1 test0
uint64
sys_sigalarm(void)
{

  //should store the alarm interval and the pointer to the handler function
  // in new fields in the proc structure in kernel/proc.h

  //get the process
  struct proc * p = myproc();

  //get the function arguments
  int ticks;
  uint64 handler;

  if (argint(0, &ticks) < 0)
    return -1;

  if (argaddr(1, &handler) < 0)
    return -1;

  //set the new proc fields
  p->alarm_interval = ticks;
  p->handler_ptr = handler;

  //if the user calls syscall(0, 0) then I think
  //the fields will reset themselves automatically

  //all done boiiiii
  return 0;
}

//lab 4.1 test0
uint64
sys_sigreturn(void)
{
  //lab 4.2 test1
  //this is a void syscall, no args to get

  //still need the process
  struct proc * p = myproc();

  //if this is called it means
  //that a user program has processed it's alarm
  //and needs the kernel to start it's program up again
  //well, earlier I saved trapframe->epc
  //so I wonder if I can just save the whole trapframe


  //1. need to restore saved registers
  //there must be a function that saves them all already
  //and a function to restore saved registers

  //now that trapframe is saved
  //can I just get it back
  //idk but this lab has been a breeze compared to pagetables so far
  memmove(p->trapframe, p->saved_trapframe, sizeof(*(p->saved_trapframe)));
  kfree(p->saved_trapframe);
  //this worked great once i finished temporarily forgetting
  // how pointers work



  //2. need to turn off the alarm
  p->ticks = 0;
  //p->alarm_interval = 0;
  //OHHHH don't reset the interval just the ticks
  //the user program will call sigalarm(0, 0) to turn it off
  //that's why I wasn't getting enough alarms!!!


  //3. need to prevent re-entrant calls to the handler (???)



  //4. need to return to the program's original location


  //I am getting this error, described in the user/alarmtest file:

    // the loop should have called foo() i times, and foo() should
    // have incremented j once per call, so j should equal i.
    // once possible source of errors is that the handler may
    // return somewhere other than where the timer interrupt
    // occurred; another is that that registers may not be
    // restored correctly, causing i or j or the address ofj
    // to get an incorrect value.
   




  //for now your sys_sigreturn should just return zero
  //lab 4.2 test1 - still return 0 for success
  return 0;
}

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

  
  //lab 4
  backtrace();


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
