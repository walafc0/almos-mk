#ifndef _STRING_H
#define _STRING_H

#include <sys/types.h>


char *strcpy(char*  dest, const char*  src);

void *memccpy(void*  dest, const void*  src, int c, size_t n);
void *memmove(void* dest, const void *src, size_t n);

int memccmp(const void* s1, const void* s2, int c, size_t n);

void* memset(void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n) ;
void* memcpy(void*  dest, const void*  src, size_t n);

char *strncpy(char*  dest, const char*  src, size_t n);
int strncmp(const char* s1, const char* s2, size_t n) ;

char *strcat(char*  dest, const char*  src);
char *strncat(char*  dest, const char*  src, size_t n);

int strcmp(const char *s1, const char *s2) ;

size_t strlen(const char *s);

#ifdef _GNU_SOURCE
size_t strnlen(const char *s,size_t maxlen) ;
#endif

char *strstr(const char *haystack, const char *needle) ;

char *strdup(const char *s) ;

char *strchr(const char *s, int c) ;
char *strrchr(const char *s, int c) ;

size_t strspn(const char *s, const char *_accept);
size_t strcspn(const char *s, const char *reject);

char *strpbrk(const char *s, const char *_accept);
char *strsep(char **  stringp, const char *  delim);

void* memchr(const void *s, int c, size_t n) ;

#ifdef _GNU_SOURCE
void* memrchr(const void *s, int c, size_t n);
#endif


char *strerror(int errnum);

/* work around b0rken GNU crapware like tar 1.13.19 */
#define strerror strerror
int strerror_r(int errnum,char* buf,size_t n);

#ifdef _GNU_SOURCE
const char *strsignal(int signum);
void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);

void* mempcpy(void*  dest,const void*  src,size_t n);

char *strndup(const char *s,size_t n);

#define strdupa(s) ({ const char* tmp=s; size_t l=strlen(tmp)+1; char* x=alloca(l); memcpy(x,tmp,l); })
#define strndupa(s,n) ({ const char* tmp=s; const char* y=memchr(tmp,0,(n)); size_t l=y?y-tmp:n; char* x=alloca(l+1); x[l]=0; memcpy(x,tmp,l); })
#endif

char *strtok(char *  s, const char *  delim);
char *strtok_r(char *  s, const char *  delim, char **  ptrptr);

size_t strlcpy(char *  dst, const char *  src, size_t size);
size_t strlcat(char *  dst, const char *  src, size_t size);

int strcoll(const char *s1, const char *s2);
size_t strxfrm(char *dest, const char *  src, size_t n);

#ifdef _BSD_SOURCE
#include <strings.h>
#endif

char *stpcpy(char *  dest, const char *  src);
char* stpncpy(char*  dest, const char*  src, size_t n);

#ifdef _GNU_SOURCE
int ffsl(long i);
int ffsll(long long i);
#endif

#endif
