#define _GNU_SOURCE

#include "integral.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>


#define LOG_NUM "cat /proc/cpuinfo | egrep 'processor'| wc -l"
#define KERN_NUM "cat /proc/cpuinfo | egrep 'core id'| wc -l"

#define PHYS 	"cat /proc/cpuinfo | egrep -i 'physical id' | egrep -o *'[0-9]+'"
#define KERN 	"cat /proc/cpuinfo | egrep -i 'core id' | egrep -o *'[0-9]+'"
#define LOG 	"cat /proc/cpuinfo | egrep -i 'processor' | egrep -o *'[0-9]+'"
#define MHZ		"cat /proc/cpuinfo | egrep -i 'cpu MHz'| egrep -o '[0-9]+'.'[0-9]+'"

#define f(x) (1/(1 + (x)^2))

double F(double x)
{
	double res = 1000/(1 + x * x);
	return res;
}

int cmp_proc(const void * a, const void * b)
{
	if (((pLog)a)->phys > ((pLog)b)->phys) {
        return 1;
    } else if (((pLog)a)->phys < ((pLog)b)->phys) {
        return 0;
    } else if (((pLog)a)->kern > ((pLog)b)->kern) {
        return 1;
    } else if (((pLog)a)->kern < ((pLog)b)->kern) {
        return 0;
    } else if (((pLog)a)->log >= ((pLog)b)->log) {
        return 1;
    } else return 0;
}


int Log_num()
{

	FILE *num_file  = popen(LOG_NUM, "r");
	if(num_file == NULL)
	{
		printf("Error(Create_Multicore) : reading /proc/cpuinfo");	exit(0);
	}

	int log_num = 0;

	if(fscanf(num_file, "%d", &log_num) <= 0)
	{
		printf("Error(Create_Multicore) : getting log_num\n"); 	exit(0);
	}

	if(log_num <= 0)
	{
		printf("Problem with log_num\n"); exit(0);
	}

	return log_num;
}


int *Create_opt(int  logs)
{
	FILE *log_file = popen(LOG, "r");
	FILE *kern_file  = popen(KERN, "r");
	FILE *phys_file = popen(PHYS, "r");

	if(phys_file == NULL || kern_file == NULL || log_file == NULL)
	{
		printf("Error(Create_Multicore) : getting regular expressions\n"); 	exit(0);
	}

	int *opt = (int*)malloc(sizeof(int) * logs);
	if(opt == NULL)
	{
		printf("Error (Create_opt) : malloc opt\n");
		exit(0);
	}

	pLog h = (pLog)malloc(logs * sizeof(Log));
	if(h == NULL)
	{
		printf("Error (Create_opt) : malloc h\n");
		exit(0);
	}


	int i = 0;
	for(i = 0; i < logs; ++i)
	{
		int res_2 = fscanf(phys_file, "%d", &(h[i].phys));
		int res_1 = fscanf(kern_file, "%d", &(h[i].kern));
		int res_3 = fscanf(log_file,  "%d", &(h[i].log));

		//printf("proc : %d ; phys : %d; log : %d ; mhz : %lf \n", h[i].proc, h[i].phys, h[i].log, h[i].mhz);

		if(res_1 <= 0 || res_2 <= 0 || res_3 <= 0)
		{
			printf("Error(Create_opt) : reading values\n"); exit(0);
		}
	}

	//start sorting
	qsort(h, logs, sizeof(Log), &cmp_proc);

	i = 1;
	int cnt  = 0;
	int start = 0; int end = 0;
	while(i < logs && cnt != logs - 1)
	{
		while(i < logs && h[i].phys == h[i - 1].phys)
		{
			i++;
		}
		end = i - 1; 
		for(i = start; i <= end; i += 2)
		{
			opt[cnt] = h[i].log;
			cnt++;
		}
		for(i = start + 1; i <= end; i += 2)
		{
			opt[cnt] = h[i].log;
			cnt++;
		}

		i = end + 1;
		start = end + 1;
	}

	return opt;
}


void *start_task(void* arg)		//return pointer to result sum on [begin; end]
{
	Task t = *((pTask)arg);

	//set Task on its log_proc
	pthread_t thrd = pthread_self(); 
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(t.log_proc, &cpuset);

	if(pthread_setaffinity_np(thrd, sizeof(cpu_set_t), &cpuset) != 0)
	{
		printf("Error(start_task) : affinity\n"); exit(0);
	}

	register double dx = t.delta;

	register double x = t.begin;
	register double end = t.end;

	//calculate integral
	register double sum = 0;
	while(x < end)
	{
		sum += (F(x)) * dx;
		x += dx;
	}

	*(t.res) = sum;
	//printf("Task #%d; sum = %f\n", t.task_num, sum);
	//return (void*)(t.res);
}


double Main_Integral(int n, double begining, double ending, double delta_1)
{
	//printf("----- Started Main_Integral -----\n");

	int logs = Log_num();	//get number of kerns
	//printf("Number of logs = %d\n", logs);

	int *opt = Create_opt(logs);			//get optimal consequence to run

	int i = 0;
	/*for(i = 0; i < logs; ++i)
	{
		printf("%d ", opt[i]);
	}
	printf("\n");
	*/
	
	double begin = begining;
	double end = ending;
	double delta = delta_1;

	int iter = 0;
	if(n >= logs)
	{
		iter = n;
	}
	else
	{
		iter = logs;
	}
	
	pTask tasks = (pTask)malloc( iter* sizeof(Task));

	//storage for thread results
	double *res = (double*)malloc((128 * iter + 10) * sizeof(double)); 

	double shift = (end - begin)/n;
	//fill tasks

	int step = 0;

	for(i = 0; i < iter; ++i)
	{
		tasks[i].task_num = i;
		tasks[i].log_proc = opt[((i + 1) % logs)];
		tasks[i].res = &res[128 * i];

		tasks[i].begin = begin + shift * i;
		tasks[i].end = begin + shift * ((i + 1));
		tasks[i].delta = delta;
	}
	//printf("Passed creating tasks\n");

	//create storage for  pthreads
	pthread_t *Wires = (pthread_t*)malloc(iter * sizeof(pthread_t));

	char *b; double integral = 0;
	
	//run on pthreads
	for(i = 0; i < iter; ++i)
	{
		if(pthread_create(&(Wires[i]), NULL, &start_task, &(tasks[i])) != 0)
		{
			printf("Error(s : pthread_create\n"); 
			goto Error_free;
		}
	}

	//printf("Passed thread creation\n");
	
	for(i = 0; i < iter; ++i)
	{
		if(pthread_join(Wires[i], (void**)&b) != 0)
		{
			printf("Error(s : pthread_join on Wires[%d] = %lu\n", i, (unsigned long int)Wires[i]);
			printf("Error(s : pthread_create\n"); 
			goto Error_free;
		}
		//printf("Finished : %d\n;    res = %.3f\n", tasks[i].task_num, res[128 * i]);
	}
	
	for(i = 0; i < n; ++i)
	{
		integral += res[128 * i];
	}
	//printf("Passed joining\n");

	//printf("Final result : %.3f\n", integral);
	Error_free:
		free(res);
		free(Wires);
		free(opt);
		free(tasks);
	//printf("----- Ended Main_Integral -----\n");
	return integral;
}






