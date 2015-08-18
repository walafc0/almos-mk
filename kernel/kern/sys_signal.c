/*
 * sys_sem.c: interface to access signal service
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

#include <types.h>
#include <errno.h>
#include <thread.h>
#include <task.h>
#include <pid.h>
#include <signal.h>


int sys_signal(uint_t sig, sa_handler_t *handler)
{  
	register struct thread_s *this;
	int retval;

	this = current_thread;

	if((sig == 0) || (sig >= SIG_NR) || (sig == SIGKILL) || (sig == SIGSTOP))
	{
		this->info.errno = EINVAL;
		return SIG_ERROR;
	}

	retval = (int) this->task->sig_mgr.sigactions[sig];
	this->task->sig_mgr.sigactions[sig] = handler;

	sig_dmsg(1, "%s: handler @%x has been registred for signal %d\n",
	       __FUNCTION__, 
	       handler, 
	       sig);

	return retval;
}


int sys_sigreturn_setup(void *sigreturn_func)
{
	struct thread_s *this;

	this = current_thread;
	this->info.attr.sigreturn_func = sigreturn_func;
	cpu_context_set_sigreturn(&this->pws, sigreturn_func);
	return 0;
}


int sys_kill(pid_t pid, uint_t sig)
{
        cid_t   location;
	error_t err;

	if((sig == 0) || (sig >= SIG_NR))
	{
		err = EINVAL;
	}
        else
        {
                if ( PID_GET_CLUSTER(pid) == current_cid )
                        location = current_cid;
                else
                        location = task_whereis(pid);

                err = signal_rise(pid, location, sig);

                /* No error, return 0 */
                if (!err)
                        return 0;
        }

        /* Error. Set errno and return */
	current_thread->info.errno = err;
	return -1;
}
