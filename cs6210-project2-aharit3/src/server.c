#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h> //Added

#include  "shm.h"

int pids_index;
int main_comm_smh_id;
main_comm* main_comm_ptr;


int fib(int n){
    if (n<0) {
        return -1;
    } else if (n<2) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

extern void deallocate(int smhid){
    int result = shmctl(smhid,IPC_RMID,NULL);
    if(result==-1){
    printf("\n**Failed to deallocate!**\n");
    }
}

void initialize_main_comm(){
    key_t shm_key;

    shm_key = ftok(".", 0);
    main_comm_smh_id = shmget(shm_key, sizeof(main_comm), IPC_CREAT|0666);
    // printf("Server key is %d\n", shm_key);
    if (main_comm_smh_id < 0) {
          printf("** Shmget error (server) **\n");
          exit(1);
    }

    main_comm_ptr = (main_comm *) shmat(main_comm_smh_id, NULL, 0);
    if ((unsigned long int) main_comm_ptr == -1) {
        printf("** Shmat error (server) **\n");
        exit(1);
    }
    printf("\n=== Server launched! ===\n\nMain communication channel opened in server.\nWaiting for clients...\n\n");
    main_comm_ptr->START = 1;
    
    // initialise mutexes to get it to work in shared memory
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&main_comm_ptr->ring_mutex, &attr);
    pthread_mutex_init(&main_comm_ptr->pids_mutex, &attr);
}


void close_main_comm(){
    printf("\n\nServer out!\n");
    deallocate(main_comm_smh_id);
}

void printRing(){
  for(int i=0; i<RINGSIZE; i++){
    printf("Ring %d\n", i);
    printf("PID %u\t", main_comm_ptr->ring[i].pid);
    printf("Argument %d\t", main_comm_ptr->ring[i].argument);
    printf("Result %d\t", main_comm_ptr->ring[i].result);
    printf("Status %d\t", main_comm_ptr->ring[i].status);
    printf("Last changing %d\t\n\n", main_comm_ptr->ring[i].lastarg);
  }
  printf("\nPIDs array:\n ");
  for(int i=0; i<NUMBER_OF_PROCESS; ++i){
  	printf("%u ", main_comm_ptr->pids[i]);
  }
}

void sigproc(){
  printf("\nCaught signal. Cleaning up and exiting.\n");
  printRing();
  close_main_comm();
  exit(1);
}

int schedule(){
	int counter;
	while(1){
		counter = 1000;
		pid_t pid_to_serve = main_comm_ptr->pids[pids_index];
		pids_index = (pids_index + 1) % NUMBER_OF_PROCESS;
		if (pid_to_serve == 0) {
			continue;
		}
		while(counter-- != 0){
			for (int i=0; i<RINGSIZE; ++i){
				if(main_comm_ptr->ring[i].pid == pid_to_serve && main_comm_ptr->ring[i].status==READYSERVER){
					printf("\nPicking up process %u at ring position %d", pid_to_serve, i);
					return i;
				}
			}
		}
	}
}

int main()
{
    int i=0, j;
    signal(SIGINT, sigproc);  //Added
    initialize_main_comm();
    while(1){
      int i = schedule();
      
      int arg = main_comm_ptr->ring[i].argument;
      printf(" with argument %d", arg);
      int result = fib(arg);

      //pthread_mutex_lock(&(main_comm_ptr->ring_mutex));
      main_comm_ptr->ring[i].result = result;
      main_comm_ptr->ring[i].status = READYCLIENT;
      //pthread_mutex_unlock(&(main_comm_ptr->ring_mutex) );
      
      // printRing();
      // Buffer issue solution
      printf("\n");
    }
    close_main_comm();
}