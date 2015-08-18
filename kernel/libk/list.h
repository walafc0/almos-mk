/*
 * list.h - simple and double circular chained lists, inspired from linux
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

#ifndef _ALMOS_LIST_H_
#define _ALMOS_LIST_H_

#include <kmagics.h>

#ifndef ALMOS_LIST_DEBUG
#define ALMOS_LIST_DEBUG    1
#endif

#ifndef NULL
#define NULL (void *) 0
#endif

#ifndef offsetof
#define offsetof(type, member) ((unsigned int) & ((type *)0)->member)
#endif

#ifdef ALMOS_LIST_DEBUG
#define list_assert_dead(_list_ptr)					\
	do{								\
		bassert(((_list_ptr)->next == (void*)LIST_NEXT_DEAD) && (_list_ptr)->pred == (void*)LIST_PRED_DEAD); \
	}while(0)

#define slist_assert_dead(_list_ptr)					\
	do{								\
		bassert((_list_ptr)->next == (void*)LIST_NEXT_DEAD);	\
	}while(0)

#else

#define list_assert_dead(_list_ptr)
#define slist_assert_dead(_list_ptr)
#endif	/* ALMOS_LIST_DEBUG */

#define LIST_ROOT_INITIALIZER(name) { &(name), &(name) }

#define list_container(list_ptr,container_type,member_name)		\
	({const typeof(((container_type*)0)->member_name) *__member_ptr = (list_ptr); \
		(container_type *)( (char*)__member_ptr - offsetof(container_type,member_name));})

/*
 * Double circular linked list definition
 */
struct list_entry{
	struct list_entry *next;
	struct list_entry *pred;
};

/* Simple circular linked list definition
 * it's managed as a stack data structure
 * we still able to use somme functions
 * and macro from list_entry (Double circular
 * linked list) see commentes of each one
 */
struct slist_entry{
	struct slist_entry *next;
};


/*
 * Can be used, and should be used for both
 * list_entry & slist_entry data structures
 */
#define list_element(list_ptr, type, member)	\
	list_container(list_ptr, type, member)

/*
 * To be used with list_entry data structure
 * probebly, it has no sens with slist_entry
 */
#define list_first(root_ptr, type, member)		\
	list_element((root_ptr)->next,type,member)

/*
 * To be used only with list_entry data structure
 */
static inline struct list_entry* list_next(struct list_entry *root, struct list_entry *current)
{
	if((root == root->next) || (current->next == root))
		return NULL;
  
	return current->next;
}

/*
 * To be used only with list_entry data structure
 */
static inline struct list_entry* list_pred(struct list_entry *root, struct list_entry *current)
{
	if((root == root->next) || (current->pred == root))
		return NULL;
  
	return current->pred;
}


/*
 * To be used only with list_entry data structure
 */
#define list_last(root_ptr, type, member)		\
	list_element((root_ptr)->pred,type,member)


/*
 * To be used with list_entry data structure
 */

/* FIXME: this forbid having 2 list_for_each in the same function, because the	*
 * variable __ptr will be defined twice, wich result in a compilation error.	*
 * The __ptr is necessary because some functions delete iter while traversing	*
 * the list.									*
 * Do a separate list_for_each_safe for functions who want to delete element	*
 * while walking a list (see linux lists)					*/

#define list_foreach_forward(root_ptr, iter)				\
struct list_entry *__ptr;						\
for(iter = (root_ptr)->next, __ptr = (struct list_entry *)(iter)->next; \
		iter != (root_ptr);					\
		iter = (typeof((iter)))__ptr, __ptr = (struct list_entry *)iter->next)
	
//for(iter = (root_ptr)->next; iter != (root_ptr);  iter = iter->next)

#if 0
/*
 * To be used with slist_entry data structure
 */
#define slist_foreach(root_ptr, iter)					\
	struct slist_entry *__ptr;					\
	for(iter = (root_ptr)->next, __ptr = (struct slist_entry *)(iter)->next; iter != (root_ptr); iter = (typeof((iter)))__ptr, __ptr = (struct slist_entry *)iter->next)
