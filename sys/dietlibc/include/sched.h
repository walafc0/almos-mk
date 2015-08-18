#ifndef _SCHED_H
#define _SCHED_H 1

#include <time.h>
#include <sys/types.h>

/*
 * Scheduling policies
 */

#define SCHED_RR		0
#define SCHED_FIFO		1
#define SCHED_OTHER		SCHED_RR

struct sched_param {
  int sched_priority;
};

/* END OF COPY form kernel-header */
int sched_setparam(pid_t pid, const struct sched_param* p);
int sched_getparam(pid_t pid, struct sched_param* p);
int sched_getscheduler(pid_t pid);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param* p);
int sched_yield(void);
int sched_get_priority_max(int policy);
int sched_get_priority_min(int policy);
int sched_rr_get_interval(pid_t pid, struct timespec* tp);

#endif	/* _SCHED_H_ */
