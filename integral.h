#ifndef INTEGRAL_H
#define INTEGRAL_H


typedef struct Logical_proc_info		// all info about Logical Processor
{
    int phys;	//index of physical proc
    int kern;	//index of kern
    int log;	//index of logical proc
} Log, *pLog;


typedef struct Task_info	// all info about Task for Log
{
	int task_num;	
	int log_proc;		// on which log proc to run
	double* res; //pointer to resulting sum on [begin; end]

	double begin;
	double end;
	double delta;

} Task, *pTask;



//int Input_handler(int argc, char ** argv);


int Log_num(); //number of logs
int Kern_num();  // numver of kerns

int *Create_opt(int logs);

void *start_task(void* arg);	//sets the task on thread

double Main_Integral(int n, double begining, double ending, double delta_1); //calc integral


#endif 