#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


#define QUEUESIZE 100
#define NUMPRODUCERS 1
#define NUMCONSUMERS 10


void *producer(void *args);
void *consumer(void *args);

typedef struct {
  void (*work)(void *);
  void *args;
  struct timeval waiting_time;
}workFunction;


typedef struct{
  workFunction *buf;
  long head,tail;
  int full, empty;
  int prods_finish, prods_counter;
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
  void (*ErrorFcn)(void);
  void *userData;

  queue *Queue;
  pthread_t thread_id;
  void *(*producer)(void *arg);
}timer;


void timer_init(timer t, int period, int taskstoexecute, int startdelay,queue *q);
void start(timer t);
void startat(timer t, int year, int month, int day, int hour, int min, int sec);

int main(){
  return 0;
}


queue *queueInit(void){
	queue *q;
	q = (queue *)malloc(sizeof(queue));
	if (q==NULL) return NULL;
  q->buf = (workFunction *)malloc(sizeof(workFunction)*QUEUESIZE);
  if(q->buf == NULL) return NULL;
	q->prods_finish=0;
 	q->prods_counter=NUMPRODUCERS;
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
	return ;
}
