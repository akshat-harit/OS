#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> //PID and fork functions
#include <errno.h>
#include <time.h>
#include <sys/time.h>


#include "shm.h"

main_comm   *main_comm_ptr;

int power(int n, int m){
if(m==0)
  return 1;

int result=1;
for(int i=0;i<m;i++){
  result=result*n;
}
return result;
}

int metric[]={100,75,50,25,5};
void  main(void)
{
     
     key_t shm_key;
     pid_t child_pid[NUMBER_OF_PROCESS];
     struct timeval tv1,tv2,tv3,tv4;
     unsigned long long timet;
     FILE* fp=fopen("results.txt", "w");
     FILE* timef=fopen("time.csv","w");
     int main_shm_id, result, sync;
     printf("%d\n", power(10,2));
    printf("\n=== Application launched! ===\n\nSelect the kind of blocking mechanism:\n1) Sync\n2) Async\nType (1/2): ");
    scanf("%d", &sync);

    for (int j=0; j<NUMBER_OF_PROCESS; j++) {
        
        child_pid[j]=fork();

        if (child_pid[j]==-1) {
             fprintf(stderr, "Failed to fork\n");
        } else if (child_pid[j]>0){
             //Parent process segment
             continue;
        } else {
            //Child process segment pid child_pid[j] should be 0
            pid_t pid = getpid();
            //Every process opens communication with the server and shared memory space
            main_comm_ptr = initialize_main_comm_client(pid);

            while(!main_comm_ptr->START);

            if(sync == 1) {
              
              int argument=j+rand()%20;
              
              
              //Code for running the QoS evaluation
              // for(int z=0;z<metric[j];z++){
              //   printf("Calling process %d\n", z);
              //   int argument=j+rand()%20;
              //   int returnFlag = call_service(main_comm_ptr, argument, &result, pid);}                
              // gettimeofday(&tv2,NULL);
              // timet=(tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
              // fprintf(timef,"\n%d,%d\n", metric[j],timet);
              
              gettimeofday(&tv1,NULL);
              int returnFlag = call_service(main_comm_ptr, argument, &result, pid);                 
              gettimeofday(&tv2,NULL);
              timet=(tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
              fprintf(timef, "%u,%d,%d,%lld\n",pid, argument, result,timet);
              fprintf(fp, "Result for pid: %u and arg %d: %d\n", pid, argument, result);
              printf("\n*** Result for pid: %u and arg %d: %d\n", pid, argument, result);
          } else {
              srand(time(NULL)+j);
              int r = ((rand() % 10) + 2);
              printf("\nSpawning %d processes for pid: %u\n",r, pid);
              //r=20;
              int h,p;
              for (int k=3; k<r+3; k=k+2){
                	printf("\nPid %u requesting with arg %d\n",pid, k);
                  gettimeofday(&tv1,NULL);
                  if( (h = call_service_async(main_comm_ptr, k, pid))==-1){
                      printf("\nServer busy");
                  }
                  gettimeofday(&tv3,NULL);
                  if((p = call_service_async(main_comm_ptr, k+1, pid))==-1) {
                      printf("\nServer busy");   
                  }
                  sleep(1);
                  if(h!=-1) {
                      int result1= return_result_async(main_comm_ptr, pid, k,h);
                      gettimeofday(&tv2,NULL);
                      fprintf(fp, "Result for pid: %u and arg %d: %d\n", pid, k, result1);
                      printf("\n*** Result for pid: %u and arg %d: %d\n", pid, k, result1);
                      timet=(tv2.tv_sec-tv1.tv_sec)*1000+(tv2.tv_usec-tv1.tv_usec);
                      fprintf(timef, "%u,%d,%d,%lld\n",pid, k, result1,timet);
                  }
                  if(p!=-1)  {
                      int result2= return_result_async(main_comm_ptr, pid, k+1,p);
                      gettimeofday(&tv4,NULL);

                      fprintf(fp, "Result for pid: %u and arg %d: %d\n", pid, k+1, result2);
                      printf("\n*** Result for pid: %u and arg %d: %d\n", pid, k+1, result2);
                      timet=(tv4.tv_sec-tv3.tv_sec)*1000+(tv4.tv_usec-tv3.tv_usec);
                      fprintf(timef, "%u,%d,%d,%lld\n",pid, k+1, result2,timet);
                  }
              }
          }
          fclose(fp);
          fclose(timef);
          close_main_comm_client(main_comm_ptr, pid);
          exit(1);
        }
    }
    return;
}