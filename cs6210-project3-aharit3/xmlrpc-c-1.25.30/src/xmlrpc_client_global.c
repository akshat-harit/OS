
#define _GNU_SOURCE
#include <stdarg.h>

#include "xmlrpc_config.h"

#include "bool.h"

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include <xmlrpc-c/client_int.h>
#include <xmlrpc-c/client_global.h>
#include  <string.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h> 
/*=========================================================================
   Global Client
=========================================================================*/

static struct xmlrpc_client * globalClientP;
static bool globalClientExists = false;


void 
xmlrpc_client_init2(xmlrpc_env *                      const envP,
                    int                               const flags,
                    const char *                      const appname,
                    const char *                      const appversion,
                    const struct xmlrpc_clientparms * const clientparmsP,
                    unsigned int                      const parmSize) {
/*----------------------------------------------------------------------------
   This function is not thread-safe.
-----------------------------------------------------------------------------*/
    if (globalClientExists)
        xmlrpc_faultf(
            envP,
            "Xmlrpc-c global client instance has already been created "
            "(need to call xmlrpc_client_cleanup() before you can "
            "reinitialize).");
    else {
        /* The following call is not thread-safe */
        xmlrpc_client_setup_global_const(envP);
        if (!envP->fault_occurred) {
            xmlrpc_client_create(envP, flags, appname, appversion,
                                 clientparmsP, parmSize, &globalClientP);
            if (!envP->fault_occurred)
                globalClientExists = true;

            if (envP->fault_occurred)
                xmlrpc_client_teardown_global_const();
        }
    }
}



void
xmlrpc_client_init(int          const flags,
                   const char * const appname,
                   const char * const appversion) {
/*----------------------------------------------------------------------------
   This function is not thread-safe.
-----------------------------------------------------------------------------*/
    struct xmlrpc_clientparms clientparms;

    /* As our interface does not allow for failure, we just fail silently ! */
    
    xmlrpc_env env;
    xmlrpc_env_init(&env);

    clientparms.transport = NULL;

    /* The following call is not thread-safe */
    xmlrpc_client_init2(&env, flags,
                        appname, appversion,
                        &clientparms, XMLRPC_CPSIZE(transport));

    xmlrpc_env_clean(&env);
}



void 
xmlrpc_client_cleanup() {
/*----------------------------------------------------------------------------
   This function is not thread-safe
-----------------------------------------------------------------------------*/
    XMLRPC_ASSERT(globalClientExists);

    xmlrpc_client_destroy(globalClientP);

    globalClientExists = false;

    /* The following call is not thread-safe */
    xmlrpc_client_teardown_global_const();
}



static void
validateGlobalClientExists(xmlrpc_env * const envP) {

    if (!globalClientExists)
        xmlrpc_faultf(envP,
                      "Xmlrpc-c global client instance "
                      "has not been created "
                      "(need to call xmlrpc_client_init2()).");
}



void
xmlrpc_client_transport_call(
    xmlrpc_env *               const envP,
    void *                     const reserved ATTR_UNUSED, 
        /* for client handle */
    const xmlrpc_server_info * const serverP,
    xmlrpc_mem_block *         const callXmlP,
    xmlrpc_mem_block **        const respXmlPP) {

    validateGlobalClientExists(envP);
    if (!envP->fault_occurred)
        xmlrpc_client_transport_call2(envP, globalClientP, serverP,
                                      callXmlP, respXmlPP);
}



xmlrpc_value * 
xmlrpc_client_call(xmlrpc_env * const envP,
                   const char * const serverUrl,
                   const char * const methodName,
                   const char * const format,
                   ...) {

    xmlrpc_value * resultP;

    validateGlobalClientExists(envP);

    if (!envP->fault_occurred) {
        va_list args;

        va_start(args, format);
    
        xmlrpc_client_call2f_va(envP, globalClientP, serverUrl,
                                methodName, format, &resultP, args);

        va_end(args);
    }
    return resultP;
}



xmlrpc_value * 
xmlrpc_client_call_server(xmlrpc_env *               const envP,
                          const xmlrpc_server_info * const serverInfoP,
                          const char *               const methodName,
                          const char *               const format, 
                          ...) {

    xmlrpc_value * resultP;

    validateGlobalClientExists(envP);

    if (!envP->fault_occurred) {
        va_list args;

        va_start(args, format);

        xmlrpc_client_call_server2_va(envP, globalClientP, serverInfoP,
                                      methodName, format, args, &resultP);
        va_end(args);
    }
    return resultP;
}



