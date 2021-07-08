#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

int main(int argc, char *argv[])
{

  int p1[2], p2[2];
  pipe(p1);
  pipe(p2);
  
  char buf[2];

  if(fork()==0){
    close(p1[WRITE]);
    read(p1[READ], buf, 1);
    fprintf(1, "%d: received ping\n", getpid());
    close(p1[READ]);
    close(p2[READ]);
    write(p2[WRITE], "p", 1);
    close(p2[WRITE]);
  }
  else{
    write(p1[WRITE], "p", 1);
    close(p1[READ]);
    close(p1[WRITE]);
    close(p2[WRITE]);
    read(p2[READ], buf, 1);
    fprintf(1, "%d: received pong\n", getpid());
    close(p2[READ]);
  }
  exit(0);
}
