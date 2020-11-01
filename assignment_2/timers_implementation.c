#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


#define QUEUESIZE 100
#define NUMCONSUMERS 100

typedef struct {
  void (*work)(void *);
  void *args;
}workFunction;


typedef struct{
  workFunction buf[QUEUESIZE];
  long head,tail;
  int full, empty;
  int prods_finish, prods_counter;
  int cons_finish, cons_counter;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
}queue;


queue *queueInit(void);
void queueDelete(queue q);
void queueAdd(queue *q, workFunction item);
void queueDel(queue *q, workFunction *out);
