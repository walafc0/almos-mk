/* http://threads.hpcl.gwu.edu/sites/npb-upc/wiki */

#ifndef UPC_TIMERS_H
#define UPC_TIMERS_H

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#define NB_TIMERS 64

static double UPC_Wtime();

double upc_timers_start[NB_TIMERS] = {0};
double upc_timers_elapsed[NB_TIMERS] = {0};
int upc_timers_status[NB_TIMERS] = {0};
#define MYTHREAD 0
#define UPC_TIMER_STOPPED 0
#define UPC_TIMER_RUNNING 1

#ifdef UPC_TIMERS_DISABLE
#define TIMER_START(n) { }while(0)
#define TIMER_STOP(n) { }while(0)
#else
#define TIMER_START(n) timer_start(n)
#define TIMER_STOP(n) timer_stop(n)
#endif

#ifdef _ALMOS_
static double UPC_Wtime(){
	return (double) clock();
}
#else  /* _ALMOS_ */
#ifdef USE_MONOTONIC_CLOCK
#include <time.h>
static double UPC_Wtime(){
    struct timespec sampletime;
    double time;

    // TODO: if you have Linux ver >= 2.6.28, you can use CLOCK_MONOTONIC_RAW
    if( clock_gettime( CLOCK_MONOTONIC, &sampletime) != 0 )
        fprintf(stderr, "Warning: Unable to read clock.\n");

    time = sampletime.tv_sec + (sampletime.tv_nsec / 1000000000.);

    return time;
}
#else  /* USE_MONOTONIC_CLOCK */
static double UPC_Wtime(){
    struct timeval sampletime;
    double time;

    gettimeofday( &sampletime, NULL);
    time = sampletime.tv_sec + (sampletime.tv_usec / 1000000.);

    return time;
}
#endif /* USE_MONOTONIC_CLOCK */
#endif /* _ALMOS_ */

double elapsed_time(){
    return UPC_Wtime();
}

void timer_clear(int n){
    if( upc_timers_status[n] == UPC_TIMER_RUNNING )
        fprintf(stderr, "%d: Clearing a running timer(%d)\n", MYTHREAD, n);

    upc_timers_elapsed[n] = 0.0;
}

void timer_start(int n){
    if( upc_timers_status[n] == UPC_TIMER_RUNNING )
        fprintf(stderr, "%d: Starting a running timer(%d)\n", MYTHREAD, n);

    upc_timers_start[n] = UPC_Wtime();
    upc_timers_status[n] = UPC_TIMER_RUNNING;
}

void timer_stop(int n){
    double t;

    if( upc_timers_status[n] != UPC_TIMER_RUNNING )
        fprintf(stderr, "%d: Stopping a non running timer(%d)\n", MYTHREAD, n);

    t = UPC_Wtime()- upc_timers_start[n];
    upc_timers_elapsed[n] += t;
    upc_timers_status[n] = UPC_TIMER_STOPPED;
}

double timer_read(int n){
    if( upc_timers_status[n] == UPC_TIMER_RUNNING )
        fprintf(stderr, "%d: Reading a running timer(%d)\n", MYTHREAD, n);

    return upc_timers_elapsed[n];
}

#endif

