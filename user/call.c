#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {

  unsigned int i = 0x00646c72;
  printf("H%x Wo%s\n\n", 57616, &i);

  //add something to the register
  register int res asm("a2") = 666;
  asm volatile("nop # just a comment, input picked %0" : : "r"(res));
  
  printf("x=%d y=%d\n\n", 3);
  
  printf("%d %d\n", f(8)+1, 13);
  exit(0);
}
