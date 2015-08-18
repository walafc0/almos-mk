#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "dietstdio.h"

typedef void (*function)(void);

#define NUM_ATEXIT	32

static function __atexitlist[NUM_ATEXIT];
static int atexit_counter;

int atexit(function t) {
  if (atexit_counter<NUM_ATEXIT) {
    __atexitlist[atexit_counter]=t;
    ++atexit_counter;
    return 0;
  }
  return -1;
}


void exit(int status)
{
  register int i=atexit_counter;
  
  //__stdio_flushall(); 
  
  while(i) {
    __atexitlist[--i]();
  }

  pthread_exit((void*) status);
  exit(status);
}
