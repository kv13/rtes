#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "inc/queue.h"
#include "inc/timer.h"


//definition of functions
void *producer(void *args);
void *consumer(void *args);
void my_function(void *arg);

//global variables
int *Waiting_times;
int Waiting_counter=0;
pthread_mutex_t waiting_mutex;

int main(){

  struct timeval timer_start,timer_end;

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

  gettimeofday(&timer_start,NULL);

  if(NUMTIMERS == 1){

    Timer = (timer *)malloc(sizeof(timer));
    if(Timer == NULL){
      fprintf(stderr,"ERROR:Cannot create the Timer object...===> EXITING\n");
      exit(1);
    }

    //periods in useconds
    //int period = 1000000;
    //int period = 100000;
    int period = 10000;

    //int TasksToExecute = 3600*(1000000/period);
    int TasksToExecute = 100;

    //calculate waiting times in queue for every task
    Waiting_times = (int*)malloc(sizeof(int)*TasksToExecute);

    //iniatilize timer
    timer_init(Timer, period, TasksToExecute, 0, 0, Queue,&producer,&my_function);

    //start timer
    start(Timer);
    //startat(Timer,2021,1,23,15,39,0);
  }
  else if (NUMTIMERS == 3){

    Timer = (timer *)malloc(sizeof(timer)*3);
    if(Timer == NULL){
      fprintf(stderr,"ERROR:Cannot create Timer objects...===> EXITING\n");
      exit(1);
    }
    int period_0 = 1000000;
    int period_1 = 100000;
    int period_2 = 10000;

    //int TasksToExecute_0 = 3600*(1000000/period_0);
    //int TasksToExecute_1 = 3600*(1000000/period_1);
    //int TasksToExecute_2 = 3600*(1000000/period_2);

    int TasksToExecute_0 = 100;
    int TasksToExecute_1 = 1000;
    int TasksToExecute_2 = 1000;

    int TasksToExecute = TasksToExecute_0+TasksToExecute_1+TasksToExecute_2;

    //calculate waiting times in queue for every task
    Waiting_times = (int*)malloc(sizeof(int)*(TasksToExecute_0+TasksToExecute_1+TasksToExecute_2));

    //iniatilize timer
    timer_init(&Timer[0], period_0, TasksToExecute_0, 0, 0, Queue,&producer,&my_function);
    timer_init(&Timer[1], period_1, TasksToExecute_1, 0, 0, Queue,&producer,&my_function);
    timer_init(&Timer[2], period_2, TasksToExecute_2, 0, 0, Queue,&producer,&my_function);

    //start timers
    start(&Timer[2]);
    start(&Timer[1]);
    start(&Timer[0]);
    //startat(&Timer[0],2020,11,15,14,35,0);
    //startat(&Timer[1],2020,11,15,14,35,0);
    //startat(&Timer[2],2020,11,15,14,35,0);
  }

  //wait to finish all producers
  if(NUMTIMERS == 1){
    pthread_join(Timer->thread_id,NULL);
  }
  else if(NUMTIMERS == 3){
    pthread_join(Timer[0].thread_id,NULL);
    pthread_join(Timer[1].thread_id,NULL);
    pthread_join(Timer[2].thread_id,NULL);
  }

  //signal consumers to finish
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
  printf("Execution ends\n");
  gettimeofday(&timer_end,NULL);
  printf("total time %ld seconds, %ld miliseconds\n",timer_end.tv_sec-timer_start.tv_sec,(timer_end.tv_usec-timer_start.tv_usec)/1000);
  printf("total jobs missed:%d\n",Queue->missing_jobs);

  //write total waiting times to file
  char title[150];
  snprintf(title,sizeof(title),"results/Waiting_Times.txt");
  writeFile(title,Waiting_times,Waiting_counter);

  //free memory and exit
  queueDelete(Queue);
  free(Waiting_times);
  free(Timer);
  return 0;
}


//THREAD FUNCTIONS
//consumer function
void *consumer(void *args){

  //initialize queue
  queue *Queue;
  Queue = (queue *)args;

  //initialize time variables
  int *waiting_time = (int *)malloc(sizeof(int));
  struct timeval temp;

  //initialize workFunction item
  workFunction *out;
  out = (workFunction *)malloc(sizeof(workFunction));
  out->args = malloc(sizeof(struct timeval));
  if(out == NULL || out->args == NULL){
    fprintf(stderr,"ERROR: cannot allocate memory for workFunction inside the producer... EXITING \n");
    exit(1);
  }
  for(;;){

    //lock the mutex
    pthread_mutex_lock(Queue->mut);

    //if queue is empty wait
    while(Queue->empty==1 && Queue->prods_finish!=1){
      pthread_cond_wait(Queue->notEmpty,Queue->mut);
    }

    //if producers have finished must break from the loop
    if(Queue->prods_finish == 1){
      pthread_mutex_unlock(Queue->mut);
      break;
    }

    //otherwise delete an element from the queue
    queueDel(Queue,out);
    gettimeofday(&temp,NULL);
    pthread_mutex_unlock(Queue->mut);

    //measure the time that the element stayed in the queue
    *waiting_time  = (int)(temp.tv_sec  - ((struct timeval *)(out->args))->tv_sec)*1000000;
    *waiting_time += (int)(temp.tv_usec - ((struct timeval *)(out->args))->tv_usec);

    out->work((void *)waiting_time);
  }

  //when exit from the loop must clean the rest element
  for(;;){
    pthread_mutex_lock(Queue->mut);

    if(Queue->empty){
      pthread_mutex_unlock(Queue->mut);
      break;
    }
    queueDel(Queue,out);
    gettimeofday(&temp,NULL);
    pthread_mutex_unlock(Queue->mut);

    //measure the time that the element stayed in the queue
    *waiting_time  = (int)(temp.tv_sec- ((struct timeval *)(out->args))->tv_sec)*1000000;
    *waiting_time += (int)(temp.tv_usec- ((struct timeval *)(out->args))->tv_usec);

    out->work((void *)waiting_time);
  }

  free(out);
  free(waiting_time);
}


