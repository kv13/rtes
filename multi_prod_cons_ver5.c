#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 5
#define LOOP 1000
#define PRODUCERS 1
#define CONSUMERS 36

// variables to store the timestamps.
int Counter = 0 ; 
long int  results_times[LOOP*PRODUCERS];

void *producer (void *args);
void *consumer (void *args);

//first define the structs
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

//all the functions 
queue *queueInit (void);
void  queueDelete (queue *q);
void  queueAdd (queue *q, workFunction *in);
void  queueDel (queue *q, workFunction *out);
void  queueReduceProducers(queue *q);
void  queueReduceConsumers(queue *q);
void  *my_function_1(void *arg) ; 
void  *my_function_2(void *arg) ;
double results();


int main ()
{

  //define the variables
  queue *fifo;
  int p=PRODUCERS;
  int q=CONSUMERS;
  int i;

  //define the threads and initialize the attribute
  pthread_t pro[p], con[q];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr , PTHREAD_CREATE_JOINABLE);

  //create the queue 
  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  //create  threads 
  for(i=0;i<p;i++){
    pthread_create (&pro[i], &attr, producer, fifo);
  }
  for(i=0;i<q;i++){
    pthread_create (&con[i], &attr, consumer, fifo);
  }

  //wait threads to finish and then continue the main program
  pthread_attr_destroy(&attr);
  for(i=0;i<p;i++){
     pthread_join (pro[i], NULL);
  }
  for(i=0;i<q;i++){
    pthread_join (con[i], NULL);
  }

  
  //the program has finished now I add this section  to 
  //save the timestamps, compute the mean and add it to 
  //a file called means.
  printf("COUNTER = %d \n",Counter);
  double mean;
  mean = results();
  printf("The mean is  %lf\n",mean);
  FILE *fp;
  fp =fopen("means.txt","a");
  if(fp == NULL ){
      printf("stderr, main: Cannot open the file means.\n");
      exit(EXIT_FAILURE);
    }
  fprintf(fp, "%lf\n", mean);
  fclose(fp);

  //delete queue 
  queueDelete (fifo);
  printf("0---------FINISHED---------0\n");
  return 0;
}

void *producer (void *q)
{
  queue *fifo=(queue *)q;
  int i;

  //a  producer thread tries to put to the queue LOOP elements
  //if queue is full the thread executes the pthread_cond_wait and wait a
  //consumer to delete an element and then add there a new element.When
  //all items are added the thread finishes.
  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }

    //define some variables.
    struct timeval tv;
    workFunction *item;
    long int *x;

    //initialize the pointers.
    x= (long int *)malloc(sizeof(long int));
    item = (workFunction *)malloc(sizeof(workFunction));

    //choose the function that the item will use.
    int r = rand()%20;
    if( r%2 == 0 ) item->work = &my_function_2;
    else item->work = &my_function_1;

    //compute the time that the element is added to the queue.
    gettimeofday(&tv,NULL);
    *x = tv.tv_usec+tv.tv_sec*1000000;
    item->arg = (void *) x;

    queueAdd (fifo,item);
    printf("producer: add another element to the queue \n");
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }

  //the producer has finished  and now return.
  //the mutex locks the queue because we need to reduce the number of producers
  pthread_mutex_lock(fifo->mut);
  queueReduceProducers(fifo);
  pthread_mutex_unlock(fifo->mut);
  pthread_cond_broadcast (fifo->notEmpty);
  return (NULL);
}

void *consumer (void *q)
{
  //define the variables that we will need.
  queue *fifo= (queue *)q;
  workFunction d;

  struct timeval tv_con;
  long int temp;
  void  *result;
  int i;

  //consumer runs until the queue is empty and there are no other producer
  //to add an element to the queue.
  while(1) {
    pthread_mutex_lock (fifo->mut);

    //if queue is empty but there is producers to add items then the consumer will wait 
    //for an item to be added
    while (fifo->empty && fifo->num_prods>0) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }

    //if the queue is not empty the consumer can delete an item
    if(fifo->empty!=1 ){

      //take the item and keep the time.
      queueDel (fifo, &d);
      gettimeofday(&tv_con , NULL);
      temp = tv_con.tv_sec*1000000 + tv_con.tv_usec;
      *(long int*)d.arg = temp - *(long int *)d.arg;
      pthread_mutex_unlock (fifo->mut);
      pthread_cond_signal (fifo->notFull);

      printf ("consumer: received %ld.\n", *(long int*)d.arg);
      //execution of the function.
      result = d.work(d.arg);
      if(result != NULL )printf("the cosine was computed to be %lf \n",*(double *)result);

      //store the value for statistics purposes
      results_times[Counter] = *(long int *)d.arg;
      Counter++;
    }
    //if the queue is empty and there are no producers to add items to queue then
    //the consumer has no point to continue working.
    else if(fifo->empty ==1 && fifo->num_prods == 0){
      break;
    }
  }
  pthread_mutex_unlock (fifo->mut);

  //the consumer has finished and so we reduce the number of consumers before exit.
  pthread_mutex_lock(fifo->mut);
  queueReduceConsumers(fifo);
  pthread_mutex_unlock(fifo->mut);
  return (NULL);
}



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


double results(){

  long int k ; 
  long int sum = 0;
  double mean;
  int i;

  FILE *fp;
  char str[30];

  //save the timestamps to a file because we will need to analyze them
  int x = sprintf(str,"resultsPROD=%dCONS=%d.txt",PRODUCERS,CONSUMERS);
  fp=fopen(str,"a");

  if(fp == NULL){
    printf( "Cannot open the file to store values.\n");
    exit(EXIT_FAILURE);
  }
  for(i=0;i<PRODUCERS*LOOP;i++){
    k = results_times[i];
    sum = sum + k ;
    fprintf(fp, "%ld\n", k);
  }
  fclose(fp);
  mean = (double)sum /(double)(PRODUCERS*LOOP);
  return mean;
}




//these are 2 simple functions
void * my_function_1(void *arg)
{
  long int k = *(long int *)arg;
  printf("this simple function just print a message with value = %ld \n",k);
  return NULL;
}

void * my_function_2(void *arg){
  double *result;
  result = (double *) malloc(sizeof(double));
  *result = cos( *(long int *)arg * 3.14 );
  return (void *)result;
}
