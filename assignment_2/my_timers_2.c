#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 50
#define NUMCONSUMERS 10
#define NUMPRODUCERS 1


void *producer(void *args);
void *consumer();


typedef struct{
	int period;
	int taskstoexecute;
	int startdelay;
	int userdata; //maybe??
}timer_informations;


typedef struct{
	void (* TimerFcn)(void *args);
	void *userData;
} workFunction;


typedef struct{
	int Period;
	int TasksToExecute;
	int StartDelay;
	workFunction *function_struct;
	void (*StartFcn)(void *t,void *args_1,void *args2);
	//void (*StopFcn)(void *args);
	//void (*ErrorFcn)(void);
}timer;


typedef struct{
	workFunction buf[QUEUESIZE];
	long head,tail;
	int full,empty;
	int prods_finish, prods_counter;
	int  cons_finish, cons_counter;
	pthread_mutex_t *mut;
	pthread_cond_t *notFull, *notEmpty;
}queue;
queue *QUEUE;


//Queue's Functions
void queueAdd(queue *q, workFunction item);
void queueDel(queue *q, workFunction *out);
void queueDelete(queue *q);
void queueReduceCons(queue *Q);
queue *queueInit(void);


//Timer's function_struct
void StartFunction(void *t,void *args, void *args2);
void TimerFunction(void *args);


int main(){
  QUEUE = queueInit();
  if(QUEUE == NULL){
    printf("ERROR:Cann't create the queue...Maybe next time \n");
    exit(1);
  }

  //initialize the  timer.
  timer_informations t1,t2;
  t1.period = 10000;
  t1.taskstoexecute = 100;
  t1.startdelay = 100;
  t1.userdata = 1000;

  t2.period = 500;
  t2.taskstoexecute = 100;
  t2.startdelay = 100;
  t2.userdata = 9999;


  workFunction *f_1;
  f_1 = (workFunction *)malloc(sizeof(workFunction));
  f_1->TimerFcn = &TimerFunction;
  f_1->userData = malloc(sizeof(int));
  *(int *)f_1->userData = t1.userdata;

  timer *timer1;
  timer1 = (timer *)malloc(sizeof(timer));
  timer1->StartFcn = &StartFunction;
  timer1->StartFcn((void *)timer1,(void* )&t1, (void *)f_1);


  //timer *timer2;
  //timer2 = (timer *)malloc(sizeof(timer));
  //timer2->StartFcn = &StartFunction;
  //*(int *)f_1->userData = t2.userdata;
  //timer2->StartFcn((void *)timer2,(void* )&t2, (void *)f_1);


  //create threads
  pthread_t prods[NUMPRODUCERS];
  pthread_t  cons[NUMCONSUMERS];
  //later i will add the attribute joinable etc.
  //start threads
  pthread_create(&prods[0],NULL,producer,timer1);
//  pthread_create(&prods[1],NULL,producer,timer2);
  int i;
  for(i=0;i<NUMCONSUMERS;i++){
    pthread_create(&cons[i],NULL,consumer,NULL);
  }

  //wait to finish the producer
  pthread_join(prods[0],NULL);
  //pthread_join(prods[1],NULL);
  //wait for consumers to finish
	printf("all PRODUCERS: FINISHED\n");
	pthread_mutex_lock(QUEUE->mut);
	QUEUE->prods_finish = 1;
	pthread_mutex_unlock(QUEUE->mut);
	while(QUEUE->cons_finish == 0){
		pthread_mutex_lock(QUEUE->mut);
		queueReduceCons(QUEUE);
		pthread_mutex_unlock(QUEUE->mut);
		pthread_cond_signal(QUEUE->notEmpty);
	}
  for(i=0;i<NUMCONSUMERS;i++){
    pthread_join(cons[i],NULL);
	}

  return 0;
}


void StartFunction(void *t,void *args, void *args2){

	timer_informations *temp_1;
	temp_1 = (timer_informations *)args;
	timer *temp;
	temp=(timer *)t;

	temp->Period = temp_1->period;
	temp->TasksToExecute = temp_1->taskstoexecute;
	temp->StartDelay = temp_1->startdelay;


  workFunction *temp_2;
  temp_2 = (workFunction *)args2;
	//after that startFunction should link the functions call
	temp->function_struct = (workFunction *)malloc(sizeof(workFunction));
	temp->function_struct->TimerFcn = temp_2->TimerFcn;
	temp->function_struct->userData = (void *)malloc(sizeof(int));
	*(int *)temp->function_struct->userData = *(int *)temp_2->userData;

}


