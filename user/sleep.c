/*
Author: Christopher Juncker
Class:  CS333-003 Fall 2021
Lab:    1
*/
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  //case 0: invalid arguments
  if (argc < 2)
  {
    printf("usage: sleep {int}\n");
  }

  //case 1: call sleep system call
  else
  {
    //convert string to int
    int time = atoi(argv[1]);

    //system call defined in kernel/sysproc.c
    sleep(time);
  }

  exit(0);
}
