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
  //create an pair of arrays to hold the pipes' file descriptors
  //fd[][0] -> read
  //fd[][1] -> write
  int fd[2][2];

  //system calls to create both pipes
  pipe(fd[0]);
  pipe(fd[1]);

  //write the ping byte to the first pipe
  char ping = '1';
  write(fd[0][1], &ping, 1);

  //close the write end of the first pipe
  close(fd[0][1]);

  //fork the program to create a parent/child
  if (fork() == 0) {
    //begin child

    //read ping from the first pipe
    char ping = '0';
    read(fd[0][0], &ping, 1);

    //verify that the ping checks out
    if (!strcmp(&ping, "1")) {

      //print required message 1
      printf("%d: received ping\n", getpid());

      //write the ping to the second pipe
      write(fd[1][1], &ping, 1);
    }

    //close the write end of the second pipe
    close(fd[1][1]);

    //close the read end of the first pipe
    close(fd[0][0]);

    //end child
    exit(0);
  }

  //wait for (a) child to complete
  int status = 0;
  wait(&status);

  //read from the second pipe
  char pong = '0';
  read(fd[1][0], &pong, 1);

  //verify that the pong checks out
  if (!strcmp(&pong, "1")) {

    //print required message 2
    printf("%d: received pong\n", getpid());
  }

  //close the read end of the second pipe
  close(fd[1][0]);

  //ping and pong should have been recieved
  //no pipes should be needlessly left open
  exit(0);
}
