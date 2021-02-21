#ifndef QUEUE_H
#define QUEUE_H

#define QUEUESIZE 3
#define NUMCONSUMERS 4

//Define work item struct
typedef struct {
  void (*work)(void *args);                 //function to execute
  void *args;                               //arguments for function
}workFunction;


//Define Queue struct
typedef struct{
  workFunction *buf;                        //pointer to buffer
  long head,tail;                           //variables to point the head and tail of queue
  int full, empty;                          //binary variables to check if the queue is empty or full
  int prods_finish, prods_counter;          //variables which shows if producers have finished
  int  cons_finish, cons_counter;           //variables which shows if consumers have finished
  int missing_jobs;                         //total number of missing jobs
  pthread_mutex_t *mut;                     //pthread mutex
  pthread_cond_t *notFull, *notEmpty;       //pthread condition variables
}queue;


/***************************************QUEUE FUNCTIONS*******************************************/

//function to initialize queue
queue *queueInit(void);

//function to destroy queue when program ends
void queueDelete(queue *q);

//function to add new element to queue
void queueAdd(queue *q, workFunction item);

//function to delete an element from queue
void queueDel(queue *q, workFunction *out);

//function to reduce consumer threads
void queueReduceCons(queue *Q);

#endif
