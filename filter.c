#include<math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <filter.h>
int filter(int n, int readfd, int writefd);

int filter(int n, int readfd, int writefd){
    int val;
    ssize_t res;
    while((res=read(readfd,&val,sizeof(int)))>0){
          if(val%n!=0){
	     if(write(writefd,&val,sizeof(int))==-1){
	        perror("write:");
		return 1;
	     }
	  }
    }
    if(res==-1){
       perror("read:");
       return 1;
    }
    return 0;
}
