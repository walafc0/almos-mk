/*
 * kern/mcs_sync.c - ticket-based barriers and locks synchronization
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

#include <config.h>
#include <types.h>
#include <mcs_sync.h>
#include <thread.h>
#include <cpu.h>
#include <kdmsg.h>

#define mcs_barrier_flush(_ptr)						\
	do{								\
		cpu_invalid_dcache_line(&(_ptr)->phase);		\
		cpu_invalid_dcache_line(&(_ptr)->ticket);		\
		cpu_invalid_dcache_line(&(_ptr)->ticket2);		\
		cpu_invalid_dcache_line(&(_ptr)->cntr);			\
	}while(0);

void mcs_barrier_init(mcs_barrier_t *ptr, char *name, uint_t count)
{
	ptr->val.value     = count;
	ptr->phase.value   = 0;
	ptr->cntr.value    = count;
	ptr->ticket.value  = 0;
	ptr->ticket2.value = 0;
	ptr->name          = name;
	
	cpu_wbflush();
	cpu_invalid_dcache_line(&ptr->val);
	mcs_barrier_flush(ptr);
}


void mcs_barrier_wait(mcs_barrier_t *ptr)
{
	register uint_t phase;
	register uint_t order;
	uint_t *current;
	uint_t *next;
 
	phase   = ptr->phase.value;
	current = (phase == 0) ? &ptr->ticket.value : &ptr->ticket2.value;
	order   = cpu_atomic_add((void*)&ptr->cntr.value, -1);

	if(order == 1)
	{
		phase            = ~(phase) & 0x1;
		next             =  (phase == 0) ? &ptr->ticket.value : &ptr->ticket2.value;
		ptr->phase.value = phase;
		ptr->cntr.value  = ptr->val.value;
		*next            = 0;
		*current         = 1;
		cpu_wbflush();
		mcs_barrier_flush(ptr);
		return;
	}

	mcs_barrier_flush(ptr);

	while(cpu_load_word(current) == 0)
		;

	cpu_invalid_dcache_line(current);
}


void mcs_lock_init(mcs_lock_t *ptr, char *name)
{
	ptr->cntr.value   = 0;
	ptr->ticket.value = 0;
	ptr->name         = name;
	
	cpu_wbflush();
	cpu_invalid_dcache_line(&ptr->cntr);
	cpu_invalid_dcache_line(&ptr->ticket);
}


void mcs_lock(mcs_lock_t *ptr, uint_t *irq_state)
{
	uint_t ticket;

	cpu_disable_all_irq(irq_state);

	ticket = cpu_atomic_add(&ptr->ticket.value, 1);

	while(ticket != cpu_load_word(&ptr->cntr.value))
		;

	current_thread->locks_count ++;
}

void mcs_unlock(mcs_lock_t *ptr, uint_t irq_state)
{
	register uint_t next;
	volatile uint_t *val_ptr;

	val_ptr  = &ptr->cntr.value;
	next     = ptr->cntr.value + 1;

	cpu_wbflush();
	cpu_invalid_dcache_line((void*)val_ptr);

	*val_ptr = next;

	cpu_wbflush();
	cpu_invalid_dcache_line((void*)val_ptr);

	assert(current_thread->locks_count > 0);
	current_thread->locks_count --;
	cpu_restore_irq(irq_state);
}

void mcs_lock_remote(mcs_lock_t *ptr, cid_t cid, uint_t *irq_state)
{
	uint_t ticket;

	cpu_disable_all_irq(irq_state);

	ticket = remote_atomic_add(&ptr->ticket.value, cid, 1);

	while(ticket != remote_lw(&ptr->cntr.value, cid))
		;

	current_thread->distlocks_count ++;
}

//FIXME: why all these dcache_invalidate
void mcs_unlock_remote(mcs_lock_t *ptr, cid_t cid, uint_t irq_state)
{
	register uint_t next;
	volatile uint_t *val_ptr;

	val_ptr  = &ptr->cntr.value;
	next     = remote_lw((void*)val_ptr, cid) + 1;

	cpu_wbflush();
	cpu_invalid_dcache_line((void*)val_ptr);

	remote_sw((void*)val_ptr, cid, next);

	cpu_wbflush();
	cpu_invalid_dcache_line((void*)val_ptr);

	assert(current_thread->distlocks_count > 0);
	current_thread->distlocks_count --;
	cpu_restore_irq(irq_state);
}
