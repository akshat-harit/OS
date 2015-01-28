/* A simple synchronous XML-RPC client written in C, as an example of an
   Xmlrpc-c client.  This invokes the sample.add procedure that the Xmlrpc-c
   example xmlrpc_sample_add_server.c server provides.  I.e. it adds two
   numbers together, the hard way.

   This sends the RPC to the server running on the local system ("localhost"),
   HTTP Port 8080.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "config.h"  /* information about this build environment */

#define NAME "Xmlrpc-c Test Client"
#define VERSION "1.0"

static void 
dieIfFaultOccurred (xmlrpc_env * const envP) {
    if (envP->fault_occurred) {
        fprintf(stderr, "ERROR: %s (%d)\n",
                envP->fault_string, envP->fault_code);
        exit(1);
    }
}
#define TEXT_LENGTH 500


int 
main(int           const argc, 
     const char ** const argv) {

    xmlrpc_env env;
    xmlrpc_value * resultP, * resultP1;
    xmlrpc_int32 sum;
    char urls[5][TEXT_LENGTH];
    char port[10];
    int i=0, number_of_calls; 
    char type;
    xmlrpc_client * clientP;
    //const char * const serverUrl = "http://localhost:8080/RPC2";
    //const char * const serverUrl=s;
    const char * const methodName = "sample.add";

    if (argc!= 5) {
        fprintf(stderr, "%s Number_of_Servers InitialPort Arguments\n",argv[0]);
        exit(1);
    }

    number_of_calls=atoi(argv[1]);
    if(number_of_calls>5){
        fprintf(stderr,"Only five servers at max supported\n");
        exit(1);
    }
    printf("\nPlease enter the option number of the desired semantic type for the MultiRPC:\n\n1).Any\n2).Majority\n3).All\n\nEnter: ");
    scanf("%d", &type);
    if(type==1)
        type='a';
    else if(type==2)
        type='m';
    else if(type==3)
        type='l';
    else{
        printf("Wrong input: Accepted values are 1,2,3\n");
        exit(1);
    }

    /* Initialize our error-handling environment. */
    xmlrpc_env_init(&env);

    /* Start up our XML-RPC client library. */
    //xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, NULL, 0);
    //dieIfFaultOccurred(&env);
    xmlrpc_client_setup_global_const(&env);
    dieIfFaultOccurred(&env);
    xmlrpc_client_create(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, NULL, 0,
                         &clientP);
    dieIfFaultOccurred(&env);

    // printf("Making  XMLRPC call to server url '%s' method '%s' "
    //        "to request the sum "
    //        "of %s and %s ...\n", serverUrl, methodName, argv[2],argv[3]);

    /* Make the remote procedure call */
    for(i=0;i<number_of_calls;i++){
        urls[i][0]='\0';
        strcat(urls[i],"http://localhost:");
        sprintf(port, "%d", atoi(argv[2])+i);
        strcat(urls[i], port);
        strcat(urls[i],"/RPC2");
        //printf("%s\n",urls[i]);
    }
    //type=argv[5][0];
    // const char * urls[5] = {"http://localhost:8080/RPC2", 
    //                   "http://localhost:8081/RPC2", 
    //                   "http://localhost:8082/RPC2", 
    //                   "http://localhost:8083/RPC2", 
    //                   "http://localhost:8084/RPC2"};
    printf("\nMaking XMLRPC call to servers method '%s' "
               "to request the sum "
               "of %d and %d...\n\n", methodName,atoi(argv[3]),atoi(argv[4]) );
    resultP=xmlrpc_client_call_multi(&env, clientP, type, number_of_calls , urls, methodName, "(ii)", (xmlrpc_int32)(atoi(argv[3])), (xmlrpc_int32)(atoi(argv[4])) );
    //resultP = xmlrpc_client_call(&env, serverUrl, methodName,
    //                             "(ii)", (xmlrpc_int32)(atoi(argv[2])), (xmlrpc_int32)(atoi(argv[3])));
    dieIfFaultOccurred(&env);
    
    /* Get our sum and print it out. */
    xmlrpc_read_int(&env, resultP, &sum);
    dieIfFaultOccurred(&env);
    printf("The sum is  %d\n", sum);
    xmlrpc_DECREF(resultP);
    // resultP1=xmlrpc_client_call_multi(&env,clientP, 'a', number_of_calls , urls, methodName, "(ii)", (xmlrpc_int32)(atoi(argv[3])+1), (xmlrpc_int32)(atoi(argv[4])+1) );
    // printf("");
    // dieIfFaultOccurred(&env);
    // xmlrpc_read_int(&env, resultP1, &sum);
    // dieIfFaultOccurred(&env);
    // printf("The sum is  %d\n", sum);
    /* Dispose of our result value. */
    //xmlrpc_DECREF(resultP1);
    // //printf("String1\n");
    /* Clean up our error-handling environment. */
    
    xmlrpc_env_clean(&env);
    /* Shutdown our XML-RPC client library. */
    //xmlrpc_client_cleanup();
    exit(0);
    //pthread_exit(NULL);
    return 0;
}

