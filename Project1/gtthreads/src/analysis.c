#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#define LINES 256
#define MIL 1000000





int main(){

typedef struct results{
int elements;
double sum;
double mean;
double ts[8];
double tu[8];
double cpu[8];
double  stddev;

}res;
  res results_t[4][4];
  
  
 
  int i,j,k;
  double tot;

  FILE *fp;
  if(  ( fp=fopen("/home/akshat/Code/OS/gtthreads/myfile", "r") )==NULL)
  {printf("Error in file opening\n");
    exit(-1);
    
  } 
int size, credit,cred, mat_size;
double time_u, time_s;
double time_cpu;
 for (i=0;i<4;i++){
	for (j=0;j<4;j++){
	  mat_size=i;
	  cred=j;
	  results_t[mat_size][cred].elements=0;
	  results_t[mat_size][cred].sum=0;
	  results_t[mat_size][cred].stddev=0;
	  results_t[mat_size][cred].sum+=time_s*MIL+time_u;
	    
	}
   
}
char str[100];
while(fgets (str, 100, fp)!=NULL) /*just assuming number of entries here to demonstrate problem*/
    {
      long tot;
    sscanf(str,"%d,%d,%lf,%lf,%lf",&size,&credit,&time_s,&time_u,&time_cpu );   
   // fsgets(fp, "%d,%d,%l,%l,%f",&size, &credit,&time_u,&time_s,&time_cpu);
    //fprintf(stderr, "%d,%d,%lf,%lf,%lf\n",size, credit,time_s,time_u,time_cpu);

    switch(size){
	  case 32: mat_size=0; break;
	  case 64:mat_size=1; break;
	  case 128:mat_size=2; break;
	  case 256:mat_size=3; break;
      
      }
	switch(credit){
	case 25: cred=0; break;
	  case 50:cred=1; break;
	  case 75:cred=2; break;
	  case 100:cred=3; break;
	  
	}
	
	    results_t[mat_size][cred].ts[results_t[mat_size][cred].elements]=time_s;
	    results_t[mat_size][cred].tu[results_t[mat_size][cred].elements]=time_u;
	    results_t[mat_size][cred].cpu[results_t[mat_size][cred].elements]=time_cpu;
	    results_t[mat_size][cred].sum+=time_s*MIL+time_u;;
	    results_t[mat_size][cred].elements=results_t[mat_size][cred].elements+1;;
	  
    }
    
    
      for (i=0;i<4;i++){
	for (j=0;j<4;j++){
	  mat_size=i;
	  cred=j;
	  results_t[mat_size][cred].mean=results_t[mat_size][cred].sum/results_t[mat_size][cred].elements;
	  for(k=0;k<results_t[i][j].elements; k++){
	    results_t[mat_size][cred].stddev+=pow(results_t[mat_size][cred].mean-(results_t[mat_size][cred].ts[k]*MIL-results_t[mat_size][cred].tu[k]),2);
	  }
	  results_t[mat_size][cred].stddev=sqrt(results_t[mat_size][cred].stddev)/results_t[mat_size][cred].elements;
	
	  fprintf(stderr, "Matrix size=%d, Credits=%d\n",(mat_size+1)*32, (cred+1)*25);
	  fprintf(stderr, "Mean = %lf, Stddev=%lf\n", results_t[mat_size][cred].mean, results_t[mat_size][cred].stddev);
	  for (k=0; k< 8; k++){
	    tot=results_t[mat_size][cred].ts[k]*MIL+results_t[mat_size][cred].tu[k];
	    
	    fprintf(stderr, "Time for a thread = %lf\n",tot);
	  }
	  fprintf(stderr, "\n\n");
	}
	      
      
  
  
}

return 0;
}