#endif

/*
 * To be used only with list_entry data structure
 */
#define list_foreach_backward(root_ptr, iter)				\
	struct list_entry *__ptr;					\
	for(iter = (root_ptr)->pred, __ptr = iter->pred; iter != (root_ptr); iter = __ptr, __ptr = iter->pred)

/*
 * Can be used, and should be used for both
 * list_entry & slist_entry data structures
 */
#define list_foreach(root_ptr, iter)		\
	list_foreach_forward(root_ptr,iter)

/*
 * To be used only with list_entry data structure
 */
static inline void list_root_init(struct list_entry *root)
{
	root->next = root;
	root->pred = root;
}

/*
 * To be used only with list_entry data structure
 */
static inline void list_entry_init(struct list_entry *entry)
{
	entry->next = NULL;
	entry->pred = NULL;
}

/*
 * To be used only with list_entry data structure
 */
static inline void list_add(struct list_entry *root, struct list_entry *entry)
{
	struct list_entry *pred_entry;
	struct list_entry *next_entry;
  
	pred_entry = root;
	next_entry = root->next;

	entry->next = next_entry;
	entry->pred = pred_entry;
  
	pred_entry->next = entry;
	next_entry->pred = entry;
}

/*
 * To be used with list_entry data structure
 * probebly, it has no sens with slist_entry
 */
#define list_add_first(root, entry)		\
	list_add((root),(entry))

/*
 * To be used with list_entry data structure
 * probebly, it has no sens with slist_entry
 */
#define list_add_next(root, entry)		\
	list_add((root),(entry))

/*
 * To be used only with list_entry data structure
 */
#define list_add_last(root, entry)		\
	list_add((root)->pred,(entry))

/*
 * To be used only with list_entry data structure
 */
#define list_add_pred(root, entry)		\
	list_add((root)->pred,(entry))

/*
 * Can be used, and should be used for both
 * list_entry & slist_entry data structures
 */
#define list_empty(root) (root == (root)->next)

/*
 * To be used only with list_entry data structure
 */
static inline void list_unlink(struct list_entry *entry)
{
	struct list_entry *pred_entry;
	struct list_entry *next_entry;

	pred_entry = entry->pred;
	next_entry = entry->next;

	pred_entry->next = entry->next;
	next_entry->pred = entry->pred;

#if ALMOS_LIST_DEBUG
	entry->next = (struct list_entry *)LIST_NEXT_DEAD;
	entry->pred = (struct list_entry *)LIST_PRED_DEAD;
#endif
}

/*
 * To be used only with list_entry data structure
 */
static inline void list_replace(struct list_entry *current, struct list_entry *new)
{
	struct list_entry *pred_entry;
	struct list_entry *next_entry;

	pred_entry = current->pred;
	next_entry = current->next;

	new->next = next_entry;
	new->pred = pred_entry;

	pred_entry->next = new;
	next_entry->pred = new;
}

///////////////////////////////////////////////////
/* Simple circular linked list functions & macro */
///////////////////////////////////////////////////

#define slist_root_init(_root)			\
	(_root)->next = (_root)


#define slist_entry_init(_entry)		\
	(_entry)->next = NULL


#define list_head(_root, _type, _member)		\
	list_element((_root)->next,_type,_member)


#define list_push(_root,_entry)			\
	do{					\
		(_entry)->next = (_root)->next;	\
		(_root)->next = _entry;		\
	}while(0)


/* list must have at least one element */
static inline struct slist_entry* list_pop(struct slist_entry *root)
{
	struct slist_entry *entry;
  
	entry = root->next;
	root->next = entry->next;

#if ALMOS_LIST_DEBUG
	entry->next = (struct slist_entry *)LIST_SNEXT_DEAD;
#endif

	return entry;
}

#endif	/* _ALMOS_LIST_H_ */