//producer function
void *producer(void *args){

  //declare time variables
  struct timeval prod_time_1,prod_time_2,prod_time_previous,prod_time_producer;

  int delay_sec,delay_usec;
  int *sleep_times;                              //sleep times
  int *execution_times;                          //times between two executions. Must be close to Period
  int *producer_times;                           //times for the producer to put the items in the queue

  //timer object
  timer *t;
  t = (timer *)args;
  int    usec_to_sleep=t->Period;

  //initialize time arrays
  sleep_times     = (int *)malloc(sizeof(int)*t->TasksToExecute);
  producer_times   = (int *)malloc(sizeof(int)*t->TasksToExecute);
  execution_times = (int *)malloc(sizeof(int)*t->TasksToExecute);

  if(sleep_times == NULL || execution_times == NULL ){
    fprintf(stderr,"ERROR: Cannot allocate memory for array...===>EXITING\n");
    exit(1);
  }

  workFunction *w;
  w = (workFunction *)malloc(sizeof(workFunction));
  w->args = malloc(sizeof(struct timeval ));
  if(w == NULL || w->args == NULL){
    fprintf(stderr,"ERROR: cannot allocate memory for workFunction inside the producer... EXITING \n");
    exit(1);
  }
  w->work = t->TimerFcn;

  //initial delay
  sleep(t->StartDelay);

  for(int i=0;i<t->TasksToExecute;i++){

    //start timer to measuer time take producer to put in the queue
    gettimeofday(&prod_time_producer,NULL);

    //try to put to queue an element=>lock the mutex
    pthread_mutex_lock(t->Queue->mut);
    gettimeofday(&prod_time_1,NULL);

    //if queue isn't full add the element
    if(t->Queue->full == 0){
      gettimeofday((struct timeval *)(w->args),NULL);
      queueAdd(t->Queue,*w);

      //unlock the mutex and signal that queue is not empty to wake up consumers
      pthread_mutex_unlock(t->Queue->mut);
      pthread_cond_signal(t->Queue->notEmpty);
    }
    //if queue is full drop the data
    else{
      t->ErrorFcn(t->Queue);
      pthread_mutex_unlock(t->Queue->mut);
    }

    gettimeofday(&prod_time_2,NULL);

    if(i==0){

      sleep_times[0]= t->Period;
      execution_times[0]=0;
      producer_times[0] = 1000000*(prod_time_2.tv_sec - prod_time_producer.tv_sec);
      producer_times[0] = producer_times[0] + prod_time_2.tv_usec - prod_time_producer.tv_usec;

      usleep(t->Period);
    }
    else{
      //find execution time between consequtives tasks
      delay_sec          = prod_time_1.tv_sec - prod_time_previous.tv_sec;
      delay_usec         = prod_time_1.tv_usec-prod_time_previous.tv_usec;
      execution_times[i] = delay_sec*1000000+delay_usec;

      //find total time to sleep.
      if(sleep_times[i-1]!=0)usec_to_sleep = t->Period+(t->Period - execution_times[i]);
      else usec_to_sleep = t->Period;
      
      printf("thread:%ld, delay_sec: %d,delay_usec: %d, usec_to_sleep: %d\n",t->thread_id,delay_sec,delay_usec,usec_to_sleep);

      //find producer time
      producer_times[i] = 1000000*(prod_time_2.tv_sec - prod_time_producer.tv_sec);
      producer_times[i] = producer_times[i] + prod_time_2.tv_usec - prod_time_producer.tv_usec;

      if (usec_to_sleep>0){
        usleep(usec_to_sleep);
        sleep_times[i]=usec_to_sleep;
      }
      else{
        sleep_times[i]=0;
      }
    }
    prod_time_previous = prod_time_1;
  }
  //WRITE TIMES TO FILES.
  char title[150];
  snprintf(title,sizeof(title),"results/Sleep_Times_Period=%d",t->Period);
  writeFile(title,sleep_times,t->TasksToExecute);
  snprintf(title,sizeof(title),"results/Execution_Times_Period=%d",t->Period);
  writeFile(title,execution_times,t->TasksToExecute);
  snprintf(title,sizeof(title),"results/Producer_Times_Period=%d",t->Period);
  writeFile(title,producer_times,t->TasksToExecute);

  t->StopFcn();
  free(sleep_times);
  free(execution_times);
  free(w);
}


//my function for tasks
void my_function(void *arg){
  fprintf(stdout,"waiting_time is %d\n",*(int *)arg);

  pthread_mutex_lock(&waiting_mutex);
  Waiting_times[Waiting_counter]=*(int *)arg;
  Waiting_counter++;
  pthread_mutex_unlock(&waiting_mutex);
}
