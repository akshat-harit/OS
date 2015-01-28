/* automatic_truncate.c - test that automatic truncate works*/

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "foo"
#define OFFSET 10

int main() 
{
     rvm_t rvm;
     trans_t trans;
     char* segs[0];
     
     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     
     int offset;
     rvm_verbose(1);
     
     for (int i=0; i<100; ++i){
          trans = rvm_begin_trans(rvm, 1, (void **) segs);
          // char buf[sizeof(int)*3+2];
          // snprintf(buf, sizeof buf, "%d", i);
          // printf("\n%s", buf);
          offset = OFFSET*i;
          rvm_about_to_modify(trans, segs[0], offset, 10);
          // sprintf(segs[0] + offset, "%s-%s",TEST_STRING, buf);
          sprintf(segs[0] + offset,TEST_STRING);
          rvm_commit_trans(trans);
     }

     printf("\nOK\n");
     // rvm_unmap(rvm, segs[0]);
     // rvm_destroy(rvm, "testseg");

     return 0;
}
