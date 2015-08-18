#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef printf_r
#define printf_r(...)					\
	do{						\
		pthread_mutex_lock(&output_lock);	\
		printf(__VA_ARGS__);			\
		pthread_mutex_unlock(&output_lock);	\
	}while(0)
#endif

static pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;

void *thread_func(void *arg)
{
	int pid;
	int tid;

	pid = getpid();
	tid = pthread_self();

	printf_r("pid %d, tid %d, arg %d: Hello World !!\n",
		 pid,
		 tid,
		 (int) arg);
	
	return arg;
}


int main(int argc, char *argv[])
{
	pthread_t *tids_tbl;
	int count;
	int state;
	int i,err;

	count = sysconf(_SC_NPROCESSORS_ONLN);
	count = (count < 1) ? 1 : count;
 
	tids_tbl = malloc(sizeof(pthread_t) * count);
	
	if(tids_tbl == NULL)
	{
		printf("Main: failed to allocate tids_tbl\n");
		exit(-1);
	}
	
	for(i = 0; i < count; i++)
	{
		err = pthread_create(&tids_tbl[i], NULL, thread_func, (void*) i);

		if(err)
		{
			printf_r("Main: failed to create thread #%d, err %d\n", i, err);
			break;
		}
	}
	
	count = i;

	for(i = 0; i < count; i++)
	{
		err = pthread_join(tids_tbl[i], (void**) &state);
		
		if(err)
			printf_r("Main: failed to join thread #%d, err %d\n", i, err);
		else
			printf_r("Main: thread #%d has finished with return value %d\n", i, state);
	}

	free(tids_tbl);
	return 0;
}
