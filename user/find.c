/*
Author: Christopher Juncker
Class:  CS333-003 Fall 2021
Lab:    2
*/
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


//modified version of fmtname() from user/ls.c
//returns name with no formatting
char*
name(char *path)
{
  char *p;

  //find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  //return name.
  return p;
}

//modified version of ls from user/ls.c
//comments and commented sections are mine
void
find(char *path, char *search)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:

    //print out the single file if it's name matches
    if (!strcmp(name(path), search)) {
      printf("%s\n", path);
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }

      //check for file match
      if (st.type == T_FILE && !strcmp(name(buf), search))
      {
        printf("%s\n", buf);
      }

      //check for directory (exclude . and ..)
      if (st.type == T_DIR && strcmp(name(buf), ".") && strcmp(name(buf), ".."))
      {
        //recursion into directories
        find(buf, search);
      }

    }
    break;
  }
  close(fd);
}


int
main(int argc, char *argv[])
{
  //case 0: invalid arguments
  if (argc < 3)
  {
    printf("usage: find {path} {filename}\n");
    exit(0);
  }

  //case 1: run the recursive find function
  find(argv[1], argv[2]);

  exit(0);
}
