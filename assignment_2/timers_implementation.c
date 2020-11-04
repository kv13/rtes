#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


#define QUEUESIZE 10
#define NUMTIMERS 1
#define NUMCONSUMERS 10


void *producer(void *args);
void *consumer(void *args);


typedef struct {
  void (*work)(void *args);
  void *args;
}workFunction;


typedef struct{
  workFunction *buf;
  long head,tail;
  int full, empty;
  int prods_finish, prods_counter;
  int  cons_finish, cons_counter;
  int missing_jobs;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
}queue;


queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, workFunction item);
void queueDel(queue *q, workFunction *out);
void queueReduceCons(queue *Q);

typedef struct{
  int Period;
  int TasksToExecute;
  int StartDelay;
  void (*StartFcn)(void *arg);
  void (*TimerFcn)(void *arg);
  void (*ErrorFcn)(queue *q);
  void (*StopFcn)(void);
  void *userData;

  queue *Queue;
  pthread_t thread_id;
  void *(*producer)(void *arg);
}timer;


void timer_init(timer *t, int period, int taskstoexecute,int userdata, int startdelay,queue *q);
void start(timer *t);
void startat(timer *t, int year, int month, int day, int hour, int min, int sec);
void stop_function();
void error_function(queue *q);
void my_function(void *arg);

void writeFile(char *str,long int *array,int size);
long int *Waiting_times;
int Waiting_counter=0;

int main(){
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
  if(NUMTIMERS == 1){
    Timer = (timer *)malloc(sizeof(timer));
    if(Timer == NULL){
      fprintf(stderr,"ERROR:Cannot create the Timer object...===> EXITING\n");
      exit(1);
    }
    //periods in seconds
    //int period = 1;
    //int period = 0.1;
    int period = 10000;

    int TasksToExecute = 36;
    Waiting_times = (long int*)malloc(sizeof(long int)*TasksToExecute);
    srand(time(NULL));
    int userdata = rand()%100+1;
    timer_init(Timer, period, TasksToExecute, userdata, 0, Queue);
    start(Timer);
    //startat(Timer,2020,11,2,22,0,0);

  }
  else if(NUMTIMERS == 3){
    Timer = (timer *)malloc(sizeof(timer)*3);
    if(Timer == NULL){
      fprintf(stderr,"ERROR:Cannot create Timer objects...===> EXITING\n");
      exit(1);
    }
    int period_0 = 1;
    int period_1 = 0.1;
    int period_2 = 0.01;
    int TasksToExecute_0 = 3600/period_0;
    int TasksToExecute_1 = 3600/period_1;
    int TasksToExecute_2 = 3600/period_2;
    Waiting_times = (long int*)malloc(sizeof(long int)*(TasksToExecute_0+TasksToExecute_1+TasksToExecute_2));
    int userdata_0 = rand()%100+1;
    int userdata_1 = rand()%100+1;
    int userdata_2 = rand()%100+1;
    timer_init(&Timer[0], period_0, TasksToExecute_0, userdata_0, 0, Queue);
    timer_init(&Timer[1], period_1, TasksToExecute_1, userdata_1, 0, Queue);
    timer_init(&Timer[2], period_2, TasksToExecute_2, userdata_2, 0, Queue);
    start(&Timer[2]);
    start(&Timer[1]);
    start(&Timer[0]);
    //startat(&Timer[0],2020,11,15,14,35,0);
    //startat(&Timer[1],2020,11,15,14,35,0);
    //startat(&Timer[2],2020,11,15,14,35,0);
  }
  if(NUMTIMERS == 1){
    pthread_join(Timer->thread_id,NULL);
  }
  else if(NUMTIMERS == 3){
    pthread_join(Timer[0].thread_id,NULL);
    pthread_join(Timer[1].thread_id,NULL);
    pthread_join(Timer[2].thread_id,NULL);
  }
  fprintf(stdout,"MAIN: PRODUCERS HAVE FINISHED\n");
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
  char title[200];
  snprintf(title,sizeof(title),"/home/kostas/results/Times_Waiting_inQueue");
  writeFile(title,Waiting_times,Waiting_counter);
  fprintf(stdout,"MAIN: CONSUMERS HAVE FINISHED\n ======> EXITING\n");
  return 0;
}


