/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.	
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */
//mean of times must be added 
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define QUEUESIZE 50
#define LOOP 400
#define PRODUCERS 300
#define CONSUMERS 1000

void *producer (void *args);
void *consumer (void *args);

typedef struct {
  void * (*work)(void *);
  void * arg;
}workFunction ;

typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  int num_prods,num_cons;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction *in);
void queueDel (queue *q, workFunction *out);
void queueReduceProducers(queue *q);
void queueReduceConsumers(queue *q);
void * my_function(void *arg) ; 

int main ()
{
  queue *fifo;
  int p=PRODUCERS;
  int q=CONSUMERS;
  int i;
  pthread_t pro[p], con[q];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr , PTHREAD_CREATE_JOINABLE);

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }
  /*pthread_create (&pro, NULL, producer, fifo);
  pthread_create (&con, NULL, consumer, fifo);
  pthread_join (pro, NULL);
  pthread_join (con, NULL);*/
  for(i=0;i<p;i++){
    pthread_create (&pro[i], &attr, producer, fifo);
  }
  for(i=0;i<q;i++){
    pthread_create (&con[i], &attr, consumer, fifo);
  }
  pthread_attr_destroy(&attr);
  for(i=0;i<p;i++){
     pthread_join (pro[i], NULL);
  }
  for(i=0;i<q;i++){
    pthread_join (con[i], NULL);
  }
  queueDelete (fifo);

  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    struct timeval tv;
    struct timezone tl;
    workFunction *item;
    long int *x;
    x= (long int *)malloc(sizeof(long int));
    item = (workFunction *)malloc(sizeof(workFunction));
    item->work = &my_function;
    printf("producer add %d \n",i);
    gettimeofday(&tv,NULL);
    *x = tv.tv_usec+tv.tv_sec*1000000;
    item->arg = (void *) x;
    queueAdd (fifo,item);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    //usleep (100000);
  }
 /* for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    queueAdd (fifo, i);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    usleep (200000);
  }*/
  pthread_mutex_lock(fifo->mut);
  queueReduceProducers(fifo);
  pthread_mutex_unlock(fifo->mut);
  pthread_cond_broadcast (fifo->notEmpty);
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  int i;
  workFunction d;

  fifo = (queue *)q;

  while(1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty && fifo->num_prods>0) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    if(fifo->empty!=1 )queueDel (fifo, &d);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    if(fifo->num_prods==0 )break;
    printf ("consumer: recieved %ld.\n", *(long int*)d.arg);
    pthread_mutex_lock(fifo -> mut);
    struct timeval tv_con;
    gettimeofday(&tv_con , NULL);
    long int temp;
    temp = tv_con.tv_sec*1000000 + tv_con.tv_usec;
    *(long int*)d.arg = temp - *(long int *)d.arg;
    d.work(d.arg);
    pthread_mutex_unlock(fifo -> mut);
    //usleep(200000);
  }
  /*for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, &d);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    printf ("consumer: recieved %d.\n", d);
    usleep (50000);
  }*/
  pthread_mutex_lock(fifo->mut);
  queueReduceConsumers(fifo);
  pthread_mutex_unlock(fifo->mut);
  return (NULL);
}

/*
  typedef struct {
  int buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->num_prods = PRODUCERS;
  q->num_cons  = CONSUMERS;
  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, workFunction *in)
{
  q->buf[q->tail] = *in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
void queueReduceProducers(queue *q){
  q->num_prods =  q->num_prods - 1;
  printf("THERE ARE NOW  %d producers \n",q->num_prods);
}
void queueReduceConsumers(queue *q){
  q->num_cons =  q->num_cons - 1;
  printf("THERE ARE NOW %d consumers \n",q->num_cons);
}


void * my_function(void *arg){
  long int k = *(long int *)arg;  
  int error;
  FILE *fp;
  printf("%ld\n",k);
  fp=fopen("/home/kostas/EMBEDDED/assignments/1/results.txt","a");
  fprintf(fp, "%ld\n", k);
  fclose(fp);
}
