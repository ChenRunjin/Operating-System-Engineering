#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define MAXARG 1024

int main(int argc, char *argv[])
{

    char *param[MAXARG];
    char line[1024];
    char *cmd = argv[1];
    int index, n;
    for(index=0; index<argc-1; index++) param[index] = argv[index+1];
    index=argc-1;
    while((n=read(0, line, 1024))>0){
        char *arg = (char*)malloc(n+1);
        int i, j;
        j=0;
        for(i=0; i<n; i++){
            if(line[i] == ' ' || line[i] == '\n' || line[i] == 0){
                if(j>0){
                    arg[j]=0;
                    param[index] = arg;
                    index++;
                    j=0;
                    arg = (char*)malloc(n+1);
                }         
            }
            else{
                arg[j]=line[i];
                j++;
            }
        }
    }
    if(fork()==0){
        exec(cmd, param);
    }
    else{
        wait((int*)0);
    }
    
    exit(0);
}