xmlrpc_value *
xmlrpc_client_call_server_params(
    xmlrpc_env *               const envP,
    const xmlrpc_server_info * const serverInfoP,
    const char *               const methodName,
    xmlrpc_value *             const paramArrayP) {

    xmlrpc_value * resultP;

    validateGlobalClientExists(envP);

    if (!envP->fault_occurred)
        xmlrpc_client_call2(envP, globalClientP,
                            serverInfoP, methodName, paramArrayP,
                            &resultP);

    return resultP;
}



xmlrpc_value * 
xmlrpc_client_call_params(xmlrpc_env *   const envP,
                          const char *   const serverUrl,
                          const char *   const methodName,
                          xmlrpc_value * const paramArrayP) {

    xmlrpc_value * resultP;

    validateGlobalClientExists(envP);

    if (!envP->fault_occurred) {
        xmlrpc_server_info * serverInfoP;

        serverInfoP = xmlrpc_server_info_new(envP, serverUrl);
        
        if (!envP->fault_occurred) {
            xmlrpc_client_call2(envP, globalClientP,
                                serverInfoP, methodName, paramArrayP,
                                &resultP);
            
            xmlrpc_server_info_free(serverInfoP);
        }
    }
    return resultP;
}                            



void 
xmlrpc_client_call_server_asynch_params(
    xmlrpc_server_info * const serverInfoP,
    const char *         const methodName,
    xmlrpc_response_handler    responseHandler,
    void *               const userData,
    xmlrpc_value *       const paramArrayP) {

    xmlrpc_env env;

    xmlrpc_env_init(&env);

    validateGlobalClientExists(&env);

    if (!env.fault_occurred)
        xmlrpc_client_start_rpc(&env, globalClientP,
                                serverInfoP, methodName, paramArrayP,
                                responseHandler, userData);

    if (env.fault_occurred) {
        /* Unfortunately, we have no way to return an error and the
           regular callback for a failed RPC is designed to have the
           parameter array passed to it.  This was probably an oversight
           of the original asynch design, but now we have to be as
           backward compatible as possible, so we do this:
        */
        (*responseHandler)(serverInfoP->serverUrl,
                           methodName, paramArrayP, userData,
                           &env, NULL);
    }
    xmlrpc_env_clean(&env);
}



void 
xmlrpc_client_call_asynch(const char * const serverUrl,
                          const char * const methodName,
                          xmlrpc_response_handler responseHandler,
                          void *       const userData,
                          const char * const format,
                          ...) {

    xmlrpc_env env;

    xmlrpc_env_init(&env);

    validateGlobalClientExists(&env);

    if (!env.fault_occurred) {
        va_list args;

        va_start(args, format);
    
        xmlrpc_client_start_rpcf_va(&env, globalClientP,
                                    serverUrl, methodName,
                                    responseHandler, userData,
                                    format, args);

        va_end(args);
    }
    if (env.fault_occurred)
        (*responseHandler)(serverUrl, methodName, NULL, userData, &env, NULL);

    xmlrpc_env_clean(&env);
}



void
xmlrpc_client_call_asynch_params(const char *   const serverUrl,
                                 const char *   const methodName,
                                 xmlrpc_response_handler responseHandler,
                                 void *         const userData,
                                 xmlrpc_value * const paramArrayP) {
    xmlrpc_env env;
    xmlrpc_server_info * serverInfoP;

    xmlrpc_env_init(&env);

    serverInfoP = xmlrpc_server_info_new(&env, serverUrl);

    if (!env.fault_occurred) {
        xmlrpc_client_call_server_asynch_params(
            serverInfoP, methodName, responseHandler, userData, paramArrayP);
        
        xmlrpc_server_info_free(serverInfoP);
    }
    if (env.fault_occurred)
        (*responseHandler)(serverUrl, methodName, paramArrayP, userData,
                           &env, NULL);
    xmlrpc_env_clean(&env);
}



void 
xmlrpc_client_call_server_asynch(xmlrpc_server_info * const serverInfoP,
                                 const char *         const methodName,
                                 xmlrpc_response_handler    responseHandler,
                                 void *               const userData,
                                 const char *         const format,
                                 ...) {

    xmlrpc_env env;

    validateGlobalClientExists(&env);

    if (!env.fault_occurred) {
        va_list args;
    
        xmlrpc_env_init(&env);

        va_start(args, format);

        xmlrpc_client_start_rpcf_server_va(
            &env, globalClientP, serverInfoP, methodName,
            responseHandler, userData, format, args);

        va_end(args);
    }
    if (env.fault_occurred)
        (*responseHandler)(serverInfoP->serverUrl, methodName, NULL,
                           userData, &env, NULL);

    xmlrpc_env_clean(&env);
}



