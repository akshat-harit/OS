#ifndef _RVM_H_
#define _RVM_H_

#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

#include <cstring>
#include <cstdlib>
/*For making a new directory*/
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
/*LINE_MAX*/
#include <limits.h>
/*mmap*/
#include <sys/mman.h>

using namespace std;

typedef struct {
  char*     directory;
  char*     logfile;
} rvm_t;

typedef struct {
  char*     name;
  int       size;
  void*     data;
} segment_t;

typedef int trans_t;

typedef struct {
  int val;
  int size;
  void * old_data;
} offset_obj;

typedef struct segment_node{
    segment_t*       segment;
    trans_t          txn;
    std::vector<offset_obj> offset_list;
}segment_node;

// RVM Library functions
rvm_t rvm_init(const char* directory);
void* rvm_map(rvm_t rvm, const char* seg_name, int size_to_create);
void rvm_unmap(rvm_t rvm, void* seg_base);
void rvm_destroy(rvm_t rvm, const char* seg_name);
trans_t rvm_begin_trans(rvm_t rvm, int num_segs, void** seg_bases);
void rvm_about_to_modify(trans_t tid, void* seg_base, int offset, int size);
void rvm_commit_trans(trans_t tid);
void rvm_commit_trans_heavy(trans_t tid);
void rvm_abort_trans(trans_t tid);
void rvm_truncate_log(rvm_t rvm);

int file_dir_exist(const char*);
int create_file_dir(const char*, char*);
void check_segment_list(void);
char* get_seg_file_path(const char*);
int save_seg_file(segment_node , FILE*);
void read_seg_log(const char* seg_name, segment_t* seg);
void remove_seg_from_transaction(trans_t tid);
void rvm_verbose(int enable_flag);
#endif
