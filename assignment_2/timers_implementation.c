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
void queueReduceCons(queue *Q);

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
void start(timer *t);
void startat(timer *t, int year, int month, int day, int hour, int min, int sec);
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
  timer *Timer;
  if(NUMTIMERS == 1){
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
    start(Timer);
    //startat(Timer,2020,11,2,22,0,0);

  }
  else if(NUMTIMERS == 3){
    Timer = (timer *)malloc(sizeof(timer)*3);
    if(Timer == NULL){
      fprintf(stderr,"ERROR:Cannot create Timer objects...===> EXITING\n");
      exit(1);
    }
    int period_0 = 1;
    int period_1 = 0.1;
    int period_2 = 0.01;
    int TasksToExecute_0 = 3600/period_0;
    int TasksToExecute_1 = 3600/period_1;
    int TasksToExecute_2 = 3600/period_2;
    timer_init(&Timer[0], period_0, TasksToExecute_0, 0, Queue);
    timer_init(&Timer[1], period_1, TasksToExecute_1, 0, Queue);
    timer_init(&Timer[2], period_2, TasksToExecute_2, 0, Queue);
    start(&Timer[2]);
    start(&Timer[1]);
    start(&Timer[0]);
    //startat(&Timer[0],2020,11,15,14,35,0);
    //startat(&Timer[1],2020,11,15,14,35,0);
    //startat(&Timer[2],2020,11,15,14,35,0);
  }
  if(NUMTIMERS == 1){
    pthread_join(Timer->thread_id,NULL);
  }
  else if(NUMTIMERS == 3){
    pthread_join(Timer[0].thread_id,NULL);
    pthread_join(Timer[1].thread_id,NULL);
    pthread_join(Timer[2].thread_id,NULL);
  }
  fprintf(stdout,"MAIN: PRODUCERS HAVE FINISHED\n");
  pthread_mutex_lock(Queue->mut);
  Queue->prods_finish = 1;
  pthread_mutex_unlock(Queue->mut);
  while(Queue->cons_finish == 0){
    pthread_mutex_lock(Queue->mut);
    queueReduceCons(Queue);
    pthread_mutex_unlock(Queue->mut);
    pthread_cond_signal(Queue->notEmpty);
  }
  for(int i = 0;i<NUMCONSUMERS;i++){
    pthread_join(consumers[i],NULL);
  }
  fprintf(stdout,"MAIN: CONSUMERS HAVE FINISHED\n ======> EXITING\n");
  return 0;
}


//   THREAD'S FUNCTIONS
void *producer(void *args){
  return NULL;
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


void start(timer *t){
  usleep(t->StartDelay*1000000);
  pthread_create(&t->thread_id,NULL,*t->producer,t->userData);
}


void startat(timer *t, int year, int month, int day, int hour, int min, int sec){
  time_t t_1 = time(NULL);
  struct tm tm = *localtime(&t_1);
  printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  //compute seconds from 01/01/1970
  long long int seconds_now = (tm.tm_year-70)*365*24*3600+tm.tm_mon*30*24*3600+tm.tm_mday*24*3600+tm.tm_hour*3600+tm.tm_min*60+tm.tm_sec;
  long long int seconds_fut = (year-1970)*365*24*3600+(month-1)*30*24*3600+day*24*3600+hour*3600+min*60+sec;
  long long int delay = seconds_fut-seconds_now;
  t->StartDelay = (int)delay;
  start(t);
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


void queueReduceCons(queue *Q){
	Q->cons_counter--;
	if(Q->cons_counter == 0){
		Q->cons_finish = 1;
	}
}