void TimerFunction(void *args){
	int i,counter,sum;
	i = *(int *)args;
	sum = 0;
	printf("Inside the Timer Function \n");
	for(counter = 0;counter<i;counter ++){
		sum = sum + counter;
	}
	printf("the total sum is %d \n",sum);
}


void queueDelete(queue *q){
  pthread_mutex_destroy(q->mut);
  free(q->mut);
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q);
}


void queueAdd(queue *q, workFunction i){
  //this is wrong!
  q->buf[q->tail]= i;
  //int *temp = (int *)i.userData;
  //void *temp_1 = q->buf[q->tail].userData;
  //*(int *)temp_1=*temp;
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
	*out = q->buf[q->head];
  q->head++;
	if(q->head == QUEUESIZE){
		q->head = 0;
	}
	if(q->head == q->tail){
		q->empty = 1;
	}
	q->full = 0;
	return ;
}


queue *queueInit(void){
	queue *q;
	q = (queue *)malloc(sizeof(queue));
	if (q==NULL) return NULL;
	q->prods_finish=0;
 	q->prods_counter=NUMPRODUCERS;
  q->cons_finish=0;
	q->cons_counter=NUMCONSUMERS;
  q->head = 0;
  q->tail = 0;
  q->full = 0;
  q->empty = 1;

  q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

  pthread_mutex_init(q->mut,NULL);
  pthread_cond_init(q->notFull,NULL);
  pthread_cond_init(q->notEmpty,NULL);

  	return q;
}


void *producer(void *args){
	struct timeval total_time;
	struct timeval total_time_1;
	struct timeval total_time_2;
	int time_execution;
	int checker=0;
  timer *t;
  t = (timer *)args;
  int i;
	int temp;
	FILE *fp;
	char str[1024];
	snprintf(str,sizeof(str),"/home/kostas/Documents/rtes/rtes/timer_period=%d",t -> Period);
	fp=fopen(str,"a+");
  for(i = 0; i<t->TasksToExecute; i++){
    pthread_mutex_lock(QUEUE->mut);
		while(QUEUE->full ==1){
			printf("Producer: The QUEUE is full ....Waiting \n");
			pthread_cond_wait(QUEUE->notFull,QUEUE->mut);
		}
    queueAdd(QUEUE,*t->function_struct);
		if(checker == 0){
			gettimeofday(&total_time_1,NULL);
			temp = t->Period;
			fprintf(fp,"execution in %ld usecs\n",total_time_1.tv_usec);
			fprintf(fp,"sleep for %d \n",temp);
			checker =1;
		}else{
			gettimeofday(&total_time_2,NULL);
			time_execution = total_time_2.tv_usec - total_time_1.tv_usec;
			temp = time_execution - temp;
			temp = t->Period - temp;
			fprintf(fp,"execution in %ld usecs\n",total_time_2.tv_usec);
			fprintf(fp,"time between the executions %d\n", time_execution);
			fprintf(fp,"sleep for %d \n",temp);
			total_time_1 = total_time_2;
		}
    pthread_mutex_unlock(QUEUE->mut);
		pthread_cond_signal(QUEUE->notEmpty);
    if(temp>0)usleep(temp);
  }
	printf("PRODUCER: FINISHED\n");
	fclose(fp);
	return (NULL);
}


void *consumer(void ){
	workFunction out;
	while(QUEUE->empty != 1 || QUEUE->prods_finish !=1){
		if(QUEUE->prods_finish == 1)break;
		pthread_mutex_lock(QUEUE->mut);
		while(QUEUE->empty == 1 && QUEUE->prods_finish==0){
			printf("CONSUMER: Queue is empty.....WAITING \n");
			pthread_cond_wait(QUEUE->notEmpty,QUEUE->mut);
		}
		if(QUEUE->prods_finish == 1){
			pthread_mutex_unlock(QUEUE->mut);
			break;
		}
		queueDel(QUEUE,&out);
		out.TimerFcn(out.userData);
		pthread_mutex_unlock(QUEUE->mut);
		if(QUEUE->prods_finish)pthread_cond_signal(QUEUE->notFull);
	}
	while(QUEUE->empty != 1){
		pthread_mutex_lock(QUEUE->mut);
		queueDel(QUEUE,&out);
		out.TimerFcn(out.userData);
		pthread_mutex_unlock(QUEUE->mut);
	}
	return (NULL);
}


void queueReduceCons(queue *Q){
	Q->cons_counter--;
	if(Q->cons_counter == 0){
		Q->cons_finish = 1;
	}
}
