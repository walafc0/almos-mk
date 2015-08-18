#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stdint.h>
#include <sys/types.h>

/* START COPY FROM KERNEL */
#define SIG_DEFAULT    0L
#define SIG_IGNORE     1L
#define SIG_ERROR     -1L

#define SIGHUP     1       /* hangup */
#define SIGINT     2       /* interrupt */
#define SIGQUIT    3       /* quit */
#define SIGILL     4       /* illegal instruction (not reset when caught) */
#define SIGTRAP    5       /* trace trap (not reset when caught) */
#define SIGIOT     6       /* IOT instruction */
#define SIGABRT    6       /* used by abort, replace SIGIOT in the future */
#define SIGEMT     7       /* EMT instruction */
#define SIGFPE     8       /* floating point exception */
#define SIGKILL    9       /* kill (cannot be caught or ignored) */
#define SIGBUS     10      /* bus error */
#define SIGSEGV    11      /* segmentation violation */
#define SIGSYS     12      /* bad argument to system call */
#define SIGPIPE    13      /* write on a pipe with no one to read it */
#define SIGALRM    14      /* alarm clock */
#define SIGTERM    15      /* software termination signal from kill */
#define SIGURG     16      /* urgent condition on IO channel */
#define SIGSTOP    17      /* sendable stop signal not from tty */
#define SIGTSTP    18      /* stop signal from tty */
#define SIGCONT    19      /* continue a stopped process */
#define SIGCHLD    20      /* to parent on child stop or exit */
#define SIGCLD     20      /* System V name for SIGCHLD */
#define SIGTTIN    21      /* to readers pgrp upon background tty read */
#define SIGTTOU    22      /* like TTIN for output if (tp->t_local&LTOSTOP) */
#define SIGIO      23      /* input/output possible signal */
#define SIGPOLL    SIGIO   /* System V name for SIGIO */
#define SIGXCPU    24      /* exceeded CPU time limit */
#define SIGXFSZ    25      /* exceeded file size limit */
#define SIGVTALRM  26      /* virtual time alarm */
#define SIGPROF    27      /* profiling time alarm */
#define SIGWINCH   28      /* window changed */
#define SIGLOST    29      /* resource lost (eg, record-lock lost) */
#define SIGUSR1    30      /* user defined signal 1 */
#define SIGUSR2    31      /* user defined signal 2 */
#define NSIG       32      /* signal 0 implied */

typedef uint32_t sigval_t;
typedef uint32_t sigset_t;

typedef struct siginfo_s
{
  int      si_signo;    /* Signal number */
  int      si_errno;    /* An errno value */
  int      si_code;     /* Signal code */
  pid_t    si_pid;      /* Sending process ID */
  uid_t    si_uid;      /* Real user ID of sending process */
  int      si_status;   /* Exit value or signal */
  clock_t  si_utime;    /* User time consumed */
  clock_t  si_stime;    /* System time consumed */
  sigval_t si_value;    /* Signal value */
  int      si_int;      /* POSIX.1b signal */
  void    *si_ptr;      /* POSIX.1b signal */
  void    *si_addr;     /* Memory location which caused fault */
  int      si_band;     /* Band event */
  int      si_fd;       /* File descriptor */
}siginfo_t;

struct sigaction_s 
{
  sigset_t sa_mask;
  uint32_t sa_flags;
  union
  { 
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
  };
};
/* END COPY FROM KERNEL */

typedef void (*__sighandler_t)(int);

#ifdef _BSD_SOURCE
typedef sighandler_t sig_t;
#endif

#ifdef _GNU_SOURCE
typedef __sighandler_t sighandler_t;
#endif

#define SIG_DFL ((void*)SIG_DEFAULT)	/* default signal handling  */
#define SIG_IGN ((void*)SIG_IGNORE)	/* ignore signal            */
#define SIG_ERR ((void*)SIG_ERROR)	/* error return from signal */

void* signal(int sig, void (*func)(int));

int kill(pid_t pid, int sig);
int raise(int sig);

#endif	/* _SIGNAL_H_ */
