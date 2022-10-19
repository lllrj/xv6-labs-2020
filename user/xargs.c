#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
#define GETL 1024
int main(int argc,char* argv[]){
    if(argc < 2){
        fprintf(2,"usage: xargs <command>\n");
        exit(1);
    }
    //char *argvs[];
    char *argvs[MAXARG];
    int index = 0;
    for(int i = 1; i < argc; i++){
        argvs[index++] = argv[i];
    }
    char buf;
    char* temp = malloc(sizeof(char)*GETL);
    int i = 0;
    while(read(0,&buf,sizeof(char))){
        //printf("buf = %d\n",buf=='\n');
        if(buf == '\n'|| buf == '\0'){
            temp[i] = 0;
            //printf("temp = %s\n",temp);
            argvs[index] = temp;
            if(fork() == 0){
                exec(argv[1],argvs);
            }else{
                wait(0);
                i = 0;
            }
        }else{
            //printf("i = %d\n",i);
            temp[i++] = buf;
        }
    }

    exit(0);
}