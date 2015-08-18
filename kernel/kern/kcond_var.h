/*
 * kern/kcond_var.h - kernel condition variables synchronization
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

#ifndef _KCOND_VAR_H_
#define _KCOND_VAR_H_

#include <types.h>
#include <spinlock.h>
#include <wait_queue.h>

///////////////////////////////////////////////////
///               Public Section                ///
///////////////////////////////////////////////////

/** Kernel Condition Variable Sync Type */
typedef struct wait_queue_s kcv_t;		

/** Initialize CondVar */
#define kcv_init(cv,name)

/** Wait on CondVar until condition completion */
#define kcv_wait(cv,lock)

/** Signal (notifiy) CondVar and Wakeup one waiting thread */
#define kcv_signal(cv)

/** Destroy CondVar */
#define kcv_destroy(cv)

/** Broadcast CondVar and return the number of notified threads */
static inline uint_t kcv_broadcast(kcv_t *cv);


///////////////////////////////////////////////////
///               Private Section               ///
///////////////////////////////////////////////////

#undef kcv_init
#define kcv_init(_cv,_name)				\
	do{						\
		wait_queue_init((_cv), (_name));	\
	}while(0)

#undef kcv_wait
#define kcv_wait(_cv,_lock)				\
	do{						\
		wait_on((_cv), WAIT_LAST);		\
		spinlock_unlock_nosched((_lock));	\
		sched_sleep(current_thread);		\
		spinlock_lock((_lock));			\
	}while(0)

#undef kcv_signal
#define kcv_signal(_cv)					\
	do{						\
		(void)wakeup_one((_cv), WAIT_ANY);	\
	}while(0)

static inline uint_t kcv_broadcast(kcv_t *cv)
{
	return wakeup_all(cv); 
}

#undef kcv_destroy
#define kcv_destroy(_cv)						\
	do{								\
		if(!(wait_queue_isEmpty((_cv))))			\
			printk(ERROR,"ERROR: %s: kcv [%s] is busy\n", __FUNCTION__, (_cv)->name); \
	}while(0)

#endif	/* _KCOND_VAR_H_ */
