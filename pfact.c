#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
void validate_arg(int arg_num,char **argv, int *n);
void close2(int fd);
void pipe2(int *fd_array);

//helper function: check if the user entered a valid argument;
void validate_arg(int arg_num,char **argv, int *n){
    //if entered more than 2 args
    if(arg_num!=2){
       fprintf(stderr, "Usage:\n\tpfact n\n");
       exit(-1);
    }
   
    *n=strtol(argv[1],NULL,10);
    
    //if did not enter ints;
    char arg_in_string[12];
    if(sprintf(arg_in_string,"%d",*n)<0){
       fprintf(stderr,"converting an int to a string fails\n");
       exit(-1);       
    }
    if(strcmp(argv[1],arg_in_string)!=0){
       fprintf(stderr, "Usage:\n\tpfact n\n");
       exit(-1);
    }
    
    //if entered ints <=1;
    if(*n<1){
       fprintf(stderr, "Usage:\n\tpfact n\n");
       exit(-1);
    }else if(*n==1){
       fprintf(stderr,"1 is neither a prime nor a composite, try another number\n");
       exit(-1);
    }
}

//helper function: close a filedescriptor;
void close2(int fd){
     if(close(fd)==-1){
        perror("close:");
	exit(-1);
     }
}

//helper function: pipe;
void pipe2(int *fd_array){
     if(pipe(fd_array)==-1){
        close2(fd_array[0]);
	close2(fd_array[1]);
        perror("pipe:");
        exit(-1);
     }
}

void sieveFilter(int fd_read, int fd_readFactor, int arg){
     int num_factor=0; //number of prime factors detected
     int reading_value;
     ssize_t reading_return;
     int filter;
     int factor[3];
     if((reading_return=read(fd_read,&reading_value,sizeof(int)))==-1){
        close2(fd_read);
	perror("read:");
	exit(-1);
     }/*else if(reading_return==0){
        printf("%d is not the product of two primes\n",arg);
	close2(fd_read);
	close2(fd_readFactor);
	exit(0);
     }*/
     
     //if there are numbers left in the fd_read, then we will get our first filter;
     filter=reading_value;
     //arg is a prime if filter==arg;
     if(filter==arg){
        printf("%d is prime\n", arg);
	exit(0);
     }
     //if filter is a factor of arg, then we store it into the array factor;
     if(arg%filter==0){
        num_factor++;
	factor[num_factor]=filter;
     }
     //store all the factors of arg found up to this point into array factor;
     int tempFactor;
     while((reading_return=read(fd_readFactor,&tempFactor,sizeof(int)))>0){
            num_factor++;
	    factor[num_factor]=tempFactor;
     }
     if(reading_return==-1){
        perror("read:");
	close2(fd_read);
	close2(fd_readFactor);
	exit(-1);
     }
     close2(fd_readFactor);
     if(filter*filter<arg){//filter < sqrt(arg);
        if(num_factor>=2){
	   printf("%d is not the product of two primes\n",arg);
	   close2(fd_read);
	   exit(0);
	}else{//num_factor=0 or 1, keep on filtering by recursion;
	   int fd[2];
	   pipe2(fd);
	   int fd_factor[2];
           pipe2(fd_factor);
	   int result=fork();
	   if(result<0){
	      close2(fd_read);
	      close2(fd[0]);
	      close2(fd[1]);
	      close2(fd_factor[0]);
	      close2(fd_factor[1]);
	      perror("fork:");
	      exit(-1);
	   }else if(result==0){//child
	      close2(fd[1]);
	      close2(fd_factor[1]);
	      sieveFilter(fd[0],fd_factor[0],arg);
	   }else{//parent
	      close2(fd[0]);
	      close2(fd_factor[0]);
	      while((reading_return=read(fd_read,&reading_value,sizeof(int)))>0){
	            if(reading_value%filter!=0){
		       if(write(fd[1],&reading_value,sizeof(int))==-1){
		          close2(fd_read);
			  close2(fd[1]);
			  close2(fd_factor[1]);
			  perror("write:");
			  exit(-1);
		       }
		    }
	      }
	      if(reading_return==-1){
	         perror("read:");
	         close2(fd_read);
	         close2(fd[1]);
		 close2(fd_factor[1]);
		 exit(-1);
	      }
	      close2(fd_read);
	      close2(fd[1]);
	      int j;
	      for(j=1;j<=num_factor;j++){
	          if(write(fd_factor[1],&factor[j],sizeof(int))==-1){
		     close2(fd_factor[1]);
	             perror("write:");
	             exit(-1);
		  }
	      }
	      close2(fd_factor[1]);
	    int filter_status;
	    if(wait(&filter_status)==-1){
	       perror("wait:");
	       exit(-1);
	    } 
	    if(WIFEXITED(filter_status)){
	       if(WEXITSTATUS(filter_status)!=-1){
	          exit(WEXITSTATUS(filter_status)+1);
	       }else{
	          fprintf(stderr,"child exited with -1\n");
		  exit(-1);
	       }
	    }else{
	       fprintf(stderr,"child did not exit normally\n");
	       exit(-1);
	    }
	  
	   } 
	}
     }else{// filter is already larger than or equal to sqrt(arg), thus we should not make filters anymore;
        if(filter*filter==arg){//check if filter is the square root of arg;
	   printf("%d %d %d\n", arg, filter, filter);
	   close2(fd_read);
	   exit(0);
	}else{// filter larger than sqrt(n);
           if(num_factor==0){
	      printf("%d is prime\n", arg);
	      close2(fd_read);
	      exit(0);
	   }else if(num_factor==2){//if arg already has 2 prime factors, then it can be factorized into 2 primes iff it is equal to the product of these 2 factors;
	      if(factor[1]*factor[2]==arg){
	         printf("%d %d %d\n", arg, factor[1], factor[2]);
	         close2(fd_read);
		 exit(0);
	      }else{
	         printf("%d is not the product of two primes\n",arg);
	         close2(fd_read);
		 exit(0);
	      }
	   }else{//num_factor=1, keep filtering;
	       if((arg/factor[1])!=factor[1]&&(arg/factor[1])%factor[1]==0){//this case is for numbers like 81=3**4, or 8=2**3, or 52=2**2*13...
	          printf("%d is not the product of two primes\n",arg);
	          close2(fd_read);
		  exit(0);
	       }
	       else{//match what is left in the original number list from 2 to arg with the factor we have found until their product is equal to arg;
	          if(filter*factor[1]==arg){
		     close2(fd_read);
		     //close2(fd_readFactor);
		     printf("%d %d %d\n", arg, filter, factor[1]);
	             exit(0);
		  }  
			 
	          while((reading_return=read(fd_read,&reading_value,sizeof(int)))>0){
	                
	                 if(reading_value*factor[1]==arg){
			    close2(fd_read);
			    printf("%d %d %d\n", arg, reading_value, factor[1]);
	                    exit(0);
			 }  
		 
		  }
	          if(reading_return==-1){
	             perror("read:");
	             close2(fd_read);
		     exit(-1);
	          }
		  close2(fd_read);
	          printf("%d is prime\n", arg);
	          exit(0); 
             }	   
	   }
                 
	}
     }
     
     
     
     
} 


