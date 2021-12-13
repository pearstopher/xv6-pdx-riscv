/*
Author: Christopher Juncker
Class:  CS333-003 Fall 2021
Lab:    1
*/
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAX 35


int
main(int argc, char *argv[])
{
  //create an array to hold some pairs of file descriptors
  //0 -> read
  //1 -> write
  int fd[MAX][2] = {0}; //MAX is definitely more than enough

  //make the first pipe
  pipe(fd[0]);
  
  //loop to fill the first pipe
  for (int i = 2; i <= 35; i++)
  {
    //feed a number into the pipe
    write(fd[0][1], &i, 4);
  }

  //close the write side of the first pipe
  close(fd[0][1]);

  //flag to check if a new pipe was created
  int newpipe = 1;

  //loop and process the most recent pipe
  for (int i = 1; newpipe == 1; ++i)
  {
    //reset flag
    newpipe = 0;

    //split and create a child
    if (fork() == 0)
    {
      //begin child
      int num = 0;
      int next = 0;

      //read the first number from left pipe
      read(fd[i - 1][0], &num, 4);

      //print the prime number
      printf("prime %d\n", num);

      //make a new pipe to push into
      pipe(fd[i]);

      //read remainder of left pipe
      while (read(fd[i-1][0], &next, 4)) {

        //check if current number is divisible by num
        if ((next % num) != 0)
        {
          //not divisible, push into the next pipe
          write(fd[i][1], &next, 4);

          //set the pipe flag
          newpipe = 1;
        }
        //else ignore the number it's not prime

      }

      //check if we used the pipe
      if (newpipe != 1)
      {
        //close the read end of the new pipe, nobody needs it
        close(fd[i][0]);
      }
    
      //always close the write end of the new pipe after loop
      close(fd[i][1]);
      //also close read end of the old pipe
      close(fd[i-1][0]);
    }

  }

  //wait until all children are terminated
  //wait returns -1 when no children are left
  int status = 0;
  while (wait(&status) > 0)
    ;

  //all processes exit here, parent and children
  exit(0);

}



