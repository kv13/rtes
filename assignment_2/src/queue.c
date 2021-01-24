#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "../inc/queue.h"
#include "../inc/timer.h"

//Queues constructor
queue *queueInit(void){

  //allocate memory for queue
	queue *q;
	q = (queue *)malloc(sizeof(queue));
	if (q==NULL) return NULL;

  //allocate memory for buffer
  q->buf = (workFunction *)malloc(sizeof(workFunction)*QUEUESIZE);
  if(q->buf == NULL) return NULL;


	q->prods_finish  = 0;
 	q->prods_counter = NUMTIMERS;
  q->cons_counter  = NUMCONSUMERS;
  q->cons_finish   = 0;
  q->head          = 0;
  q->tail          = 0;
  q->full          = 0;
  q->missing_jobs  = 0;
  q->empty         = 1;

  q->mut           = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  q->notFull       = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  q->notEmpty      = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  if(q->mut == NULL || q->notFull == NULL || q->notEmpty == NULL)return NULL;

  pthread_mutex_init(q->mut,    NULL);
  pthread_cond_init(q->notFull, NULL);
  pthread_cond_init(q->notEmpty,NULL);

  return q;
}


//function to destroy queue struct
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


//function to add a new element to Queue
void queueAdd(queue *q, workFunction item){

  //copy the new item inside buffer
  q->buf[q->tail].work=item.work;
  q->buf[q->tail].args = malloc(sizeof(struct timeval));
  *(struct timeval *)(q->buf[q->tail].args)=*(struct timeval *)item.args;

  q->tail++;

	if(q->tail==QUEUESIZE) q->tail=0;

  if(q->tail == q->head)q->full =1;

  q->empty=0;
}


//fuction to delete an item from queueAdd
void queueDel(queue *q, workFunction *out){
	*(struct timeval *)(out->args) = *(struct timeval *)(q->buf[q->head].args);
  out->work = q->buf[q->head].work;

  q->head++;

	if(q->head == QUEUESIZE) q->head = 0;

  if(q->head == q->tail)   q->empty = 1;

	q->full = 0;
}


//function to reduce consumers
void queueReduceCons(queue *Q){
	Q->cons_counter--;
  if(Q->cons_counter == 0)  Q->cons_finish = 1;
}