int main(int argc, char **argv){
    //ignore the SIGPIPE signal;
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(-1);
    } 
    
    //validate arguments;
    int arg;
    validate_arg(argc,argv,&arg);
        
    //starts deciding if n can be factorized into 2 prime factors
    int number[2];
    pipe2(number);
    int factor[2];
    pipe2(factor);
    int result=fork();
    if(result<0){
       perror("fork:");
       close2(number[0]);
       close2(number[1]);
       close2(factor[0]);
       close2(factor[1]);
       exit(-1);
    }else if(result==0){//child process
       close2(number[1]);
       close2(factor[1]);
       sieveFilter(number[0], factor[0] , arg);
    }else{//parent process
       close2(number[0]);
       int i;
       for(i=2;i<=arg;i++){
           if(write(number[1],&i,sizeof(int))==-1){
	      perror("write:");
	      close2(number[1]);
	      close2(factor[0]);
              close2(factor[1]);
	      exit(-1);
	   }
       }
       close2(number[1]);
       close2(factor[0]);
       close2(factor[1]);
       int status;
       if(wait(&status)==-1){
          perror("wait:");
          exit(-1);
       }
       if(WIFEXITED(status)){
          if(WEXITSTATUS(status)!=-1){
             printf("Number of filters = %d\n",WEXITSTATUS(status));    
          }else{
	     fprintf(stderr,"child exited with -1\n");
	     exit(-1);
	  }      
       }else{
          fprintf(stderr,"child exited abnormally\n");
          exit(-1);
       } 
    }
    
    return 0;
}
