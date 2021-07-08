#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

int main(int argc, char *argv[])
{

  int i, remain, p, fd[2], number[34];
  remain=34;
  for(i=0; i<34; i++){
    number[i] = i+2;
  }
  while(remain > 0){
    p=number[0];
    fprintf(1, "prime %d\n", p);
    remain--;
    if(remain > 0){
      pipe(fd);
      if(fork()==0){
        close(fd[WRITE]);
        remain=0;
        while(read(fd[READ], &number[remain], sizeof(int))){
          remain++;
        }
        close(fd[READ]);
      }
      else{
        close(fd[READ]);
        for(i=1; i<=remain; i++){
          if(number[i]%p != 0){
            write(fd[WRITE], &number[i], sizeof(int));
          }
        }
        close(fd[WRITE]);
        wait((int*)0);
      }
    }
  }
  exit(0);
}
