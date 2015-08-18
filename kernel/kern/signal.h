/*
 * kern/signal.h - signal-management related operations
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS-kernel.
 *
 * ALMOS-kernel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS-kernel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS-kernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <types.h>

#define SIG_DEFAULT    (void*)0L
#define SIG_IGNORE     (void*)1L
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
#define SIG_NR     32      /* signal 0 implied */

#define SIG_DEFAULT_MASK         0xFFEEFFFF
#define SIG_DEFAULT_STACK_SIZE   2048

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

#define SIGNAL_HANDLER(n) void (n) (int sig)

typedef SIGNAL_HANDLER(sa_handler_t);

struct sigaction_s 
{
	sigset_t sa_mask;
	uint32_t sa_flags;
	union
	{ 
		sa_handler_t *sa_handler;
		void (*sa_sigaction)(int, siginfo_t *, void *);
	};
};

struct thread_s;
struct task_locator_s;
struct task_s;

struct sig_mgr_s
{
	sa_handler_t *sigactions[SIG_NR];
	struct thread_s     *handler;
};

int sys_signal(uint_t sig, sa_handler_t* handler);
int sys_sigreturn_setup(void *sigreturn_func);
int sys_kill(pid_t pid, uint_t sig);
error_t signal_manager_init(struct task_s *task);
error_t signal_init(struct thread_s *thread);
error_t signal_rise(pid_t pid, cid_t location, uint_t sig);
void signal_notify(struct thread_s *this);

/* do nothing in this version */
#define signal_manager_destroy(task)

#endif	/* _SIGNAL_H_ */
