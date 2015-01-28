#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#include  "shm.h"

int main_comm_smh_id;
extern main_comm* main_comm_ptr;

main_comm * initialize_main_comm_client(pid_t pid){
      
    key_t shm_key;
    main_comm *main_comm_ptr;
    shm_key = ftok(".", 0);

    main_comm_smh_id = shmget(shm_key, sizeof(main_comm), 0666);
    if (main_comm_smh_id < 0) {
          printf("** shmget error (client) ** with errno=%d and key =%d\n", errno, shm_key);
          exit(1);
    }

    main_comm_ptr = (main_comm *) shmat(main_comm_smh_id, NULL, 0);
    if ((unsigned long int) main_comm_ptr == -1) {
        printf("** shmat error (client) **\n");
        exit(1);
    }
    pthread_mutex_lock(&(main_comm_ptr->pids_mutex));
        for (int i=0; i<NUMBER_OF_PROCESS; i=((i+1)%NUMBER_OF_PROCESS)) {  
            if(main_comm_ptr->pids[i]==0){    
              main_comm_ptr->pids[i]=pid;
              break;
            }
        }
    pthread_mutex_unlock(&(main_comm_ptr->pids_mutex));
    //printf("Main comm channel opened in client...\n");
    return(main_comm_ptr);
}


int call_service(main_comm * main_comm_ptr, int arg, int * result, pid_t pid){
    //Process checks if slot at q_pos is free for use or not
    int result_val;
    int q_pos=call_service_async(main_comm_ptr, arg, pid);

    if(q_pos==-1){
      printf("\nServer busy");
      return -1;
    } 
    result_val = return_result_async(main_comm_ptr, pid, arg,q_pos);
    *result = result_val;
    return 1;
}

int return_result_async(main_comm * main_comm_ptr, pid_t pid, int arg, int q_pos){
    //Wait for the server to set the status as 2 (result ready)
    while(main_comm_ptr->ring[q_pos].status!=READYCLIENT);

    int result = main_comm_ptr->ring[q_pos].result;
    pthread_mutex_lock(&(main_comm_ptr->ring_mutex));
    main_comm_ptr->ring[q_pos].status = EMPTY;
    // printf("\n* Setting status for pid %u and arg %d to 0 at index %d\n", pid, arg, q_pos);
    pthread_mutex_unlock(&(main_comm_ptr->ring_mutex) );    

    return result;
}

int call_service_async(main_comm * main_comm_ptr, int arg, pid_t pid){
  int q_pos = 0;
  int busy_counter=0;

  start: 
  while(1){
      //find an empty spot in the queue
      if (main_comm_ptr->ring[q_pos].status == EMPTY) {
          break;
      } else {
          q_pos = (q_pos + 1) % RINGSIZE;
          busy_counter++;
          if(busy_counter==5000) 
            return -1;
      }
  }

  pthread_mutex_lock(&(main_comm_ptr->ring_mutex));
    //Confirm if you actually got the lock
    if (main_comm_ptr->ring[q_pos].status!=EMPTY){
      pthread_mutex_unlock(&(main_comm_ptr->ring_mutex));
      printf("\nUsing goto\n");
      goto start;
    }
    main_comm_ptr->ring[q_pos].pid      = pid;
    main_comm_ptr->ring[q_pos].argument = arg;
    main_comm_ptr->ring[q_pos].status   = READYSERVER;
  pthread_mutex_unlock(&(main_comm_ptr->ring_mutex));

  // printf("\nArg: %d, Pos: %d Pid:%u", arg, q_pos, pid);
  return q_pos;
}

void detach(int smhid){
  int result = shmdt(main_comm_ptr);
  if(result==-1){
    printf("\n**Failed to deallocate!**\n");
  }
}

void close_main_comm_client(main_comm* main_comm_ptr, pid_t pid){
    pthread_mutex_lock(&(main_comm_ptr->pids_mutex));
    for(int i=0; i<NUMBER_OF_PROCESS; i++)
     {
       if(main_comm_ptr->pids[i]==pid){
       	printf("\nProcess %u at position %d is now disconnecting\n", pid, i);
	     main_comm_ptr->pids[i]=0;
	     break;
      }
    }
    pthread_mutex_unlock(&(main_comm_ptr->pids_mutex));
    detach(main_comm_smh_id);
    exit(0);
}
