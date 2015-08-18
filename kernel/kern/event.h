/*
 * kern/event.h - Per-CPU Events-Manager
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

#ifndef _EVENT_H_
#define _EVENT_H_

#include <types.h>
#include <list.h>

struct cpu_s;

/** Private event flags */
#define EVENT_PENDING     1

//////////////////////////////////////////////////
//              Public Section                  //
//////////////////////////////////////////////////

/** Local Events Priorities */
typedef enum
{
	E_CLK = 0,			/* Fixed Priority */
	E_TLB,
	E_CHR,
	E_BLK,
	E_MIGRATE,
	E_CREATE,
	E_FORK,
	E_EXEC,
	E_FUNC,
	E_PRIO_NR
}event_prio_t;

typedef enum
{
	EL_LOCAL,
	EL_REMOTE
}event_listner_type_t;

/** Opaque event structure */
struct event_s;

/** Opaque events listner structure */
struct event_listner_s;

/** Get CPU of given listner */
#define event_listner_get_cpu(el,name)

/** Event handler type definition */
#define EVENT_HANDLER(n)  sint_t (n)(struct event_s *event)
typedef EVENT_HANDLER(event_handler_t);

/** Set event's priority */
#define event_set_priority(e, _prio)

/** Set event's sender id */
#define event_set_senderId(e,_id)

/** Set event's handler */
#define event_set_handler(e,_handler)

/** Set event handler's argument */
#define event_set_argument(e,_arg)

/** Set handler execution error code */
#define event_set_error(e,_error)

/** Get event's priority */
#define event_get_priority(e)

/** Get event's sender id */
#define event_get_senderId(e)

/** Get event's handler */
#define event_get_handler(e)

/** Get handler's argument */
#define event_get_argument(e)

/** Get handler execution error code */
#define event_get_error(e)

/** True if event manager has a pending event */
#define event_is_pending(el)


/** Initialize event */
error_t event_init(struct event_s *event);


/** Destroy event */
void event_destroy(struct event_s *event);


/** 
 * Send an event to local event listner 
 * must be called with interrupts disabled
 * Doing the same for a remote event listner
 * is not required
 */
void event_send(struct event_s *event, uint_t cpu_gid);


/** 
 * Notify about all recived events,
 * must be called with interrupts disabled
 */
void event_listner_notify(struct event_listner_s *el);


/** Initialize event listner */
error_t event_listner_init(struct event_listner_s *el);


/** Destroy event listner */
void event_listner_destroy(struct event_listner_s *el);

/** Events Manager thread */
void* thread_event_manager(void *arg);

//////////////////////////////////////////////////
//             Private Section                  //
//////////////////////////////////////////////////


/** 
 * Private events list based database 
 **/
struct event_list_s
{
	volatile uint_t count;
	struct list_entry root;
};

/** opaque events listner */
struct event_listner_s
{
	uint_t count;
	volatile uint_t flags;
	volatile uint_t prio CACHELINE;
	struct  event_list_s tbl[E_PRIO_NR];
		
};


/** internal event info structure */
typedef struct 
{
	/* event type */
	uint_t prio;

	/* event handler */
	event_handler_t *handler;

	/* event argument */
	void *arg;

	/* event sender id */
	void *id;
  
	/* event error code */
	error_t err;
}event_info_t;


/** Event opaque structure */
struct event_s
{
	/* Event Primary Info */
	event_info_t e_info;

	/* Event List Chain */
	struct list_entry e_list;

	/* Event Backup Info */
	event_info_t e_bckp;
  
	/* Event Private Data */
	void *data;
};

#define event_backup(e)						\
	do{							\
		(e)->e_bckp.prio    = (e)->e_info.prio;		\
		(e)->e_bckp.handler = (e)->e_info.handler;	\
		(e)->e_bckp.arg     = (e)->e_info.arg;		\
		(e)->e_bckp.id      = (e)->e_info.id;		\
		(e)->e_bckp.err     = (e)->e_info.err;		\
	}while(0)

#define event_restore(e)					\
	do{							\
		(e)->e_info.prio    = (e)->e_bckp.prio;		\
		(e)->e_info.handler = (e)->e_bckp.handler;	\
		(e)->e_info.arg     = (e)->e_bckp.arg;		\
		(e)->e_info.id      = (e)->e_bckp.id;		\
		(e)->e_info.err     = (e)->e_bckp.err;		\
	}while(0)


#undef event_listner_get_cpu
#define event_listner_get_cpu(_el,_name)		\
	list_container((_el), struct cpu_s, _name)

#undef event_set_priority
#define event_set_priority(e, _prio)  do{(e)->e_info.prio = (_prio);}while(0)

#undef event_set_senderId
#define event_set_senderId(e,_id)     do{(e)->e_info.id = (_id);}while(0)

#undef event_set_handler
#define event_set_handler(e,_handler) do{(e)->e_info.handler = (_handler);}while(0)

#undef event_set_argument
#define event_set_argument(e,_arg)    do{(e)->e_info.arg = (_arg);}while(0)

#undef event_set_error
#define event_set_error(e,_error)     do{(e)->e_info.err = (_error);}while(0)

#undef event_get_priority
#define event_get_priority(e)   ((e)->e_info.prio)

#undef event_get_senderId
#define event_get_senderId(e)   ((e)->e_info.id)

#undef event_get_handler
#define event_get_handler(e)    ((e)->e_info.handler)

#undef event_get_argument
#define event_get_argument(e)   ((e)->e_info.arg)

#undef event_get_error
#define event_get_error(e)      ((e)->e_info.err)

#undef event_is_pending
#define event_is_pending(el)    ((el)->flags & EVENT_PENDING)

#endif	/* _EVENT_H_ */
