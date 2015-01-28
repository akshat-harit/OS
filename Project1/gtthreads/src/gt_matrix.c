#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include<sys/resource.h>
#include<math.h>
#include "CPUGetTime.c"
#define LOGGING
#include "gt_include.h"


#define ROWS 512
#define COLS ROWS
#define SIZE COLS

#define NUM_CPUS 2
#define NUM_GROUPS NUM_CPUS
#define PER_GROUP_COLS (SIZE/NUM_GROUPS)

#define NUM_THREADS 128
#define PER_THREAD_ROWS (SIZE/NUM_THREADS)
#define NUM_OF_COMBINATIONS 16



/* A[SIZE][SIZE] X B[SIZE][SIZE] = C[SIZE][SIZE]
 * Let T(g, t) be thread 't' in group 'g'. 
 * T(g, t) is responsible for multiplication : 
 * A(rows)[(t-1)*SIZE -> (t*SIZE - 1)] X B(cols)[(g-1)*SIZE -> (g*SIZE - 1)] */


  
typedef struct matrix
{
	int **m;

	int rows;
	int cols;
	unsigned int reserved[2];
} matrix_t;


typedef struct __uthread_arg
{
	matrix_t *_A, *_B;
	unsigned int reserved0;

	unsigned int tid;
	unsigned int gid;
	int start_row; /* start_row -> (start_row + PER_THREAD_ROWS) */
	int start_col; /* start_col -> (start_col + PER_GROUP_COLS) */
	int credits;
	FILE *fp1;

  
}uthread_arg_t;
	
struct timeval tv1;

static void generate_matrix(matrix_t *mat, int val, int rows, int cols)
{	//printf("\nEntering generate_matrix in matrix\n");

	int i,j,k;
	mat->rows = rows;
	mat->cols = cols;
	
	  
	mat->m = (int**) malloc(rows*sizeof(int*));  
	for (k = 0;k < rows; k++)  
	    mat->m[k] = (int*) malloc(cols*sizeof(int)); 
	
	for(i = 0; i < mat->rows;i++)
		for( j = 0; j < mat->cols; j++ )
		{
			mat->m[i][j] = val;
		}
	return;
}

static void print_matrix(matrix_t *mat)
{	printf("\nEntering print_matrix in matrix\n");

	int i, j;
	printf("Rows = %d, Cols= %d\n", mat->rows, mat->cols);
	for(i=0;i<mat->rows;i++)
	{
		for(j=0;j<mat->cols;j++)
		  ;
			//printf(" %d ",mat->m[i][j]);
		//printf("\n");
	}

	return;
}

static void * uthread_mulmat(void *p)
{	//printf("\nEntering uthread_mulmat in matrix\n");
	int i, j, k;
	int start_row, end_row;
	int start_col, end_col;
	unsigned int cpuid;
	struct timeval tv2;
	struct timeval tv3;
	int w= RUSAGE_CHILDREN;
	struct rusage start;
	struct rusage *s=&start;
	struct rusage end;
	struct rusage *e=&end;
	double startTime, endTime;
	startTime = getCPUTime( );
		getrusage(w, s);
#define ptr ((uthread_arg_t *)p)

	i=0; j= 0; k=0;
	int mat_size;
	int cred;
	start_row = ptr->start_row;
	end_row = ptr->_A->rows;

	start_col = 0;
	end_col = ptr->_A->cols ;

	fprintf(stderr, "\nThread(id:%d, group:%d) started\n",ptr->tid, ptr->gid);

	for(i = start_row; i < end_row; i++)
		for(j = start_col; j < end_col; j++)
			for(k = 0; k < end_col; k++)
				ptr->_B->m[i][j] += ptr->_A->m[i][k] * ptr->_A->m[k][j];


	gettimeofday(&tv2,NULL);
	timersub(&tv2,&tv1,&tv3);
	//fprintf(ptr->fp1, "%d, %d, %d, %d\n",ptr->_A->cols,ptr->credits,tv3.tv_sec, tv3.tv_usec );
	//fprintf(stderr, "For matrix rows=%d and columns=%d and credits=%d",ptr->_A->rows,ptr->_A->cols,ptr->credits);
	//fprintf(stderr, "\nThread(id:%d, group:%d) finished (TIME : %lu s and %lu us)",
	//		ptr->tid, ptr->gid, tv3.tv_sec, tv3.tv_usec);

	endTime = getCPUTime( );
	getrusage(w, e);
	//printf("user time used: %16lf  %16lf\n",((long)e->ru_utime.tv_sec-(long)s->ru_utime.tv_sec),((long)e->ru_utime.tv_usec-(long)s->ru_utime.tv_usec));
	
	//printf("system time used: %16lf  %16lf\n", ((long)e->ru_stime.tv_sec-(long)s->ru_stime.tv_sec),((long)s->ru_stime.tv_usec-(long)e->ru_stime.tv_usec));
	//printf("Signals = %d\n", s->ru_nsignals);
	
		fprintf(ptr->fp1,"%d,%d,%d,%d,%f\n", ptr->_A->rows, ptr->credits,tv3.tv_sec,tv3.tv_usec,(endTime - startTime));
#if 0
	
#endif
	  
	//fprintf( stderr, "CPU time used = %lf\n", (endTime - startTime) );
#undef ptr
	return 0;
}

