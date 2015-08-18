#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#define BUFF_SIZE   5		/* total number of slots */
#define NP          3		/* total number of producers */
#define NC          3		/* total number of consumers */
#define NITERS      4		/* number of items produced/consumed */

typedef struct {
    int buf[BUFF_SIZE];   /* shared var */
    int in;         	  /* buf[in%BUFF_SIZE] is the first empty slot */
    int out;        	  /* buf[out%BUFF_SIZE] is the first full slot */
    sem_t full;     	  /* keep track of the number of full spots */
    sem_t empty;    	  /* keep track of the number of empty spots */
    sem_t mutex;    	  /* enforce mutual exclusion to shared data */
} sbuf_t;

sbuf_t shared;
pthread_spinlock_t lock;

#if 0
#define printf_r(MSG, ...)			\
  do									\
  {									\
    pthread_spin_lock(&lock);						\
    printf(MSG, __VA_ARGS__);						\
    fflush(stdout);							\
    pthread_spin_unlock(&lock);					\
  }while(1)
#else
#define printf_r(MSG, ...) printf(MSG, __VA_ARGS__)
#endif

void *Producer(void *arg)
{
    int i, item, index;
    int err;
    
    index = (int)arg;

    for (i=0; i < NITERS; i++) {

        /* Produce item */
        item = i;      
        /* Write item to buf */
        /* If there are no empty slots, wait */
        if((err=sem_wait(&shared.empty)))
	  fprintf(stderr, "Producer: faild to sem_wait on empty, err %d\n", err);
        /* If another thread uses the buffer, wait */
        if((err=sem_wait(&shared.mutex)))
	  fprintf(stderr, "Producer: faild to sem_wait on mutex, err %d\n", err);

        shared.buf[shared.in] = item;
        shared.in = (shared.in+1)%BUFF_SIZE;
	printf_r("[P%d] Producing %d , iter %d ...\n", index, item, i);
        /* Release the buffer */
        if((err=sem_post(&shared.mutex)))
	  fprintf(stderr, "Producer: faild to sem_post on mutex, err %d\n", err);
        /* Increment the number of full slots */
        if((err=sem_post(&shared.full)))
	  fprintf(stderr, "Producer: faild to sem_post on full, err %d\n", err);

        /* Interleave  producer and consumer execution */
        if (i % 2 == 1) sleep(1);
    }
    return NULL;
}

void *Consumer(void *arg)
{
  int i, item, index,err;
    
    index = (int)arg;
    //printf("Started [C%d]\n", index);  fflush(stdout);
    for (i=0; i < NITERS; i++) {
    
      /* Consuming item */
      /* If there are no full slots, wait */
      if((err=sem_wait(&shared.full)))
	fprintf(stderr, "Consumer: faild to sem_wait on full, err %d\n", err);
      
      /* If another thread uses the buffer, wait */
      if((err=sem_wait(&shared.mutex)))
	fprintf(stderr, "Consumer: faild to sem_wait on mutex, err %d\n", err);

      item = shared.buf[shared.out];
      shared.out = (shared.out+1)%BUFF_SIZE;
      printf_r("-----> [C%d] Consuming %d, iter %d ...\n", index, item, i);
      /* Release the buffer */
      if((err=sem_post(&shared.mutex)))
	fprintf(stderr, "Consumer: faild to sem_post on mutex, err %d\n", err);
      /* Increment the number of empty slots */
      if((err=sem_post(&shared.empty)))
	fprintf(stderr, "Consumer: faild to sem_post on empty, err %d\n", err);

      /* Interleave  producer and consumer execution */
      if (i % 2 == 1) sleep(1);
    }
}

int main()
{
    pthread_t idP, idC;
    pthread_attr_t attr;

    int index;

    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    sem_init(&shared.full, 0, 0);
    sem_init(&shared.empty, 0, BUFF_SIZE);
    sem_init(&shared.mutex, 0, 1);

    /* Insert code here to initialize mutex*/

    for (index = 0; index < NP; index++)
    {  
       /* Create a new producer */
       pthread_create(&idP, &attr, Producer, (void*)index);
    }

    /* Insert code here to create NC consumers */

    for (index = 0; index < NC; index++)
    {  
       /* Create a new producer */
       pthread_create(&idC, &attr, Consumer, (void*)index);
    }
    
    //while(1);
    //pthread_exit(NULL);
}
