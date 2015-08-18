#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "dietfeatures.h"

int execv(const char *file, char *const argv[]) {
  if (execve(file,argv,environ)==-1) {
    return -1;
  }
  return -1;
}
