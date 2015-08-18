/*
 * kern/cpu-trace.h - Per-CPU, In-core trace system
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

#ifndef _CPU_TRACE_H_
#define _CPU_TRACE_H_

#include <config.h>
#include <types.h>
#include <kdmsg.h>

/** Opaque CPU trace recorder */
struct cpu_trace_recorder_s;

/** Write CPU trace information to current CPU recorder */
#define cpu_trace_write(_cpu,func)

/** Dump CPU trace log to specified tty */
static inline void cpu_trace_dump(struct cpu_s *cpu);


///////////////////////////////////////////////////////////////////////
//                         Private Section                           //
///////////////////////////////////////////////////////////////////////

#define CPU_TRACE_LOG_LENGTH      100

/** Private trace info structure */
typedef struct cpu_trace_info_s
{
	uint_t time_stamp;
	void *func_addr;
	uint_t stack_ptr;
	void *thread_ptr;
	uint_t line;
}cpu_trace_info_t;

/** Opaque CPU trace recorder */
struct cpu_trace_recorder_s
{
	uint_t current_index;
	cpu_trace_info_t log[CPU_TRACE_LOG_LENGTH];
};


#if CONFIG_CPU_TRACE
#undef  cpu_trace_write
#define cpu_trace_write(_cpu,func)					\
	do{{								\
			uint_t state;					\
			cpu_disable_all_irq(&state);			\
			cpu_trace_info_t *info = &(_cpu)->trace_recorder->log[(_cpu)->trace_recorder->current_index]; \
			info->time_stamp = cpu_time_stamp();		\
			info->func_addr  = (func);			\
			info->stack_ptr  = cpu_get_stack();		\
			info->thread_ptr = cpu_current_thread();	\
			(_cpu)->trace_recorder->current_index = ((_cpu)->trace_recorder->current_index  + 1) % CPU_TRACE_LOG_LENGTH; \
			cpu_restore_irq(state);				\
		}}while(0)

static inline void cpu_trace_dump(struct cpu_s *cpu)
{
	register uint_t i;
	register cpu_trace_info_t *info;
  
	except_dmsg("------------------------------------------\n");
	except_dmsg("TRACE DUMP FOR CPU %d\n", cpu->gid);

	for(i=0; i < CPU_TRACE_LOG_LENGTH; i++)
	{
		info = &cpu->trace_recorder->log[i];
		except_dmsg("Entry %d\n", i);
		except_dmsg("\tTimeStamp %d\n\tFunction: %x\n\tLine %d\n\tStack %x\n\tThread %x\n",
			    info->time_stamp, info->func_addr, info->line, info->stack_ptr, info->thread_ptr);
	}
	except_dmsg("---> Current_index: %d\n", cpu->trace_recorder->current_index);
}

#else

#define cpu_trace_write(_cpu,func)

static inline void cpu_trace_dump(struct cpu_s *cpu)
{
	/* Nothing to do */
}

#endif	/* CONFIG_CPU_TRACE */

#endif	/* _CPU_TRACE_H_ */
