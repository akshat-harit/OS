/* A simple standalone XML-RPC server written in C. */

/* This server knows one RPC class (besides the system classes):
   "sample.add".

   The program takes one argument: the HTTP port number on which the server
   is to accept connections, in decimal.

   You can use the example program 'xmlrpc_sample_add_client' to send an RPC
   to this server.

   Example:

   $ ./xmlrpc_sample_add_server 8080&
   $ ./xmlrpc_sample_add_client

   For more fun, run client and server in separate terminals and turn on
   tracing for each:

   $ export XMLRPC_TRACE_XML=1
*/

#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/prctl.h>
#  include <signal.h>
#endif

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

#include "config.h"  /* information about this build environment */


#ifdef WIN32
  #define SLEEP(seconds) SleepEx(seconds * 1000, 1);
#else
  #define SLEEP(seconds) sleep(seconds);
#endif

//~ #define NUMBER_OF_SERVERS 2
int serverPort=0;
static xmlrpc_value *
sample_add(xmlrpc_env *   const envP,
           xmlrpc_value * const paramArrayP,
           void *         const serverInfo,
           void *         const channelInfo) {

    xmlrpc_int32 x, y, z;

    /* Parse our argument array. */
    xmlrpc_decompose_value(envP, paramArrayP, "(ii)", &x, &y);
    if (envP->fault_occurred)
        return NULL;

    /* Add our two numbers. */
    z = x + y;
    printf("Server Port Number : %d\n%d+%d=%d\n",serverPort,(int)x,(int)y,(int)z );
    /* Sometimes, make it look hard (so client can see what it's like
       to do an RPC that takes a while).
    */
    if (y == 1)
        SLEEP(3);

    /* Return our result. */
    return xmlrpc_build_value(envP, "i", z);
}

void sigproc(){
  printf("\nCaught signal. Cleaning up and exiting.\n");
  exit(1);
}

int 
main(int           const argc, 
     const char ** const argv) {

    struct xmlrpc_method_info3 const methodInfo = {
        /* .methodName     = */ "sample.add",
        /* .methodFunction = */ &sample_add,
    };
    xmlrpc_server_abyss_parms serverparm;
    xmlrpc_registry * registryP;
    xmlrpc_env env;

    //~ if (argc-1 != 1) {
        //~ fprintf(stderr, "You must specify 1 argument:  The TCP port "
                //~ "number on which the server will accept connections "
                //~ "for RPCs (8080 is a common choice).  "
                //~ "You specified %d arguments.\n",  argc-1);
        //~ exit(1);
    //~ }
    
    xmlrpc_env_init(&env);

    registryP = xmlrpc_registry_new(&env);

    xmlrpc_registry_add_method3(&env, registryP, &methodInfo);

    /* In the modern form of the Abyss API, we supply parameters in memory
       like a normal API.  We select the modern form by setting
       config_file_name to NULL: 
    */
    //~ serverparm.config_file_name = NULL;
    //~ serverparm.registryP        = registryP;
    //~ serverparm.port_number      = atoi(argv[1]);
    //~ serverparm.log_file_name    = "/tmp/xmlrpc_log";

    // printf("Running XML-RPC server...\n");

    // xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));

    int j;
    int server_count = atoi(argv[2]);
    pid_t child_pid[server_count];
    
    for (j=0; j<server_count; j++) {
        
        child_pid[j]=fork();

        if (child_pid[j]==-1) {
             fprintf(stderr, "Failed to fork\n");
        } else if (child_pid[j]>0){
             //Parent process segment
              signal(SIGINT, sigproc);
             continue;
        } else {
            //Child process segment pid child_pid[j] should be 0
            pid_t pid = getpid();
            prctl(PR_SET_PDEATHSIG, SIGHUP);

            serverparm.config_file_name = NULL;
            serverparm.registryP        = registryP;
            serverparm.port_number      = atoi(argv[1])+j;
            serverparm.log_file_name    = "/tmp/xmlrpc_log";
            serverPort=serverparm.port_number;
            printf("Running XML-RPC server on %d ...\n", serverparm.port_number);

            xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));
            exit(1);
        }
    }
    /* xmlrpc_server_abyss() never returns */
    while(1);
    return 0;
}
