/*
Author: Christopher Juncker
Class:  CS333-003 Fall 2021
Lab:    1
*/
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h" //MAXARG


int
main(int argc, char *argv[])
{
  //reminder
  //standard input =  fd 0
  //standard output = fd 1
  //standard error =  fd 2


  //1. copy the arguments from argv

  //create new argc and argv
  int xargc = argc - 1;
  char * xargv[MAXARG];


  //copy argv to new array, skipping argv[0]
  for (int i = 1; i < argc; ++i)
  {
    //make some memory and copy the string
    xargv[i-1] = (char *) malloc(sizeof(argv[i]) + 1);
    strcpy(xargv[i-1], argv[i]);
  }


  //2. get the extra args from the pipe

  //read one character at a time
  //' ' =  separator between args
  //'\n' = end of argument list

  //the assignment example shows using echo to create
  //new lines but this does not seem to work in xv6

  //set up temporary vars for reading args
  char c;
  char temp_arg[128];
  int p = 0;

  //read() returns 0 when pipe is closed
  while(read(0, &c, 1))
  {
    //case 1: regular character
    //help, there is no strncmp and strcmp fails me
    if (memcmp(&c, " ", 1) && memcmp(&c, "\n", 1))
    {
      //add char to temporary arg
      strcpy(&temp_arg[p], &c);
      ++p;
    }
    //case 2: space or newline
    else
    {
      //add argument to the xargv array
      strcpy(&temp_arg[p], "\0"); //manual end of file?
      xargv[xargc] = malloc(sizeof(temp_arg) + 1);
      strcpy(xargv[xargc], temp_arg);
      ++xargc; //increment argc counter
      p = 0; //reset character marker
    }
    //case 3: newline
    if (!memcmp(&c, "\n", 1))
    { 
      //arguments are built, ready to call program!

      //create a child
      if(fork() == 0)
      {
        //run the program (didn't need xargc it turns out) 
        exec(xargv[0], xargv);
        //kill child
        exit(0);
      }
      //wait for child to complete
      int status = 0;
      wait(&status);


      //reset the counter so args can be rebuilt
      xargc = argc - 1;
    }
  }

  //this one was fun!
  exit(0);
}