//   THREAD'S FUNCTIONS
void *producer(void *args){
  struct timeval prod_time_1,prod_time_previous,prod_time_2;
  int delay_sec,delay_usec;
  long int *sleep_times,*produce_times,*queue_times,*execution_times;
  timer *t;
  t = (timer *)args;
  long int usec_to_sleep=(long int)t->Period;
  sleep_times = (long int *)malloc(sizeof(long int)*t->TasksToExecute);
  produce_times = (long int *)malloc(sizeof(long int)*t->TasksToExecute);
  queue_times = (long int *)malloc(sizeof(long int)*t->TasksToExecute);
  execution_times = (long int *)malloc(sizeof(long int)*t->TasksToExecute);
  if(sleep_times == NULL || produce_times == NULL || queue_times == NULL){
    fprintf(stderr,"ERROR: Cannot allocate memory for array...===>EXITING\n");
    exit(1);
  }
  fprintf(stdout,"PRODUCER: inside the thread producer....\n");
  workFunction *w;
  w = (workFunction *)malloc(sizeof(workFunction));
  w->args = malloc(sizeof(struct timeval ));
  if(w == NULL || w->args == NULL){
    fprintf(stderr,"ERROR: cannot allocate memory for workFunction inside the producer... EXITING \n");
    exit(1);
  }
  w->work = t->TimerFcn;
  sleep(t->StartDelay);
  for(int i=0;i<t->TasksToExecute;i++){
    pthread_mutex_lock(t->Queue->mut);
    gettimeofday(&prod_time_1,NULL);
    if(t->Queue->full == 0){
      fprintf(stdout,"PRODUCER: Adding to the queue...\n");
      gettimeofday((struct timeval *)(w->args),NULL);
      queueAdd(t->Queue,*w);
      pthread_mutex_unlock(t->Queue->mut);
      pthread_cond_signal(t->Queue->notEmpty);
    }
    else{
      fprintf(stdout,"WARNING: Queue is full drop the data \n");
      t->ErrorFcn(t->Queue);
      pthread_mutex_unlock(t->Queue->mut);
    }
    gettimeofday(&prod_time_2,NULL);
    if(i==0){
      usleep(t->Period);
      sleep_times[0]= (long int)t->Period;
      queue_times[0]=0;
      execution_times[0]=0;
    }
    else{
      delay_sec  = prod_time_1.tv_sec - prod_time_previous.tv_sec;
      delay_usec = prod_time_1.tv_usec-prod_time_previous.tv_usec;
      execution_times[i]=delay_sec*1000000+delay_usec;
      queue_times[i] =delay_usec+delay_sec*1000000-usec_to_sleep;
      usec_to_sleep =(long int) 2*t->Period-delay_usec-delay_sec*1000000;
      if (usec_to_sleep>0){
        usleep(usec_to_sleep);
        sleep_times[i]=usec_to_sleep;
      }
      else{
        sleep_times[i]=0;
      }
    }
    produce_times[i] = prod_time_2.tv_usec-prod_time_1.tv_usec;
    prod_time_previous = prod_time_1;
  }
  //WRITE TIMES TO FILES.
    char title[1000];
    snprintf(title,sizeof(title),"/home/kostas/results/Sleep_Times_Period=%d",t->Period);
    writeFile(title,sleep_times,t->TasksToExecute);
    snprintf(title,sizeof(title),"/home/kostas/results/Producer_Time_Period=%d",t->Period);
    writeFile(title,produce_times,t->TasksToExecute);
    snprintf(title,sizeof(title),"/home/kostas/results/Queue_Time_Period=%d",t->Period);
    writeFile(title,queue_times,t->TasksToExecute);
    snprintf(title,sizeof(title),"/home/kostas/results/Execution_Times_Period=%d",t->Period);
    writeFile(title,execution_times,t->TasksToExecute);

}


void *consumer(void *args){
  long int *waiting_time = malloc(sizeof(long int));
  struct timeval temp;
  workFunction *out;
  out = (workFunction *)malloc(sizeof(workFunction));
  out->args = malloc(sizeof(struct timeval));
  queue *Queue;
  Queue = (queue *)args;
  fprintf(stdout,"CONSUMER:inside the thread consumer....\n");
  for(;;){
    pthread_mutex_lock(Queue->mut);
    while(Queue->empty==1 && Queue->prods_finish!=1){
      fprintf(stdout,"CONSUMER: Queue is empty... WAITING \n");
      pthread_cond_wait(Queue->notEmpty,Queue->mut);
    }
    if(Queue->prods_finish == 1){
      pthread_mutex_unlock(Queue->mut);
      break;
    }
    queueDel(Queue,out);
    gettimeofday(&temp,NULL);
    pthread_mutex_unlock(Queue->mut);
    *waiting_time = (temp.tv_sec- ((struct timeval *)(out->args))->tv_sec)*1000000;
    *waiting_time += (temp.tv_usec- ((struct timeval *)(out->args))->tv_usec);
    out->work((void *)waiting_time);
  }
  for(;;){
    pthread_mutex_lock(Queue->mut);
    if(Queue->empty){
      pthread_mutex_unlock(Queue->mut);
      break;
    }
    queueDel(Queue,out);
    pthread_mutex_unlock(Queue->mut);
    *waiting_time = (temp.tv_sec- ((struct timeval *)(out->args))->tv_sec)*1000000;
    *waiting_time += (temp.tv_usec- ((struct timeval *)(out->args))->tv_usec);
    out->work((void *)waiting_time);
  }
  return NULL;

}


