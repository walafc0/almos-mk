#include "dietstdio.h"
#include <unistd.h>
#include <string.h>

size_t fread_unlocked(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  int res;
  unsigned long i,j;
  j=size*nmemb;
  i=0;
  ///log_msg("fread started fd %d, size %d, nmemb %d\n", stream->fd, size, nmemb);
  
  if (!(stream->flags&CANREAD)) {
    log_msg("fread: fd %d, can't read %d\n", stream->fd, (int)j);
    stream->flags|=ERRORINDICATOR;
    return 0;
  }
  if (!j || j/nmemb!=size) return 0;
  
  log_msg("fread: fd %d, line %d\n", stream->fd, __LINE__);
  
  if (stream->ungotten) {
    stream->ungotten=0;
    *(char*)ptr=stream->ungetbuf;
    ++i;
    log_msg("fread, line %d\n", __LINE__);
    if (j==1) return 1;
  }

#ifdef WANT_FREAD_OPTIMIZATION
  if ( !(stream->flags&FDPIPE) && (j>stream->buflen)) {
    size_t tmp=j-i;
    ssize_t res;
    size_t inbuf=stream->bs-stream->bm;
    memcpy(ptr+i,stream->buf+stream->bm,inbuf);
    stream->bm=stream->bs=0;
    tmp-=inbuf;
    i+=inbuf;
    
    if (fflush_unlocked(stream)) return 0;
    while ((res=read(stream->fd,ptr+i,tmp))<(ssize_t)tmp) {
      if (res==-1) {
	stream->flags|=ERRORINDICATOR;
	goto exit;
      } else if (!res) {
	stream->flags|=EOFINDICATOR;
	goto exit;
      }
      i+=res; tmp-=res;
    }
    return nmemb;
  }
#endif
 
  for (; i<j; ++i) {
    res=fgetc_unlocked(stream);
    log_msg("fread, line %d, res %d, i %d, j %d\n", __LINE__, res,(int)i,(int)j);
    if (res==EOF)
exit:
      //log_msg("fread, line %d\n", __LINE__);
      return i/size;
    else
      ((unsigned char*)ptr)[i]=(unsigned char)res;
  }
  //log_msg("fread, line %d, nmemb %d\n", __LINE__, nmemb);
  return nmemb;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) __attribute__((weak,alias("fread_unlocked")));
