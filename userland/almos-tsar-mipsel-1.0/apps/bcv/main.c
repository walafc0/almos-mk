#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


typedef struct barrier_s
{
  pthread_mutex_t lock;
  int count;
  int value;
  pthread_cond_t runcond;
}barrier_t;

static barrier_t global_barrier;

static void barrier_init(barrier_t *barrier, int value)
{
  int err;
  
  if ((err = pthread_mutex_init (&barrier->lock, NULL)) != 0)
  {
    fprintf (stderr, "Error, faild to init mutex with this error code: %d \n", err);
    pthread_exit ((void *) EXIT_FAILURE);
  }
  
  err = pthread_cond_init(&barrier->runcond, NULL);

  if(err)
  {
    printf("failed to init cond var\n");
    pthread_exit((void*) EXIT_FAILURE);
  }

  barrier->value = value;
  barrier->count = 0;
}

static void barrier_wait(barrier_t *barrier, int id)
{
  int err;

  err = pthread_mutex_lock(&barrier->lock);
  
  if(err)
   {
     fprintf (stderr, "Error: faild to lock mutex [%d]\n", err);
     pthread_exit ((void *) EXIT_FAILURE);
   }

  barrier->count ++;
  
  if(barrier->count == barrier->value)
  {
    printf("thread %d: broadcast\n", id);
    err = pthread_cond_broadcast(&barrier->runcond);

    if(err)
      printf("thread %d: failed to broadcast event\n", id);
  }
  else
  {
    printf("thread %d: going to wait\n", id);

    err = pthread_cond_wait(&barrier->runcond, &barrier->lock);

    if(err)
      printf("thread %d: failed to cond wait\n", id);

    if(barrier->count != barrier->value)
      fprintf(stderr, "thread %d: Unexpected event\n", id);
  }
  
  printf("thread %d: barrier end\n", id);
  err = pthread_mutex_unlock(&barrier->lock);

  if(err)
    printf("thread %d: failed to unlock mutex\n", id);
}


static void *thread_func (void *id)
{
   register int err, i = 0;
   register int v = (int *) id;

   if( id == 0)
     sleep(3);

   barrier_wait(&global_barrier, v);
   pthread_exit((void*)v);
}


int main ()
{
   char *buff;
   int i, err;
   int *ptr_id;
   int state;
   pthread_attr_t attr;
   char ch;
   int cpu_count;

   if (pthread_attr_init (&attr) != 0)
      fprintf (stderr, "Error initialization of thread's attribute \n ");

   cpu_count = sysconf(_SC_NPROCESSORS_ONLN);

   if(cpu_count <= 0)
   {
     fprintf(stderr, "Error, failed to get ONLN CPUs number\n");
     pthread_exit((void*) EXIT_FAILURE);
   }

   pthread_t th[cpu_count];

   barrier_init(&global_barrier, cpu_count);

   for (i = 0; i < cpu_count; i++)
   {
      
#ifdef _ALMOS_
      pthread_attr_setcpuid_np (&attr, i % cpu_count, NULL);
#endif
   
      if ((err = pthread_create (&th[i], &attr, thread_func, (void *)i)) != 0)
      {
         fprintf (stderr, " Error creating thread ");
         pthread_exit ((void *) EXIT_FAILURE);
      }

      //fprintf (stderr, " Main:thread %d <=> %x has been created\n", i, th[i]);
   }

   puts ("Main: waiting end of all threads");

   for (i = 0; i < cpu_count; i++)
   {
      if ((err = pthread_join (th[i], (void **) &state)) != 0)
      {
         fprintf (stderr, "Error, faild to join on thread %x , with this error code: %d\n", th[i], err);
         pthread_exit ((void *) EXIT_FAILURE);
      }
      fprintf (stderr, "Main: thread #%x has finished, exit value: %d\n", th[i], state);
   }

   puts ("Main: all created threads have been finished");
   
   return EXIT_SUCCESS;
}
