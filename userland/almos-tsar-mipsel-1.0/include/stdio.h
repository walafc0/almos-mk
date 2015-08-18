#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <sys/types.h>
#include <endian.h>

struct __stdio_file;
typedef struct __stdio_file FILE;

extern FILE *stdin, *stdout, *stderr;

FILE *fopen (const char *path, const char *mode) ;
FILE *fdopen (int fildes, const char *mode) ;
FILE *freopen (const char *path, const char *mode, FILE *stream) ;

int printf(const char *format, ...)  __attribute__((__format__(__printf__,1,2)));
int fprintf(FILE *stream, const char *format, ...)  __attribute__((__format__(__printf__,2,3)));
int sprintf(char *str, const char *format, ...)  __attribute__((__format__(__printf__,2,3)));
int snprintf(char *str, size_t size, const char *format, ...)  __attribute__((__format__(__printf__,3,4)));
int asprintf(char **ptr, const char* format, ...)  __attribute__((__format__(__printf__,2,3)));

int scanf(const char *format, ...)  __attribute__((__format__(__scanf__,1,2)));
int fscanf(FILE *stream, const char *format, ...)  __attribute__((__format__(__scanf__,2,3)));
int sscanf(const char *str, const char *format, ...)  __attribute__((__format__(__scanf__,2,3)));

int vprintf(const char *format, va_list ap)  __attribute__((__format__(__printf__,1,0)));
int vfprintf(FILE *stream, const char *format, va_list ap)  __attribute__((__format__(__printf__,2,0)));
int vsprintf(char *str, const char *format, va_list ap)  __attribute__((__format__(__printf__,2,0)));
int vsnprintf(char *str, size_t size, const char *format, va_list ap)  __attribute__((__format__(__printf__,3,0)));

int fdprintf(int fd, const char *format, ...)  __attribute__((__format__(__printf__,2,3)));
int vfdprintf(int fd, const char *format, va_list ap)  __attribute__((__format__(__printf__,2,0)));

int vscanf(const char *format, va_list ap)  __attribute__((__format__(__scanf__,1,0)));
int vsscanf(const char *str, const char *format, va_list ap)  __attribute__((__format__(__scanf__,2,0)));
int vfscanf(FILE *stream, const char *format, va_list ap)  __attribute__((__format__(__scanf__,2,0)));

int fgetc(FILE *stream) ;
int fgetc_unlocked(FILE *stream) ;
char *fgets(char *s, int size, FILE *stream) ;
char *fgets_unlocked(char *s, int size, FILE *stream) ;

char *gets(char *s) ;
int ungetc(int c, FILE *stream) ;
int ungetc_unlocked(int c, FILE *stream) ;

int fputc(int c, FILE *stream) ;
int fputc_unlocked(int c, FILE *stream) ;
int fputs(const char *s, FILE *stream) ;
int fputs_unlocked(const char *s, FILE *stream) ;

int getc(FILE *stream) ;
int getchar(void) ;
int putchar(int c) ;
int putchar_unlocked(int c) ;

#if !defined(__cplusplus)
#define putc(c,stream) fputc(c,stream)
#define putchar(c) fputc(c,stdout)
#define putc_unlocked(c,stream) fputc_unlocked(c,stream)
#define putchar_unlocked(c) fputc_unlocked(c,stdout)
#else
inline int putc(int c, FILE *stream)  { return fputc(c,stream); }
inline int putc_unlocked(int c, FILE *stream)  { return fputc_unlocked(c,stream); }
#endif

#if !defined(__cplusplus)
#define getc(stream) fgetc(stream)
#define getchar() fgetc(stdin)
#define getc_unlocked(stream) fgetc_unlocked(stream)
#define getchar_unlocked() fgetc_unlocked(stdin)
#else
inline int getc_unlocked(FILE *stream)  { return fgetc_unlocked(stream); }
inline int getchar_unlocked(void)  { return fgetc_unlocked(stdin); }
#endif

