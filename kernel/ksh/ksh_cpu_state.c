/*
   This file is part of MutekP.
  
   MutekP is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   MutekP is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with MutekP; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2008
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <config.h>
#include <kminiShell.h>
#include <list.h>
#include <thread.h>
#include <task.h>
#include <cluster.h>
#include <cpu.h>
#include <spinlock.h>
#include <system.h>
#include <wait_queue.h>

//FIXME do it for all cluster
error_t ksh_cpu_state_func(void *param)
{
	register uint_t cld, cpu;
	uint_t cpu_nr;
	ms_args_t args;

#if 0
	uint_t clstr_nr;
	clstr_nr = arch_onln_cluster_nr();
 
	for(cld=0; cld < clstr_nr; cld ++)
	{
		ksh_print("/SYS/CLUSTER%d\n", cld);
		cpu_nr = clusters_tbl[cld].cluster->cpu_nr;

		for(cpu=0; cpu < cpu_nr; cpu++)
		{
			args.argc = 2;
			sprintk(args.argv[1], "/SYS/CLUSTER%d/CPU%d", cld,cpu);
			cat_func(&args);
		}
	}
#endif
	cld = current_cid;
	cpu_nr = current_cluster->cpu_nr;
	ksh_print("/SYS/CLUSTER%d\n", current_cid);

	for(cpu=0; cpu < cpu_nr; cpu++)
	{
		args.argc = 2;
		sprintk(args.argv[1], "/SYS/CLUSTER%d/CPU%d", cld, cpu);
		cat_func(&args);
	}

	return 0;
}
