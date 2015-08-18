/*
 * kern/radix.h - interface for a dynamic radix tree
 * 
 * Copyright (c) 2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2010, Fabrice de Gans-Riberi <fabrice.degans@gmail.com>
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

#ifndef RADIX_H_
#define RADIX_H_

#include <types.h>
#include <kmem.h>

struct radix_node_s;

struct radix_s {
	struct radix_node_s*	root_node;
	int			height;
	uint32_t		max_key;
};
typedef struct radix_s radix_t;

/* semi-opaque structure */
struct radix_item_info_s 
{
	/* Public */
	void *item;
  
	/* Private */
	struct radix_node_s *node;
	uint_t key;
	uint_t index;
	uint_t height;
};

typedef struct radix_item_info_s radix_item_info_t;

typedef enum
{
	TAG_PG_DIRTY = 0,
	NB_TAGS
} radix_tags_t;

typedef enum
{
	RADIX_INFO_SET,
	RADIX_INFO_DELETE,
	RADIX_INFO_INSERT
} radix_info_op_t;


/**
 * Initializes a radix tree.
 *
 * @root	radix root
 */
void
radix_tree_init(struct radix_s*	root);


/**
 * Initializes the kmem cache for the radix node structure.
 */
KMEM_OBJATTR_INIT(radix_node_kmem_init);


/**
 * Apply desired operation on radix item information.
 * Nota : item informations depand on tree topologie
 *        Insert/Delete operations caused by other
 *        radix functions ivalidat the item info.
 *
 * @root        radix root
 * @info        item information
 * @operation   one of enum operations (RADIX_INFO_DELELTE, ...etc).
 * @new_item    new item to be set/inserted up on operation
 * @return      Error code
 */
error_t radix_item_info_apply(struct radix_s *root, 
			      radix_item_info_t *info, 
			      radix_info_op_t operation,
			      void *new_item);

/**
 * Find item information by key.
 *
 * @root        radix root
 * @key         item's key
 * @info        item's info to be filled
 * @return      true if found otherise false.
 */
bool_t radix_item_info_lookup(struct radix_s *root, 
			      uint_t key, 
			      radix_item_info_t *info);

/**
 * Inserts a new item in the tree.
 *
 * @root	radix root
 * @key		key of the new item
 * @item	item to be inserted
 * @return	error code
 */
int
radix_tree_insert(struct radix_s*	root,
		  unsigned long		key,
		  void*			item);

/**
 * Removes an item from the tree.
 *
 * @root	radix root
 * @key		key of the item to be removed
 * @return	removed item (NULL if item not found)
 */
void*
radix_tree_delete(struct radix_s*	root,
		  unsigned long		key);

/**
 * Looks up an item in the tree.
 *
 * @root	radix root
 * @key		key of the item to be looked up
 * @return	item looked up (NULL if item not found)
 */
void* radix_tree_lookup(struct radix_s*	root, unsigned long key);


/**
 * Looks up up to @max_items items in the tree.
 * Will always look up for items even if the item
 * at @first_index is not present.
 *
 * @root	radix root
 * @results	results
 * @first_index	key of the first item to be looked up
 * @max_items	max items to look up for
 * @return	number of items actually in @results
 */
unsigned int
radix_tree_gang_lookup(struct radix_s*	root,
		       void**		results,
		       unsigned long	first_index,
		       unsigned int	max_items);

/**
 * Sets a tag for an item.
 *
 * @root	radix root
 * @index	key of the item to be looked up
 * @tag		index of the tag to be set
 *		(less than NB_TAGS)
 * @return	item whose tag has been set
 */
void*
radix_tree_tag_set(struct radix_s*	root,
		   unsigned long	index,
		   unsigned int		tag);

/**
 * Clears a tag for an item.
 *
 * @root	radix root
 * @index	key of the item to be looked up
 * @tag		index of the tag to be cleared
 *		(less than NB_TAGS)
 * @return	item whose tag has been cleared
 */
void*
radix_tree_tag_clear(struct radix_s*	root,
		     unsigned long	index,
		     unsigned int	tag);

/**
 * Gets a tag from an item.
 *
 * @root	radix root
 * @index	key of the item to be looked up
 * @tag		index of the tag to be looked up
 *		(less than NB_TAGS)
 * @return	tag value (0 or 1)
 *		negative value if error
 */
int
radix_tree_tag_get(struct radix_s*	root,
		   unsigned long	index,
		   unsigned int		tag);

/**
 * Looks up up to @max_items items in the tree.
 * Only looks up items with @tag set.
 * Will always look up for items even if the item
 * at @first_index is not present.
 *
 * @root	radix root
 * @results	results
 * @first_index	key of the first item to be looked up
 * @max_items	max items to look up for
 * @tag		index of the tag to be looked up
 *		(less than NB_TAGS)
 * @return	number of items actually in @results
 */
unsigned int
radix_tree_gang_lookup_tag(struct radix_s*	root,
			   void**		results,
			   unsigned long	first_index,
			   unsigned int		max_items,
			   unsigned int		tag);

/**
 * Looks up up to @max_items items in the tree,
 * returns pointers on items.
 * Only looks up items with @tag set.
 * Will always look up for items even if the item
 * at @first_index is not present.
 *
 * @root	radix root
 * @results	results
 * @first_index	key of the first item to be looked up
 * @max_items	max items to look up for
 * @tag		index of the tag to be looked up
 *		(less than NB_TAGS)
 * @return	number of items actually in @results
 */
unsigned int
radix_tree_gang_lookup_tag_slot(struct radix_s*	root,
				void***		results,
				unsigned long	first_index,
				unsigned int	max_items,
				unsigned int	tag);

/**
 * Tests if at least one item in the tree has @tag set.
 *
 * @root	radix root
 * @tag		index of the tag to be looked up
 *		(less than NB_TAGS)
 * @return	result of the test
 *		negative value if error
 */
int
radix_tree_tagged(struct radix_s*	root,
		  unsigned int		tag);

#endif /* RADIX_H_ */