matrix_t A[128];
matrix_t B[128];

static void init_matrices()
{	int i,j,k;
	for(i=0;i<32;i++)
	{for(j=32;j<=256;j*=2)
	  {
	//  printf("Generating matrix %d with rows and columns = %d", k,j) ;
	  generate_matrix(A+k, 1,j,j);
	  generate_matrix(B+k, 0,j,j);
	  k++;
	  }
	  
	}
	return;
}


uthread_arg_t uargs[NUM_THREADS];
uthread_t utids[NUM_THREADS];
//FILE* fp1;

int main(int argc, char *argv[])
{		//fp1 = fopen ("file.txt", "w+"); 
	  
	  uthread_arg_t *uarg;
	int inx,i,j,k, credits=0;
	char scheduler;
	FILE *fp1 = fopen("myfile", "w");
	//fprintf(fp1,"Size,Credits,Time(sec),Time(usec),Time(CPU)\n");
	if (*argv[1]=='1')
	{
	  fprintf(stderr, "Credit Scheduler running\n");
	}
	else {
	  fprintf(stderr,"Wrong Argument\n: matrix number: 1 for Credit Scheduler");
	  exit(0);
	}
	scheduler=*argv[1];
	gtthread_app_init(scheduler);
	
	init_matrices();

	gettimeofday(&tv1,NULL);

	for(inx=0; inx<NUM_THREADS; inx++)
	{
		uarg = &uargs[inx];
		uarg->_A = A+inx;
		uarg->_B = (B+inx);

		uarg->tid = inx;

		uarg->gid = (inx % NUM_GROUPS);
		uarg->fp1=fp1;
		uarg->start_row = 0;
		if(inx<NUM_THREADS/4)
		  credits=25;
		else if (inx<NUM_THREADS/2)
		  credits=50;
		else if (inx<3*(NUM_THREADS/4))
		  credits=75;
		else
		  credits=100;
		    
		//fprintf(fp1,"Credits = %d, matrix=%d\n",credits, (A+inx)->cols);
		uarg->credits=credits;
		uthread_create(&utids[inx], uthread_mulmat, uarg, uarg->gid, credits, scheduler); //scheduler is redundant here
	}
      
	gtthread_app_exit();
#if 0	
	print_matrix(&A[1]);
	print_matrix(&B[1]);
	for(inx=0;inx<NUM_THREADS;inx++){
	 print_matrix(&(B[inx]));
	}
#endif

#if 0
	for(i=0; i<4; i++){
	  for(j=0;j<4;j++){
	    res[i][j].mean=res[i][j].sum/res[i][j].elements;
	    for(k=0; k<8; k++)
	      res[i][j].stddev=(res[i][j].mean-res[i][j].ts[i]*1000000-res[i][j].tu[i])*(res[i][j].mean-res[i][j].ts[i]*1000000-res[i][j].tu[i]);
	    res[i][j].stddev=sqrt(res[i][j].stddev)/res[i][j].elements;;
	    fprintf(stderr, "For thread with matrix = %d, credits= %d,\n total time=%d,\n avg time=%d,\n std dev=%d ", (i+1)*32,(j+1)*25, res[i][j].sum, res[i][j].mean, res[i][j].stddev);
	    for(k=0; k<8; k++)
	      fprintf(stderr, "\nThread %d had Run time %d\n CPU Time=%f",k,  res[i][j].ts[k]*1000000+res[i][j].tu[k], res[i][j].cpu[k]);
	  }
	  
	}
	
	for(i=0; i<4; i++){
	  for(j=0;j<4;j++){
	    fprintf(stderr,"For thread with matrix = %d, credits= %d,\n Elements=%d\n", (i+1)*32,(j+1)*25, res[i][j].elements);
	  }
	  
	}
#endif
	  //printf("\nExiting main in matrix\n");
	fclose(fp1);

	// print_matrix(&C);
	// fprintf(stderr, "********************************");
	return(0);
}
