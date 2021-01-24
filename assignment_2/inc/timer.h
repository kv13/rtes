#ifndef TIMER_H
#define TIMER_H

#include "queue.h"

#define NUMTIMERS 3

//Define timer struct
typedef struct{
  int Period;                                           //time period in useconds
  int TasksToExecute;                                   //total tasks to execute
  long int StartDelay;                                  //initial delay in seconds
  void (*StartFcn)(void *arg);                          //pointer to function start before the main execution
  void (*TimerFcn)(void *arg);                          //pointer to the main function
  void (*ErrorFcn)(queue *q);                           //pointer to function that runs when queue full
  void (*StopFcn)(void);                                //pointer for function that runs after timer's last iteration
  void *userData;                                       //timers data

  queue *Queue;                                        //pointer to Queue
  pthread_t thread_id;                                 //thread id which corresponds to timer object
  void *(*producer)(void *arg);                        //pointer to producer function

}timer;

/***************************************QUEUE FUNCTIONS*******************************************/

//timer constructor function
void timer_init(timer *t, int period, int taskstoexecute,int userdata, int startdelay,queue *q, void *(*prod_func)(void *),void (*my_func)(void *));

//start timer immediately
void start(timer *t);

//start timer at specific time
void startat(timer *t, int year, int month, int day, int hour, int min, int sec);

//function to execute after timer finish
void stop_function();

//function to execute when error occurs
void error_function(queue *q);

//write results to file
void writeFile(char *str, int *array, int size);

#endif
