#include "kernel/types.h"
#include "user/user.h"
int main(){
    char buf[8];
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    if(fork() == 0){
        write(p2[1],"pong",4);
        read(p1[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
    }else{
        write(p1[1],"ping",4);
        wait(0);
        read(p2[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
        
    }
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    exit(0);
}