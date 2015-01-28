/* A simple asynchronous XML-RPC client written in C, as an example of
   Xmlrpc-c asynchronous RPC facilities.  This is the same as the 
   simpler synchronous client xmlprc_sample_add_client.c, except that
   it adds 3 different pairs of numbers with the summation RPCs going on
   simultaneously.

   Use this with xmlrpc_sample_add_server.  Note that that server
   intentionally takes extra time to add 1 to anything, so you can see
   our 5+1 RPC finish after our 5+0 and 5+2 RPCs.
*/

#include <stdlib.h>
#include <stdio.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "config.h"  /* information about this build environment */

#define NAME "Xmlrpc-c Asynchronous Test Client"
#define VERSION "1.0"
#define ANY 1
#define MAJORITY 2
#define ALL 3

//int asam[10]={0};
//int index_on_call=0;

static void 
die_if_fault_occurred(xmlrpc_env * const envP) {
    if (envP->fault_occurred) {
        fprintf(stderr, "Something failed. %s (XML-RPC fault code %d)\n",
                envP->fault_string, envP->fault_code);
        exit(1);
    }
}


// static void handle_sample_add_response(const char *   const serverUrl,
//                            const char *   const methodName,
//                            xmlrpc_value * const paramArrayP,
//                            void *         const user_data,
//                            xmlrpc_env *   const faultP,
//                            xmlrpc_value * const resultP){

//   printf("Hoopla\n");
// }

static void 
handle_sample_add_response(const char *   const serverUrl,
                           const char *   const methodName,
                           xmlrpc_value * const paramArrayP,
                           void *         const user_data,
                           xmlrpc_env *   const faultP,
                           xmlrpc_value * const resultP) {
    
    xmlrpc_env env;
    xmlrpc_int addend=(xmlrpc_int32)5, adder=(xmlrpc_int32)6;
    
    /* Initialize our error environment variable */
    xmlrpc_env_init(&env);

    /* Our first four arguments provide helpful context.  Let's grab the
       addends from our parameter array. 
    */
    //printf("Before decompose\n");
    xmlrpc_decompose_value(&env, paramArrayP, "(ii)", &addend, &adder);
    
    die_if_fault_occurred(&env);

    printf("\nResult from %s. RPC with method '%s' at URL '%s' to add %d and %d "
           "has been returned\n", serverUrl, methodName, serverUrl, addend, adder);
    
    if (faultP->fault_occurred)
        printf("The RPC failed.  %s\n", faultP->fault_string);
    else {
        xmlrpc_int sum;
        //printf("Before read_int\n");
        xmlrpc_read_int(&env, resultP, &sum);
        //printf("After read_int\n");
        die_if_fault_occurred(&env);
        //asam[index_on_call++]=sum;
        printf("The sum is  %d\n", sum);
    }
}

int 
main(int           const argc, 
     const char ** const argv) {
    int i=0;
    char port[10];
    const char * const serverUrl = "http://localhost:";
    const char * const methodName = "sample.add";
    char urls[5][500];
    xmlrpc_env env;
    xmlrpc_client * clientP;
    xmlrpc_int adder;
    
    int semantic = 0;
    if (argc!= 3) {
        fprintf(stderr, "%s InitialPort Number_of_calls\n",argv[0]);
        exit(1);
    }
    int num_requests = atoi(argv[2]);
        if(num_requests>5){
        fprintf(stderr,"Only five servers at max supported\n");
        exit(1);
    }
    printf("\nPlease enter the option number of the desired semantic type for the MultiRPC:\n\n1.Any\n2.Majority\n3.All\n\nEnter: ");
    scanf("%d", &semantic);
    for(i=0;i<num_requests;i++){
        urls[i][0]='\0';
        strcat(urls[i],"http://localhost:");
        sprintf(port, "%d", atoi(argv[1])+i);
        strcat(urls[i], port);
        strcat(urls[i],"/RPC2");
    }

    /* Initialize our error environment variable */
    xmlrpc_env_init(&env);

    /* Required before any use of Xmlrpc-c client library: */
    xmlrpc_client_setup_global_const(&env);
    die_if_fault_occurred(&env);

    xmlrpc_client_create(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, NULL, 0,
                         &clientP);
    die_if_fault_occurred(&env);
    for (adder = 3; adder < 4; ++adder) {
        printf("\nMaking XMLRPC call to servers method '%s' "
               "to request the sum "
               "of 5 and %d...\n\n", methodName, adder);

        /* request the remote procedure call */
        xmlrpc_multi_client_start_rpcf(&env, clientP, semantic, num_requests, urls, methodName,
                                  &handle_sample_add_response , NULL,
                                  "(ii)", (xmlrpc_int32) 5 , adder);
        die_if_fault_occurred(&env);
    }
    
    printf("RPCs all requested.  Waiting for & handling responses...\n");

    /* Wait for all RPCs to be done.  With some transports, this is also
       what causes them to go.
    */
    
    xmlrpc_client_event_loop_finish(clientP);

    //~ printf("All RPCs finished.\n");

    xmlrpc_client_destroy(clientP);
    xmlrpc_client_teardown_global_const();
    
    pthread_exit(NULL);
    return 0;
}
