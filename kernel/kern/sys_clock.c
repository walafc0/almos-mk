/*
 * sys_clock: get system current time
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
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

#include <errno.h>
#include <config.h>
#include <cpu.h>
#include <thread.h>

int sys_clock(uint64_t *val)
{
	error_t err;
	uint64_t cycles;

	if((val == NULL) || NOT_IN_USPACE((uint_t)val))
	{
		err = EINVAL;
		goto fail_inval;
	}

	cycles = cpu_get_cycles(current_cpu);
	err    = cpu_copy_to_uspace(val, &cycles, sizeof(cycles));

fail_inval:
	current_thread->info.errno = err;
	return err; 
}