//TIMER'S FUNCTIONS
void timer_init(timer *t, int period, int taskstoexecute, int userdata, int startdelay,queue *q){
  t->Period = period;
  t->TasksToExecute = taskstoexecute;
  t->StartDelay = 0;
  t->Queue = q;
  t->producer = &producer;
  t->StartFcn = NULL;
  t->StopFcn = &stop_function;
  t->TimerFcn = &my_function;
  t->ErrorFcn = &error_function;
  t->userData = malloc(sizeof(int));
  if(t->userData == NULL){
    fprintf(stderr,"TIMER INITIALIZATION: Cannot create the timer object... EXITING \n");
    exit(1);
  }
  *(int*)t->userData=userdata;
}


void start(timer *t){
  pthread_create(&t->thread_id,NULL,t->producer,(void *)t);
}


void startat(timer *t, int year, int month, int day, int hour, int min, int sec){
  time_t t_1 = time(NULL);
  struct tm tm = *localtime(&t_1);
  printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  //compute seconds from 01/01/1970
  long long int seconds_now = (tm.tm_year-70)*365*24*3600+tm.tm_mon*30*24*3600+tm.tm_mday*24*3600+tm.tm_hour*3600+tm.tm_min*60+tm.tm_sec;
  long long int seconds_fut = (year-1970)*365*24*3600+(month-1)*30*24*3600+day*24*3600+hour*3600+min*60+sec;
  long long int delay = seconds_fut-seconds_now;
  t->StartDelay = (int)delay;
  pthread_create(&t->thread_id,NULL,*t->producer,(void *)t);
}


void error_function(queue *q){
  q->missing_jobs++;
}


void stop_function(){
  fprintf(stdout,"TIMER STATUS: FINISHED\n");
}


void my_function(void *arg){
  printf("the waiting time is %ld\n",*(long int *)arg);
  Waiting_times[Waiting_counter]=*(long int *)arg;
  Waiting_counter++;
}


// QUEUE'S FUNCTIONS
queue *queueInit(void){
	queue *q;
	q = (queue *)malloc(sizeof(queue));
	if (q==NULL) return NULL;
  q->buf = (workFunction *)malloc(sizeof(workFunction)*QUEUESIZE);
  if(q->buf == NULL) return NULL;
	q->prods_finish = 0;
 	q->prods_counter = NUMTIMERS;
  q->cons_counter = NUMCONSUMERS;
  q->cons_finish = 0;
  q->head = 0;
  q->tail = 0;
  q->full = 0;
  q->missing_jobs = 0;
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


void queueAdd(queue *q, workFunction item){
  q->buf[q->tail].work=item.work;
  q->buf[q->tail].args = malloc(sizeof(struct timeval));
  *(struct timeval *)(q->buf[q->tail].args)=*(struct timeval *)item.args;
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
	*(struct timeval *)(out->args) = *(struct timeval *)(q->buf[q->head].args);
  out->work = q->buf[q->head].work;
  q->head++;
	if(q->head == QUEUESIZE){
		q->head = 0;
	}
	if(q->head == q->tail){
		q->empty = 1;
	}
	q->full = 0;
}


void queueReduceCons(queue *Q){
	Q->cons_counter--;
	if(Q->cons_counter == 0){
		Q->cons_finish = 1;
	}
}


void writeFile(char *str,long int *array,int size){
  FILE *fp;
  fp = fopen(str,"a+");
  if( fp == NULL ){
    fprintf(stderr,"ERROR: Cannot open files...EXITING\n");
    exit(1);
  }
  for(int i =0;i<size;i++){
    fprintf(fp,"%ld \n", array[i]);
  }
  fclose(fp);

}
