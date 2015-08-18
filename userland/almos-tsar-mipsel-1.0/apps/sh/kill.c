/*
 * apps/kill.c : kill function in user shell
 * 
 * Copyright (c) 2015 UPMC Sorbonne Universites
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

#include <stdio.h>
#include <signal.h>
#include <sys/syscall.h>

/* ush.h is not in CFLAGS and we can't add it in $ALMOS_USR_TOP/include */
#include "ush.h"

void
usage()
{
       fprintf(stdout, "Usage: kill [PID] [SIG]\n");     
       return;
}

int
kill_func(void* param)
{
        int     err;
        pid_t   pid;
        uint_t  sig;
        ms_args_t *args;

        args = (ms_args_t*) param;
        pid = atoi(args->argv[1]);
        sig = atoi(args->argv[2]);

        if ( sig != SIGKILL && sig != SIGTERM )
        {
                fprintf(stderr, "Wrong signal. Must be SIGTERM (%u) or SIGKILL (%u)\n", \
                                SIGTERM, SIGKILL);
                usage();
                return EXIT_FAILURE;
        }

        cpu_syscall((void*)pid, (void*)sig, NULL, NULL, SYS_KILL);

        return EXIT_SUCCESS;

}
