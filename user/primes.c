#include "kernel/types.h"
#include "user/user.h"

void reproject(int x,int p[]){
    close(x);
    dup(p[x]);
    close(p[0]);
    close(p[1]);
}

void prime(){
    int prm;
    if(read(0,&prm,sizeof(int)) == 0){
        //printf("will exit");
        exit(0);
    }
    printf("prime %d\n",prm);

    int p[2];
    pipe(p);
    int temp;
    while(read(0,&temp,sizeof(int))){
        if(temp % prm)
            write(p[1],&temp,sizeof(int));//每次传的temp值固定，有影响？  无影响
    }

    reproject(0,p);
    
    if(fork() == 0){
        prime();
    }else{
        // close(p[0]);
        // close(1);
        wait(0);
        exit(0);
    }
}

int main(void){
    int p[2];
    pipe(p);
    for(int i = 2; i <= 35; i++){
        write(p[1],&i,sizeof(int));
    }
    reproject(0,p);
    if(fork() == 0){
        prime();
    }else{
        close(0);
        wait(0);
    }
    exit(0);
}