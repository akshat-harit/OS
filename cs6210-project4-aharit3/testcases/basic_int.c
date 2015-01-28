/* basic.c - test that basic persistency works */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "hello, world"
#define OFFSET2 1000


const int SIZE=3;
int array_test[SIZE]={1,2,3};
/* proc1 writes some data, commits it, then exits */
void proc1() 
{
     rvm_t rvm;
     trans_t trans;
     int* seg[1];
     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     seg[0] =(int *)rvm_map(rvm, "testseg", 100);
     if(seg[0]==NULL)
     {    printf("Error in setting up\n");
          exit(2);
     }
     trans = rvm_begin_trans(rvm, 1, (void **) seg);
     if(trans==-1){
          printf("error in transaction id setting\n");
          exit(2);  
     }
     rvm_about_to_modify(trans, seg[0], 0, 3*sizeof(int)+10);
     sprintf(((char *)seg[0]+3*sizeof(int)), "Hello");
     
     for(int i=0;i<SIZE;i++)
     {     *(seg[0]+i)=array_test[i];
          printf("Address: %d data: %d\n", (seg[0]+i),*(seg[0]+i));

     }
     // rvm_about_to_modify(trans, segs[0], OFFSET2, 100);
     // sprintf(segs[0]+OFFSET2, TEST_STRING);
     
     rvm_commit_trans(trans);

     abort();
}


/* proc2 opens the segments and reads from them */
void proc2() 
{
     int* seg[1];
     rvm_t rvm;
     rvm_verbose(0);
     rvm = rvm_init("rvm_segments");

     seg[0] = (int *) rvm_map(rvm, "testseg", 100);
     printf("Data\n\n");
     for(int i=0;i<SIZE;i++){
          printf("Address: %d data: %d\n",(seg[0]+i), *(seg[0]+i));
     }
     for(int i=0;i<SIZE;i++){
          if(*(seg[0]+i)!=array_test[i]) {
     	  printf("ERROR: numbers not present\n");
     	  exit(2);
          }
     }
     printf("OK\n");
     exit(0);
}


int main(int argc, char **argv)
{
     int pid;

     pid = fork();
     if(pid < 0) {
	  perror("fork");
	  exit(2);
     }
     if(pid == 0) {
	  proc1();
	  exit(0);
     }

     waitpid(pid, NULL, 0);

     proc2();

     return 0;
}
