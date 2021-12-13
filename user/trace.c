#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];

  //1. there must be at least 3 args
  //2. the 2nd arg's 1st char must be numeric
  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  //system call trace()
  //returns negative on failure
  if (trace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
 
  //copy the relevant parts of argv to new array
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }

  //execute the command and exit
  exec(nargv[0], nargv);
  exit(0);
}