int puts(const char *s) ;

int fseek(FILE *stream, long offset, int whence) ;
int fseek_unlocked(FILE *stream, long offset, int whence) ;
long ftell(FILE *stream) ;
long ftell_unlocked(FILE *stream) ;
int fseeko(FILE *stream, off_t offset, int whence) ;
int fseeko_unlocked(FILE *stream, off_t offset, int whence) ;
off_t ftello(FILE *stream) ;
off_t ftello_unlocked(FILE *stream) ;

#if __WORDSIZE == 32

#if defined _FILE_OFFSET_BITS && _FILE_OFFSET_BITS == 64
#define off_t loff_t
#define fseeko(foo,bar,baz) fseeko64(foo,bar,baz)
#define ftello(foo) ftello64(foo)
#endif

#endif

void rewind(FILE *stream) ;
int fgetpos(FILE *stream, fpos_t *pos) ;
int fsetpos(FILE *stream, fpos_t *pos) ;

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) ;
size_t fread_unlocked(void *ptr, size_t size, size_t nmemb, FILE *stream) ;

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) ;
size_t fwrite_unlocked(const void *ptr, size_t size, size_t nmemb, FILE *stream) ;

int fflush(FILE *stream) ;
int fflush_unlocked(FILE *stream) ;

int fclose(FILE *stream) ;
int fclose_unlocked(FILE *stream) ;

int feof(FILE *stream) ;
int feof_unlocked(FILE *stream) ;
int ferror(FILE *stream) ;
int ferror_unlocked(FILE *stream) ;
int fileno(FILE *stream) ;
int fileno_unlocked(FILE *stream) ;
void clearerr(FILE *stream) ;
void clearerr_unlocked(FILE *stream) ;

int remove(const char *pathname) ;
int rename(const char *oldpath, const char *newpath) ;

void perror(const char *s) ;

#define EOF (-1)

#define BUFSIZ 128

#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2

int setvbuf(FILE *stream, char *buf, int mode , size_t size) ;
int setvbuf_unlocked(FILE *stream, char *buf, int mode , size_t size) ;

#if !defined(__cplusplus)
#define setbuf(stream,buf) setvbuf(stream,buf,buf?_IOFBF:_IONBF,BUFSIZ)
#define setbuffer(stream,buf,size) setvbuf(stream,buf,buf?_IOFBF:_IONBF,size)
#define setlinebuf(stream) setvbuf(stream,0,_IOLBF,BUFSIZ)
#else
inline int setbuf(FILE *stream, char *buf) 
  { return setvbuf(stream,buf,buf?_IOFBF:_IONBF,BUFSIZ); }
inline int setbuffer(FILE *stream, char *buf, size_t size) 
  { return setvbuf(stream,buf,buf?_IOFBF:_IONBF,size); }
inline int setlinebuf(FILE *stream) 
  { return setvbuf(stream,0,_IOLBF,BUFSIZ); }
#endif

FILE *popen(const char *command, const char *type) ;
int pclose(FILE *stream) ;


#define L_tmpnam 128
#define P_tmpdir "/tmp"
char* tmpnam(char *s) ;	/* DO NOT USE!!! Use mkstemp instead! */
char* tempnam(char* dir,char* _template);	/* dito */
FILE* tmpfile(void) ;
FILE* tmpfile_unlocked(void) ;

#define FILENAME_MAX NAME_MAX 
#define FOPEN_MAX 16

#define TMP_MAX 10000

/* this is so bad, we moved it to -lcompat */
#define L_ctermid 9
char* ctermid(char* s); /* returns "/dev/tty" */

void flockfile(FILE* f) ;
void funlockfile(FILE* f) ;
int ftrylockfile (FILE *__stream) ;

#ifdef _GNU_SOURCE
int vasprintf(char **strp, const char *fmt, va_list ap);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
#endif

#endif
