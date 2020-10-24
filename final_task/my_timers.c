//In this first implementation i will have only one timer. 
//Many timers will be implemented later.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//maybe useless includes
#include <unistd.h>
#include <sys/time.h>

#define QUEUESIZE 100
#define NUMCONSUMERS 100
#define NUMPRODUCERS 1


void *producer(void *args);
void *consumer(void *args);


typedef struct{
	int period;
	int taskstoexecute;
	int startdelay;
	int userdata; //maybe??
}timer_informations;

typedef struct{
	void (* TimerFcn)(void *args);
	void *userData;
} workFunction;

typedef struct{
	int Period;
	int TasksToExecute;
	int StartDelay;
	workFunction *function_struct;
	void (*StartFcn)(void *t,void *args);
	void (*StopFcn)(void *args);
	void (*ErrorFcn)(void);
}timer;

typedef struct{
	workFunction buf[QUEUESIZE];	
	long head,tail;
	int full,empty;
	int prods_finish, prods_counter;
	int  cons_finish, cons_counter;
	pthread_mutex_t *mut;
	pthread_cond_t *notFull, *notEmpty;
}queue;

queue *QUEUE;

void queueAdd(queue *q, workFunction item);
void queueDel(queue *q, workFunction *out);
void queueDelete(queue *q);
queue *queueInit(void);


void StartFunction(void *t,void *args);
//what's the functionality of the next two funcs??
void StopFunction();
void ErrorFunction();
//***********************************************
void TimerFunction(void *args);


int main(){
	//create timers characteristics.
	timer_informations t1;
	t1.period = 1;
	t1.taskstoexecute = 10;
	t1.startdelay = 100;

	//create and initialize the timer.	
	timer *timer_1;
	timer_1 = (timer *)malloc(sizeof(timer));
	timer_1->StartFcn = &StartFunction;
	timer_1->StartFcn((void *)timer_1,(void* )&t1);
	//printf("%d,%d,%d \n",timer_1->Period,timer_1->TasksToExecute,timer_1->StartDelay);			
	
	//create threads
	pthread_t prods[NUMPRODUCERS];
	pthread_t  cons[NUMCONSUMERS];
	//later i will add the attribute joinable etc.
	//start threads	
	pthread_create(&prods[0],NULL,producer,timer_1);
	int i;
	for(i=0;i<NUMCONSUMERS;i++){
		pthread_create(&cons[i],NULL,consumer,fifo)
	}
	
	//wait to finish the producer
	pthread_join(prod[0],NULL);
	//wait for consumers to finish
	for(i=0;i<NUMCONSUMERS;i++){
		pthread_join(cons[i],NULL);
	}
	return 0;
}


void StartFunction(void *t,void *args){

	timer_informations *temp_1;
	temp_1 = (timer_informations *)args;
	timer *temp;
	temp=(timer *)t;

	temp->Period = temp_1->period;
	temp->TasksToExecute = temp_1->taskstoexecute;
	temp->StartDelay = temp_1->startdelay;

	//after that startFunction should link the functions call
	temp->functions_struct = (workFunction *)malloc(sizeof(workFunction));
	temp->funcion_struct->TimerFcn = &TimerFunction;
	temp->function_struct->userData = (void *)malloc(sizeof(int));
	temp->function_struct->*userData = temp->userdata;
	
}	

void TimerFunction(void *args){
	int i,counter,sum;
	i = *(int *)args;
	sum = 0;
	printf("Inside the Timer Function \n");
	for(counter = 0;counter<i;counter ++){
		sum = sum + counter;
	}	
	printf("the total sum is %d \n",sum);
}


queue *queueInit(void){
	queue *q;
	q = (queue *)malloc(sizeof(queue));
	if (q==NULL) return NULL;
	q->prods_finish=0;
 	q->prods_counter=NUMPRODS;
  	q->cons_finish=0;
	q->cons_counter=NUMCONS;
  	q->head = 0;
  	q->tail = 0;
  	q->full = 0;
  	q->empty = 1;

  	q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  	q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  	q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

  	pthread_mutex_init(q->mut,NULL);
  	pthread_cond_init(q->notFull,NULL);
  	pthread_cond_init(q->notEmpty,NULL);

  	return q;
}


void queueDelete(queue *q){
	pthread_mutex_destroy(q->mut);
  	free(q->mut);
  	pthread_cond_destroy(q->notFull);
  	free(q->notFull);
  	pthread_cond_destroy(q->notEmpty);
  	free(q->notEmpty);
  	free(q);
}

void queueAdd(queue *q, workFunction i){
	q->buf[q->tail]= i;
  	q->tail++;
  	if(q->tail==QUEUESIZE){
    		q->tail=0;
  	}
  	if(q->tail == q->head){
    		q->full =1;
  	}
  	q->empty=0;
  	return ;
}
void queueDel(queue *q, workFunction *out){
	*out = q->buf[q->head];
  	q->head++;
  	if(q->head == QUEUESIZE){
    		q->head =0;
  	}
  	if(q->head == q->tail){
    		q->empty =1;
  	}
  	q->full = 0;
 	return ;

