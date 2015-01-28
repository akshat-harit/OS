/* map_size - test that maps a larger size to check if a larger backing store is allocated */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_STRING1 "hello, world"
#define TEST_STRING2 "bleg!"
#define OFFSET2 1000


int main(int argc, char **argv)
{
     rvm_t rvm;
     char *seg;
     void *segs[1];
     trans_t trans;

     rvm_verbose(0);
     rvm = rvm_init("rvm_segments");
     printf("Destroying previous testseg backing stores\n");     
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);

     printf("After first map\n\n");
     system("ls -l rvm_segments");

     printf("\n===========\nMapped once with size 1000. Will now map with 2000 and display file size.\n");
     printf("Deliberately waiting so that you can read this!\n===========\n\n");
     sleep(2);

     seg = (char *)segs[0];
     rvm_unmap(rvm, seg);
     segs[0] = (char *) rvm_map(rvm, "testseg", 20000);
     seg = (char *)segs[0];

     printf("After second map\n\n");
     system("ls -l rvm_segments");     
     rvm_unmap(rvm, seg);
     printf("\n");
     
     exit(0);
}

