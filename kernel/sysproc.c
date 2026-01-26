#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
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
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  backtrace();
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
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

  if (argint(0, &pid) < 0)
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

// obtain the interval and the handler function address from user space
uint64 sys_sigalarm(void)
{
  int interval;
  uint64 handler;

  if (argint(0, &interval) < 0)
    return -1;
  if (argaddr(1, &handler) < 0)
    return -1;

  struct proc *p = myproc();

  p->alarm_interval = interval;
  p->alarm_handler = (void (*)(void))handler;
  p->alarm_ticks = 0;
  return 0;
}

/*
1. At point A in test() function, we trigger an interrupt and this is the one that needs to call the handler.
  -> trap into kernel, check the timer and if it is ready, set the user pc to the handler address.
2. Handler runs in user space. This means it overrides the registers at point A.
3. When handler finishes it calls sigreturn().
4. Here, we will need:
    a. Location of user VA to jump to, which is at address A. This is sepc in trapframe at Point A trap
    b. The 32 user registers that were originally at point A.
    c. Essentially on each timer interrupt, we need to save the 32 user registers because this could be the point where
    we hop to handler
*/
uint64 sys_sigreturn(void)
{
  struct proc *p = myproc();
  struct restore_from_timer rft = p->rft;
  p->guard = 0;
  // restore user registers
  p->trapframe->ra = rft.ra;
  p->trapframe->sp = rft.sp;
  p->trapframe->gp = rft.gp;
  p->trapframe->tp = rft.tp;
  p->trapframe->t0 = rft.t0;
  p->trapframe->t1 = rft.t1;
  p->trapframe->t2 = rft.t2;
  p->trapframe->s0 = rft.s0;
  p->trapframe->s1 = rft.s1;
  p->trapframe->a0 = rft.a0;
  p->trapframe->a1 = rft.a1;
  p->trapframe->a2 = rft.a2;
  p->trapframe->a3 = rft.a3;
  p->trapframe->a4 = rft.a4;
  p->trapframe->a5 = rft.a5;
  p->trapframe->a6 = rft.a6;
  p->trapframe->a7 = rft.a7;
  p->trapframe->s2 = rft.s2;
  p->trapframe->s3 = rft.s3;
  p->trapframe->s4 = rft.s4;
  p->trapframe->s5 = rft.s5;
  p->trapframe->s6 = rft.s6;
  p->trapframe->s7 = rft.s7;
  p->trapframe->s8 = rft.s8;
  p->trapframe->s9 = rft.s9;
  p->trapframe->s10 = rft.s10;
  p->trapframe->s11 = rft.s11;
  p->trapframe->epc = rft.return_address;

  return 0;
}
