#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "dietstdio.h"
#include <stdlib.h>
#ifdef WANT_THREAD_SAFE
#include <pthread.h>
#endif

extern int __stdio_atexit;

FILE*__stdio_init_file(int fd,int closeonerror,int mode) {
  FILE *tmp=(FILE*)malloc(sizeof(FILE));
  if (!tmp) goto err_out;
  tmp->buf=(char*)malloc(BUFSIZE);
  if (!tmp->buf) {
    free(tmp);
err_out:
    if (closeonerror) close(fd);
    errno=ENOMEM;
    return 0;
  }
  tmp->fd=fd;
  tmp->bm=0;
  tmp->bs=0;
  tmp->buflen=BUFSIZE;
  {
    //struct stat st;
    //st.st_mode = 0;
    //fstat(fd,&st);
    //tmp->flags=(S_ISFIFO(st.st_mode))?FDPIPE:0;
  }
  tmp->flags = 0;
  tmp->flags = (mode & O_WRONLY) ? tmp->flags | CANWRITE : tmp->flags;
  tmp->flags = (mode & O_RDONLY) ? tmp->flags | CANREAD : tmp->flags;

  tmp->popen_kludge=0;
  if (__stdio_atexit==0) {
    __stdio_atexit=1;
    atexit(__stdio_flushall);
  }
  tmp->next=__stdio_root;
  __stdio_root=tmp;
  tmp->ungotten=0;

  return tmp;
}

FILE* __stdio_init_file_nothreads(int fd,int closeonerror,int mode) __attribute__((alias("__stdio_init_file")));
