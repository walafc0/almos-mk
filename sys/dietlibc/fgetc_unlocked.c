#include "dietstdio.h"
#include <unistd.h>

int fgetc_unlocked(FILE *stream) {
  unsigned char c;
  if (!(stream->flags&CANREAD)) goto kaputt;

  if (stream->ungotten) {
    stream->ungotten=0;
    return stream->ungetbuf;
  }

  log_msg("fgetc fd %d, mb %d, bs %d\n", stream->fd, stream->bm, stream->bs);
  /* common case first */
  if (stream->bm<stream->bs)
  {
    log_msg("fgetc fd %d, %c\n", stream->fd, stream->buf[stream->bm]);
    return (unsigned char)stream->buf[stream->bm++];
  }

  if (feof_unlocked(stream))
    return EOF;
  
  if (__fflush4(stream,BUFINPUT)) return EOF;
  if (stream->bm>=stream->bs) {
    ssize_t len= read(stream->fd,stream->buf,stream->buflen);
    if (len==0) {
      stream->flags|=EOFINDICATOR;
      return EOF;
    } else if (len<0) {
kaputt:
      stream->flags|=ERRORINDICATOR;
      return EOF;
    }
    stream->bm=0;
    stream->bs=len;
  }
  c=stream->buf[stream->bm];
  ++stream->bm;
  return c;
}

int fgetc(FILE* stream) __attribute__((weak,alias("fgetc_unlocked")));
