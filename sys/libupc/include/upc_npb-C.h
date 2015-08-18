/* NAS Parallel Benchmarks 2.3 UPC Version */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(__CYGWIN__) && defined(__RPCNDR_H__)
/* bug 2419 : workaround a typedef of bool in the cygwin headers */
#define boolean int
#else
typedef int boolean;
#endif

typedef struct {
    double real;
    double imag;
} dcomplex;

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#define MEM_OK(var) {                                         \
    if( var == NULL )                                           \
    {                                                         \
        printf("TH%02d: ERROR: %s == NULL\n", MYTHREAD, #var ); \
        upc_global_exit(1);                                     \
    } }

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#define	pow2(a) ((a)*(a))

#define get_real(c) c.real
#define get_imag(c) c.imag
#define cadd(c,a,b) (c.real = a.real + b.real, c.imag = a.imag + b.imag)
#define csub(c,a,b) (c.real = a.real - b.real, c.imag = a.imag - b.imag)
#define cmul(c,a,b) (c.real = a.real * b.real - a.imag * b.imag, \
        c.imag = a.real * b.imag + a.imag * b.real)
#define crmul(c,a,b) (c.real = a.real * b, c.imag = a.imag * b)

extern double randlc(double *, double);
extern void vranlc(int, double *, double, double *);

extern void c_print_results(const char *name, char class, int n1, int n2,
        int n3, int niter, int nthreads, double t,
        double mops, const char *optype, int passed_verification,
        const char *npbversion, const char *compiletime, const char *cc,
        const char *clink, const char *c_lib, const char *c_inc,
        const char *cflags, const char *clinkflags, const char *rand);
