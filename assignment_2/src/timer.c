#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "../inc/timer.h"
#include "../inc/queue.h"


//timer constructor function
void timer_init(timer *t, int period, int taskstoexecute, int userdata, int startdelay,queue *q, void *(*prod_func)(void *),void (*my_func)(void *)){

  t->Period         = period;
  t->TasksToExecute = taskstoexecute;
  t->StartDelay     = 0;
  t->Queue          = q;
  t->producer       = prod_func;
  t->TimerFcn       = my_func;
  t->StartFcn       = NULL;
  t->StopFcn        = &stop_function;
  t->ErrorFcn       = &error_function;

  t->userData       = malloc(sizeof(int));
  if(t->userData == NULL){
    //fprintf(stderr,"TIMER INITIALIZATION: Cannot create the timer object... EXITING \n");
    exit(1);
  }

  *(int*)t->userData=userdata;
}


//function to start producer immediately
void start(timer *t){
  pthread_create(&t->thread_id,NULL,t->producer,(void *)t);
}


//function to start producer at specific time
void startat(timer *t, int year, int month, int day, int hour, int min, int sec){
  time_t t_1 = time(NULL);
  struct tm tm = *localtime(&t_1);
  //printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  //compute seconds from 01/01/1970
  long long int seconds_now = (tm.tm_year-70)*365*24*3600+tm.tm_mon*30*24*3600+tm.tm_mday*24*3600+tm.tm_hour*3600+tm.tm_min*60+tm.tm_sec;
  long long int seconds_fut = (year-1970)*365*24*3600+(month-1)*30*24*3600+day*24*3600+hour*3600+min*60+sec;

  long int delay            = seconds_fut-seconds_now;
  t->StartDelay             = delay;

  fprintf(stdout,"The starting delay is %ld\n",delay);
  pthread_create(&t->thread_id,NULL,*t->producer,(void *)t);
}


//error function
void error_function(queue *q){
  q->missing_jobs++;
  printf("***************************FUCK*************************\n");
}


//function to execute after timer finish
void stop_function(){
  fprintf(stdout,"TIMER STATUS: FINISHED\n");
}


//write results to file function
void writeFile(char *str, int *array,int size){
  FILE *fp;
  fp = fopen(str,"a+");

  if( fp == NULL ){
    fprintf(stderr,"ERROR: Cannot open files...EXITING\n");
    exit(1);
  }

  for(int i =0;i<size;i++){
    fprintf(fp,"%d \n", array[i]);
  }

  fclose(fp);
}
