#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


#define QUEUESIZE 100
#define NUMTIMERS 1
#define NUMCONSUMERS 10


void *producer(void *args);
void *consumer(void *args);


typedef struct {
  void (*work)(void *);
  void *args;
}workFunction;


typedef struct{
  workFunction *buf;
  long head,tail;
  int full, empty;
  int prods_finish, prods_counter;
  int  cons_finish, cons_counter;
  int missing_jobs;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
}queue;


queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, workFunction item);
void queueDel(queue *q, workFunction *out);


typedef struct{
  int Period;
  int TasksToExecute;
  int StartDelay;
  void (*StartFcn)(void *arg);
  void (*StopFcn)(void *arg);
  void (*TimerFcn)(void *arg);
  void (*ErrorFcn)(queue *q);
  void *userData;

  queue *Queue;
  pthread_t thread_id;
  void *(*producer)(void *arg);
}timer;


void timer_init(timer *t, int period, int taskstoexecute, int startdelay,queue *q);
void start(timer t);
void startat(timer t, int year, int month, int day, int hour, int min, int sec);
void stop_function();
void error_function(queue *q);
void my_function();


int main(){
  queue *Queue;
  Queue = queueInit();
  if(Queue == NULL){
    fprintf(stderr,"ERROR:Cannot create QUEUE...===> EXITING\n");
    exit(1);
  }
  //create consumers
  pthread_t consumers[NUMCONSUMERS];
  for(int j=0;j<NUMCONSUMERS;j++){
    pthread_create(&consumers[j],NULL,consumer,Queue);
  }

  //create timers
  if(NUMTIMERS == 1){
    timer *Timer;
    Timer = (timer *)malloc(sizeof(timer));
    if(Timer == NULL){
      fprintf(stderr,"ERROR:Cannot create the Timer object...===> EXITING\n");
      exit(1);
    }
    //periods in seconds
    int period = 1;
    //int period = 0.1;
    //int period = 0.01;
    int TasksToExecute = 3600/period;
    timer_init(Timer, period, TasksToExecute, 0, Queue);
  }
  else if(NUMTIMERS == 3){
    timer *Timers;
    Timers = (timer *)malloc(sizeof(timer)*3);
    if(Timers == NULL){
      fprintf(stderr,"ERROR:Cannot create Timer objects...===> EXITING\n");
      exit(1);
    }
    int period_0 = 1;
    int period_1 = 0.1;
    int period_2 = 0.01;
    int TasksToExecute_0 = 3600/period_0;
    int TasksToExecute_1 = 3600/period_1;
    int TasksToExecute_2 = 3600/period_2;
    timer_init(&Timers[0], period_0, TasksToExecute_0, 0, Queue);
    timer_init(&Timers[1], period_1, TasksToExecute_1, 0, Queue);
    timer_init(&Timers[2], period_2, TasksToExecute_2, 0, Queue);
  }
  return 0;
}


void *consumer(void *args){
  return NULL;
}


//TIMER'S FUNCTIONS


void timer_init(timer *t, int period, int taskstoexecute, int startdelay,queue *q){
  t->Period = period;
  t->TasksToExecute = taskstoexecute;
  t->StartDelay = 0;
  t->Queue = q;
  t->StartFcn = NULL;
  t->StopFcn = &stop_function;
  t->TimerFcn = &my_function;
  t->ErrorFcn = &error_function;
  t->userData = NULL;
}


void error_function(queue *q){
  q->missing_jobs++;
}


void stop_function(){
  fprintf(stdout,"TIMER STATUS: FINISHED\n");
}


void my_function(){
  fprintf(stdout,"For now just print a message \n");
}
// QUEUE'S FUNCTIONS


queue *queueInit(void){
	queue *q;
	q = (queue *)malloc(sizeof(queue));
	if (q==NULL) return NULL;
  q->buf = (workFunction *)malloc(sizeof(workFunction)*QUEUESIZE);
  if(q->buf == NULL) return NULL;
	q->prods_finish=0;
 	q->prods_counter=NUMTIMERS;
  q->head = 0;
  q->tail = 0;
  q->full = 0;
  q->missing_jobs = 0;
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
  free(q->buf);
  free(q);
}


void queueAdd(queue *q, workFunction i){
  //q->buf[q->tail].work = i.work;
  //*(int *)q->buf[q->tail].args = *(int *)i.args;
  q->buf[q->tail] = i;
  q->tail++;
	if(q->tail==QUEUESIZE){
  		q->tail=0;
	}
	if(q->tail == q->head){
  		q->full =1;
	}
	q->empty=0;
}


void queueDel(queue *q, workFunction *out){
	*out = q->buf[q->head];
  q->head++;
	if(q->head == QUEUESIZE){
		q->head = 0;
	}
	if(q->head == q->tail){
		q->empty = 1;
	}
	q->full = 0;
}