void 
xmlrpc_client_event_loop_finish_asynch(void) {

    XMLRPC_ASSERT(globalClientExists);
    xmlrpc_client_event_loop_finish(globalClientP);
}



void 
xmlrpc_client_event_loop_finish_asynch_timeout(
    unsigned long const milliseconds) {

    XMLRPC_ASSERT(globalClientExists);
    xmlrpc_client_event_loop_finish_timeout(globalClientP, milliseconds);
}
#define TEXT_LENGTH 500
typedef struct argument_struct_sync{
xmlrpc_env * envP;
xmlrpc_client * clientP;
char serverUrl[TEXT_LENGTH];
char methodName[TEXT_LENGTH];
char format[TEXT_LENGTH];
xmlrpc_value ** resultPP;
va_list args;
} argument_struct_sync;


void xmlrpc_client_call_2f_va_wrapper(void * arguments){
  argument_struct_sync arg;
  arg=*((argument_struct_sync*)arguments);
  printf("Request sent to URL: %s\n", arg.serverUrl);
  xmlrpc_client_call2f_va(arg.envP, arg.clientP, arg.serverUrl, arg.methodName,arg.format, arg.resultPP,arg.args);

}

xmlrpc_value * 
xmlrpc_client_call_multi(xmlrpc_env * const envP,
                    xmlrpc_client * const clientP,
                   const char type,
                   const int number,
                   char serverUrl[5][500],
                   const char * const methodName,
                   const char * const format,
                   ...) {
  int i=0,j=0;
  int array_thread[5]={0,0,0,0,0};
  argument_struct_sync arguments[number];
  XMLRPC_ASSERT_PTR_OK(format);
  xmlrpc_value * resultP[number];
  pthread_t threads[number];
  int rc;
  int counter=0;
    //validateGlobalClientExists(envP);
    if (!envP->fault_occurred) 
    {
        for(i=0; i< number;i++)
        {
          va_start(arguments[i].args, format);
          arguments[i].envP=envP;
          arguments[i].clientP=clientP;
          strcpy(arguments[i].serverUrl, serverUrl[i]);
          strcpy(arguments[i].methodName, methodName);
          strcpy(arguments[i].format, format);
          arguments[i].resultPP=&(resultP[i]);
          rc=pthread_create(&threads[i],NULL, &xmlrpc_client_call_2f_va_wrapper, (void *)&arguments[i]);
          if(rc!=0)
            printf("Error creating thread\n");
      }
      if(type=='a')
      {
        for(i=0;i<number;i=(i+1)%number)
        { 
          rc=pthread_tryjoin_np(threads[i],NULL);

          if(rc==0)
          { 
            counter++;
            for (j=0;j<number;j++) 
            { 
              va_end(arguments[j].args);
            }
            printf("Returning for any semantic\n");
            return *(arguments[i].resultPP);
          }
        }
      }
      else if(type=='m')
      {
        for(i=0;i<number;i=(i+1)%number)
        { //printf("In m semantic Counter %d\n", counter);
          if(array_thread[i]==0)
            rc=pthread_tryjoin_np(threads[i],NULL);
          else
            continue;
          if(rc==0)
          { array_thread[i]=1;
            counter++;
            if(counter>(number)/2.0)
            { for (j=0;j<number;j++) 
              { 
                //pthread_cancel(threads[j]);
                //printf("Blocked here\n");
                va_end(arguments[j].args);
              }
              printf("Returning for majority semantic with queries %d returned\n", counter);
              return *(arguments[i].resultPP); 
            }  
          }
        }
      }
      else if(type=='l')
      { for(i=0;i<number;i++)
        {
          rc=pthread_join(threads[i], NULL);
          if(rc!=0) 
            printf("Error\n");
          counter++;
        }
      }
      else
      {
        printf("Wrong code supplied\n");
        return NULL;
      }


  //Used only in case of all semantic  
   for (i=0;i<number;i++) 
   {
      va_end(arguments[i].args);
    }
      printf("Returning for all semantic\n");
      return *(arguments[i-1].resultPP); //Used only in case of all semantic
  }
  return NULL;    
}

