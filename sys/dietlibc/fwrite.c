#include "dietstdio.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>

size_t fwrite_unlocked(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  ssize_t res;
  size_t len=size*nmemb;
  size_t i,done;

  //log_msg("fwrite_unlocked started, streem's fd %d, size %d, nmemb %d\n", stream->fd, size, nmemb);
  if (!(stream->flags&CANWRITE) || __fflush4(stream,0)) {
kaputt:
    //log_msg("fwrite_unlocked errnor at line %d, streem's fd %d, can write ? %d\n", __LINE__, stream->fd,
    //	    stream->flags & CANWRITE);
    stream->flags|=ERRORINDICATOR;
    return 0;
  }

  if (!nmemb || len/nmemb!=size) return 0; /* check for integer overflow */
  
  if (len>stream->buflen || (stream->flags&NOBUF)) {
    if (fflush_unlocked(stream)) return 0;
    i=2;
    do {
      res= write(stream->fd,ptr,len);
    } while ((res==-1) && (i--));// && errno==EINTR);
  } else {
    /* try to make the common case fast */
    size_t todo=stream->buflen-stream->bm;
    if (todo>len) todo=len;
    
    if (todo) {
      if (stream->flags&BUFLINEWISE) {
	for (i=0; i<todo; ++i) {
	  if ((stream->buf[stream->bm++]=((char*)ptr)[i])=='\n') {
	    if (fflush_unlocked(stream)) goto kaputt;
	  }
	}
      } else {
	memcpy(stream->buf+stream->bm,ptr,todo);
	stream->bm+=todo;
      }
      done=todo;
    } else
      done=0;
    for (i=done; i<len; ++i)
      if (fputc_unlocked(((char*)ptr)[i],stream)) {
	res=len-i;
	goto abort;
      }
    res=len;
  }
  if (res<0) {
    stream->flags|=ERRORINDICATOR;
    return 0;
  }
abort:
  return size?res/size:0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) __attribute__((weak,alias("fwrite_unlocked")));
