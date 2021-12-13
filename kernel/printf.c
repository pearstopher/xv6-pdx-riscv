//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

volatile int panicked = 0;

// lock to avoid interleaving concurrent printf's.
static struct {
  struct spinlock lock;
  int locking;
} pr;

static char digits[] = "0123456789abcdef";

//lab 4
//void printf(char *fmt, ...)
//
//backtrace is defined in defs.h
void
backtrace()
{

  //r_fp() is defined in riscv.h
  uint64 s0 = r_fp();
  
  printf("backtrace:\n");

  //stack is one page
  //stack starts at top of page
  //return address is at fp - 8
  //saved frame ptr is at fp - 16
  //s0 = frame pointer/fp
  //use PGROUNDUP(fp) to compute bottom address
  //use PGROUNDDOWN(fp) to compute top address
  //it appears from example that the most recent
  //  call should be displayed last

  //for (; s0 > PGROUNDDOWN(s0); s0 = * (uint64 *) (s0 - 16))
  //both strategies (rouding up or down) give same result
  for (; s0 < PGROUNDUP(s0); s0 = * (uint64 *) (s0 - 16) )
  {
    //print the return address
    //getting " error: invalid type argument of unary '*' (have 'uint64') "
    //maybe I can cast it to uint64 and dereference
    //works but seems weirdly unnecessary
    printf("%p\n", * (uint64 *) (s0 - 8) );
    //regarding printf itself,
    //  %i is signed
    //  %u doesn't exist
    //  %p aaaaaah that's the stuff

    //find the next saved frame
    //s0 = *(s0 - 16); //first try
    //s0 = *((*s0) - 16); //second try
    //s0 = s0 - 16; //third try
    //do the same cast and dereference thing here (thanks error msg)
    //my first try was basically right see?
    //s0 = * (uint64 *) (s0 - 16); //fourth try is the charm
    //putting it in the loop tho
  }
}

static void
printint(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}

static void
printptr(uint64 x)
{
  int i;
  consputc('0');
  consputc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
printf(char *fmt, ...)
{
  va_list ap;
  int i, c, locking;
  char *s;

  locking = pr.locking;
  if(locking)
    acquire(&pr.lock);

  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&pr.lock);
}

void
panic(char *s)
{
  pr.locking = 0;
  printf("panic: ");
  printf(s);
  printf("\n");

  //lab 4
  //damn that's quite the for loop down there
  backtrace();



  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;

}

void
printfinit(void)
{
  initlock(&pr.lock, "pr");
  pr.locking = 1;
}
