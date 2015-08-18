#include <stdlib.h>
#include <stdio.h>
#include "dietstdio.h"
//#include <pthread.h>

void abort(void) {
  //pthread_exit((void*)127);
  //abort();
  exit(127);
}
