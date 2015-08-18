/*
 * sys_ftime: get current time
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

#include <cpu.h>
#include <thread.h>
#include <time.h>

int sys_ftime(struct timeb *utime)
{
	error_t err;
	uint_t tm_now;
	struct timeb time;

	if((utime == NULL) || NOT_IN_USPACE((uint_t)utime))
	{
		err = EINVAL;
		goto fail_inval;
	}

	tm_now = cpu_get_ticks(current_cpu) * MSEC_PER_TICK;
	printk(INFO, "%s: the time now is %d\n", __FUNCTION__, tm_now);
	time.time = tm_now/1000;
	time.millitm = tm_now%1000;
	time.timezone = 120;//GMT+2
	time.time = 1;

	err    = cpu_copy_to_uspace(utime, &time, sizeof(time));

fail_inval:
	current_thread->info.errno = err;
	return err; 
}
