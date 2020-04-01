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

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 50
#define LOOP 2000
#define PRODUCERS 1
#define CONSUMERS 96

void *producer (void *args);
void *consumer (void *args);

//first define the structs we need
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
void   store_values(void *arg);
double calculate_mean();


int main ()
{

  //define the variables
  queue *fifo;
  int p=PRODUCERS;
  int q=CONSUMERS;
  int i;

  //define the threads and initialize the atrribute
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
  //compute the mean and add it to a file called means.
  double mean;
  mean = calculate_mean();
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
  //if queue if full the thread executes the pthread_cond_wait and wait a
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

    //give them values.
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
  void  *result;
  queue *fifo= (queue *)q;
  int i;
  workFunction d;
  struct timeval tv_con;
  long int temp;
  //consumer runs until the queue is empty and there are no other producer
  //to add an element to the queue.
  while(1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty && fifo->num_prods>0) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }

    if(fifo->empty!=1 ){
      //take the item and keep the time...
      queueDel (fifo, &d);
      gettimeofday(&tv_con , NULL);
      temp = tv_con.tv_sec*1000000 + tv_con.tv_usec;
      *(long int*)d.arg = temp - *(long int *)d.arg;

    }

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);

    //the consumer executes the function with argument the waiting time
    //in the queue  
    printf ("consumer: received %ld.\n", *(long int*)d.arg);
    //execution
    result = d.work(d.arg);
    if(result != NULL )printf("the cosine was computed to be %lf \n",*(double *)result);

    //store the value for statistics purposes
    store_values(d.arg);

    if(fifo->num_prods==0 )break;
  }

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


void store_values(void *arg){
  long int k = *(long int *)arg;  
  int error;
  FILE *fp;
  char str[30];
  int x = sprintf(str,"resultsPROD=%dCONS=%d.txt",PRODUCERS,CONSUMERS);
  fp=fopen(str,"a");
  if(fp == NULL){
    printf( "Cannot open the file to store values.\n");
    exit(EXIT_FAILURE);
  }
  fprintf(fp, "%ld\n", k);
  fclose(fp);
}

double calculate_mean(){
  double mean=0;
  long int counter=0;
  long int sum=0;
  FILE *fp;
  char str[30];

  int x = sprintf(str,"resultsPROD=%dCONS=%d.txt",PRODUCERS,CONSUMERS);
  fp=fopen(str,"r");
  printf("NOW COMPUTE THE MEAN\n");
  char * line = NULL;
  ssize_t read;
  size_t len = 0;
  if(fp == NULL) exit(EXIT_FAILURE);
  while ((read = getline(&line, &len, fp)) != -1) {
    counter++;
    sum = sum + atoi(line);
  }
  mean = (double)sum / (double)counter;
  fclose(fp);
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
