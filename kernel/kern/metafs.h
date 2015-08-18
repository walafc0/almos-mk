/*
 * kern/metafs.h - Meta File-System manages parent-child accesses
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

#ifndef _METAFS_H_
#define _METAFS_H_

#include <types.h>
#include <list.h>
#include <libk.h>
#include <config.h>

///////////////////////////////////
//       Public Section          //
///////////////////////////////////

/** Type of metafs node */
struct metafs_s;

/** Type of children iterator */
struct metafs_iter_s;

/** Initialize metafs node */
static inline void metafs_init(struct metafs_s *node, char *name);

/** Initialize metafs children iterator */
static inline void metafs_iter_init(struct metafs_s *node, struct metafs_iter_s *iter);

/** Register child into it's parent children list */
static inline void metafs_register(struct metafs_s *parent, struct metafs_s *child);

/** Lookup a child by its name */
static inline struct metafs_s* metafs_lookup(struct metafs_s *parent, char *name);

/** Lookup next child belonging to parent */
static inline struct metafs_s* metafs_lookup_next(struct metafs_s *parent, 
						  struct metafs_iter_s *iter);

/** Get a pointer to node's name */
#define metafs_get_name(_node)

/** Set node's name to one pointed by _ptr */
#define metafs_set_name(_node, _ptr)

/** Get children number */
#define metafs_get_children_nr(_parent)

/** Test if node has at least one child */
#define metafs_hasChild(_parent)

/** Unregister a child from it's parent list */
#define metafs_unregister(_parent,_child)

/** Get a pointer to metafs node's container */
#define metafs_container(_node, _type, _member)


///////////////////////////////////
//      Private Section          //
///////////////////////////////////

#define METAFS_HASH_SIZE    10

struct metafs_s
{
	char *name;
	uint_t children_nr;
	struct list_entry list;
	struct list_entry hash_tbl[METAFS_HASH_SIZE];
};

struct metafs_iter_s
{
	uint_t hash_index;
	struct metafs_s *current;
};

static inline uint_t metafs_hash_index(char *dev_name)
{
	register char *str = dev_name;
	register uint_t index = 0;
  
	while(*str)
		index = index + (*(str++) ^ index);

	return index % METAFS_HASH_SIZE;
}

static inline void metafs_init(struct metafs_s *node, char *name)
{
	register uint_t i;
  
	node->name = name;
	node->children_nr = 0;

	for(i=0; i < METAFS_HASH_SIZE; i++)
		list_root_init(&node->hash_tbl[i]);  
}

static inline void metafs_iter_init(struct metafs_s *node, struct metafs_iter_s *iter)
{
	register uint_t i;

	for(i=0; i < METAFS_HASH_SIZE; i++)
	{
		if(!(list_empty(&node->hash_tbl[i])))
		{
			iter->hash_index = i;
			iter->current = list_first(&node->hash_tbl[i], struct metafs_s, list);
			return;
		}
	}

	iter->current = NULL;
}

static inline void metafs_register(struct metafs_s *parent, struct metafs_s *child)
{
	register uint_t index;
  
	parent->children_nr ++;
	index = metafs_hash_index(child->name);
	list_add_last(&parent->hash_tbl[index], &child->list);
}

static inline struct metafs_s* metafs_lookup(struct metafs_s *parent, char *name)
{
	register struct list_entry *iter;
	register struct metafs_s *current;
	register uint_t index;

	index = metafs_hash_index(name);

	list_foreach(&parent->hash_tbl[index], iter)
	{
		current = list_element(iter, struct metafs_s, list);
		if(!strcmp(current->name, name))
			return current;
	}
	return NULL;
}

static inline struct metafs_s* metafs_lookup_next(struct metafs_s *parent, struct metafs_iter_s *iter)
{
	register struct list_entry *next;
	register struct metafs_s *child;
	register uint_t i;

	if(iter->current == NULL)
		return NULL;

	child = iter->current;
	i = iter->hash_index;

	if((next = list_next(&parent->hash_tbl[i], &iter->current->list)) == NULL)
	{ 
		for(i = i + 1; i < METAFS_HASH_SIZE; i++)
		{
			if(!(list_empty(&parent->hash_tbl[i])))
			{
				iter->hash_index = i;
				iter->current = list_first(&parent->hash_tbl[i], struct metafs_s, list);
				return child;
			}
		}
		iter->current = NULL;
		return child;
	}
   
	iter->current = list_element(next, struct metafs_s, list);
	return child;
}

#undef metafs_get_name
#define metafs_get_name(_node) ((_node)->name)

#undef metafs_set_name
#define metafs_set_name(_node, _ptr) do{(_node)->name = (_ptr);}while(0)

#undef metafs_get_children_nr
#define metafs_get_children_nr(_parent) ((_parent)->children_nr)

#undef metafs_hasChild
#define metafs_hasChild(_parent) ((_parent)->children_nr != 0)

#undef metafs_unregister
#define metafs_unregister(_parent,_child)	\
	do					\
	{					\
		list_unlink(&(_child)->list);	\
		(_parent)->children_nr --;	\
	}while(0)

#undef metafs_container
#define metafs_container(_metafs, _type, _member)	\
	list_container(_metafs, _type, _member)


#if CONFIG_METAFS_DEBUG
#include <kdmsg.h>

static inline void metafs_print(struct metafs_s *node)
{
	uint_t i;
	struct list_entry *iter;
  
	printk(DEBUG, "metafs_print [%s]\n", node->name);
  
	for(i=0; i<METAFS_HASH_SIZE; i++)
	{
		printk(DEBUG, "Entry %d: ", i);
		list_foreach(&node->hash_tbl[i], iter)
		{
			printk(DEBUG, "%s, ", list_element(iter, struct metafs_s, list)->name);
		}
		printk(DEBUG, "\n");
	}
}
#else
static inline void metafs_print(struct metafs_s *node)
{
}
#endif	/* CONFIG_METAFS_DEBUG */

#endif	/* _METAFS_H_ */
