/*
 * sys_sbrk: increase the limit of process's heap
 * 
 * Copyright (c) 2012 UPMC Sorbonne Universites
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
#include <thread.h>
#include <task.h>
#include <vmm.h>

int sys_sbrk(uint_t current_heap_ptr, uint_t size)
{
	struct task_s *task;

	if(size == 0)
		return CONFIG_TASK_HEAP_MIN_SIZE;

	size = size * PMM_PAGE_SIZE;
	task = current_task;

	return vmm_sbrk(&task->vmm, current_heap_ptr, size);
}
