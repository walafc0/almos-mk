#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "dietfeatures.h"


int execl(const char *path, const char *arg, ...){
//int execl( const char *path,...) {
  va_list ap,bak;
  int n,i;
  char **argv,*tmp;
  va_start(ap, arg);
  va_copy(bak,ap);
  n=2;
  while ((tmp=va_arg(ap,char *)))
    ++n;
  va_end (ap);
  if ((argv=(char **)alloca(n*sizeof(char*)))) {
    for (i=0; i<n; ++i)
	    argv[i]=va_arg(bak,char *);
    va_end (bak);

    argv[0]   = (char*)path;
    argv[n-1] = NULL;
    return execve(path,argv,environ);
  }
  va_end (bak);
  errno=ENOMEM;
  return -1;
}
