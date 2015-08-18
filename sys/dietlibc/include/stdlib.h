#ifndef _STDLIB_H
#define _STDLIB_H


#include <sys/types.h>
#include <wchar.h>
//#include <alloca.h>

#define alloca(x) __builtin_alloca(x)

void *calloc(size_t nmemb, size_t size);
void *malloc(size_t size);
void* realloc(void *ptr, size_t size);
void* valloc(size_t size);
void free(void *ptr);

int atexit(void (*function)(void));
long int strtol(const char *nptr, char **endptr, int base);
unsigned long int strtoul(const char *nptr, char **endptr, int base);
double strtod(const char* s, char** endptr);

extern int __ltostr(char *s, unsigned int size, unsigned long i, unsigned int base, int UpCase);
extern int __dtostr(double d,char *buf,unsigned int maxlen,unsigned int prec,unsigned int prec2,int g);

#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L
long long int strtoll(const char *nptr, char **endptr, int base);
unsigned long long int strtoull(const char *nptr, char **endptr, int base);
int __lltostr(char *s, unsigned int size, unsigned long long i, unsigned int base, int UpCase);
#endif

int atoi(const char *nptr);
long int atol(const char *nptr);
long long int atoll(const char *nptr);
#define atof(_x) strtod((_x), NULL)

void exit(int status);
void abort(void);

extern int rand(void);
extern int rand_r(unsigned int *seed);
extern void srand(unsigned int seed);

extern char **environ;
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int putenv(const char *string);

typedef unsigned short randbuf[3];
long lrand48(void);
long mrand48(void);
double drand48(void);
void srand48(long seed);
unsigned short *seed48(randbuf buf);
void lcong48(unsigned short param[7]);
long jrand48(randbuf buf);
long nrand48(randbuf buf);
double frand48(randbuf buf);

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

int mkstemp(char *_template);

int abs(int i) __attribute__((__const__));
long int labs(long int i) __attribute__((__const__));
long long int llabs(long long int i) __attribute__((__const__));


#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define RAND_MAX 	0x7ffffffe

#define MB_CUR_MAX 1

/* now these functions are the greatest bullshit I have ever seen.
 * The ISO people must be out of their minds. */

typedef struct { int quot,rem; } div_t;
typedef struct { long quot,rem; } ldiv_t;

div_t div(int numerator, int denominator);
ldiv_t ldiv(long numerator, long denominator);

#ifdef _GNU_SOURCE
typedef struct { long long quot,rem; } lldiv_t;
lldiv_t lldiv(long long numerator, long long denominator);

int clearenv(void);
#endif

int mbtowc(wchar_t *pwc, const char *s, size_t n);
int wctomb(char *s, wchar_t wc);
size_t mbstowcs(wchar_t *dest, const char *src, size_t n);
int mblen(const char* s,size_t n);

#endif
