#include <pthread.h>
#include <stdlib.h>
#define NOT_READY  -1
#define FILLED     0
#define TAKEN      1
#define EMPTY 0
#define READYSERVER 1
#define READYCLIENT 2
#define RINGSIZE 10
#define NUMBER_OF_PROCESS 5 

typedef struct shmem_struct {
     int status;
     int argument;
     int result;  
     pid_t pid;   
     int lastarg;
}shmem_struct;

typedef struct main_comm {
    int START;                          //indicate server readiness
    int END;
    int ring_pointer;
    pthread_mutex_t ring_mutex;
    pthread_mutex_t pids_mutex;
    pid_t pids[NUMBER_OF_PROCESS];      //Array containing the pids of all threads currnetly connected to server
    shmem_struct ring[RINGSIZE];        //Ring DS that the client and server will use to exchange data 
}main_comm;


// typedef int (*handler)(main_comm*, pid_t, int);
extern void main_service();
extern main_comm* initialize_main_comm_client(pid_t);
extern int call_service(main_comm * main_comm_ptr, int arg, int * result, pid_t pid);
extern int call_service_async(main_comm * main_comm_ptr, int arg, pid_t pid);
extern void close_main_comm_client(main_comm * main_comm_ptr, pid_t pid);
extern int return_result_async(main_comm * main_comm_ptr, pid_t pid, int arg, int queue_pos);

