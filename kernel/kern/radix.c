/*
 * kern/radix.c - a dynamic radix tree implementation
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

#include <errno.h>
#include <radix.h>
#include <bits.h>
#include <kdmsg.h>
#include <kmem.h>
#include <libk.h>

/*
  TODO Move this error code to errno.h
*/
#define ENOTFOUND 42

#if 0
#include <mapper.h>
#define radix_dmsg(x,msg, ...)				\
	do{						\
		if((uint_t)(x) == 0x10ff34c0)		\
			printk(DEBUG,msg, __VA_ARGS__);	\
	}while(0)

#define radix_container(_radix) (list_container(_radix, struct address_space_s, a_page_tree))
#else
#define radix_dmsg(x,msg, ...)
#define radix_container(_radix)
#endif

#define TAG_GET(node, tag_id, index)   (node->tags[tag_id] & (1LLU << index))
#define TAG_SET(node, tag_id, index)   node->tags[tag_id] = node->tags[tag_id] | (1LLU << index)
#define TAG_CLEAR(node, tag_id, index) node->tags[tag_id] = node->tags[tag_id] & ~(1LLU << index)

#define NEXT_CHILD(root, current_height, key)				\
	( ( 0x3F << ((root->height-(current_height+1))*6) ) & key ) >> ((root->height-(current_height+1))*6)
#define MAX_KEY_ID(root) \
	( (root->height == 6) ? 0xFFFFFFFF : (unsigned long) ((1 << (root->height*6)) - 1) )

struct radix_node_s 
{
	void*			children[64];
	uint64_t		tags[NB_TAGS];
	BITMAP_DECLARE(bitmap,8);
	struct radix_node_s*	parent;
	uint_t		count;
};


KMEM_OBJATTR_INIT(radix_node_kmem_init)
{
	attr->type = KMEM_RADIX_NODE;
	attr->name = "KCM Radix Node";
	attr->size = sizeof(struct radix_node_s);
	attr->aligne = 0;
	attr->min  = CONFIG_RADIX_NODE_MIN;
	attr->max  = CONFIG_RADIX_NODE_MAX;
	attr->ctor = NULL;
	attr->dtor = NULL;
	return 0;
}

/**
 * Expands the tree.
 * @root	tree root
 * @return	error code
 */
static int expand_tree(struct radix_s* root)
{
	kmem_req_t            req;
	struct radix_node_s	*new_root;
	uint64_t		tmp = 0x8000000000000000LLU;
	int			i;

	req.type  = KMEM_RADIX_NODE;
	req.size  = sizeof(struct radix_node_s);
	req.flags = AF_KERNEL;
  
	if ((new_root = kmem_alloc(&req)) == NULL)
		return -ENOMEM;

	// new_root initialization
	memset(new_root, 0, sizeof(*new_root));
	new_root->children[0] = (void*) root->root_node;
  
	if (root->height != 0)
	{
		for (i = 0; i < NB_TAGS; i++)
			new_root->tags[i] = (root->root_node->tags[i]) ? tmp : 0;
    
		new_root->count = 1;
		bitmap_set(new_root->bitmap, 63); 
		root->root_node->parent = new_root;
	}
  
	// replacing root_node
	root->root_node = new_root;
	root->height++;
	root->max_key = MAX_KEY_ID(root);
	return 0;
}

/**
   Shrinks the tree, as long as possible.
   @root	tree root
*/
static void shrink_tree(struct radix_s*	root)
{
	kmem_req_t req;
	struct radix_node_s* current_node;
  
	req.type = KMEM_RADIX_NODE;
 
	while (root->height > 0)
	{
		current_node = root->root_node;

		if(current_node->count != 1)
			break;
    
		if(current_node->children[0] == NULL)
			break;
    
		root->root_node = current_node->children[0];
		root->height--;
		req.ptr = current_node;
		kmem_free(&req);
	}

	root->max_key = MAX_KEY_ID(root);
}

/**
   Adds a child node to the node @node at index @index.
   @node		node
   @index	index of the child to add
   @return	error code
*/
static int create_node(struct radix_node_s* node, int index)
{
	kmem_req_t req;
	struct radix_node_s* new_node;

	req.type  = KMEM_RADIX_NODE;
	req.size  = sizeof(struct radix_node_s);
	req.flags = AF_KERNEL;
	new_node  = kmem_alloc(&req);
  
	if( new_node == NULL) return -ENOMEM;

	memset(new_node, 0, sizeof(*new_node));
  
	new_node->count = 0;
	new_node->parent = node;
	node->count++;
	node->children[index] = new_node;
	bitmap_set(node->bitmap, index);
	return 0;
}

/**
 * Remove given item & update the tree
 * 
 * @root    radix root
 * @info    item info
 * @return  error code
 */
error_t radix_tree_remove_item(struct radix_s *root, radix_item_info_t *info)
{
	kmem_req_t req;
	struct radix_node_s *current_node;
	struct radix_node_s *current_parent;
	uint_t current_height;
	uint_t next_child;
	uint_t deleted;
	uint_t key;
	uint_t i;
  
	req.type = KMEM_RADIX_NODE;

	current_node = info->node;
	current_node->children[info->index] = NULL;
	bitmap_clear(current_node->bitmap, info->index);
	current_node->count--;
	current_height = info->height;
	next_child = info->index;
	key = info->key;

	// updating local tag 
	for (i = 0; i < NB_TAGS; ++i)
		TAG_CLEAR(current_node, i, next_child);
  
	if (current_node->count == 0)
	{
		req.ptr = current_node;
		kmem_free(&req);
		deleted = 1;
	}
  
	current_node = current_node->parent;
	current_height--;

	// going back up the tree, up till we reach the root
	// updating tags on the way up
	// deleting childless nodes on the way up
	while (current_node)
	{
		next_child     = NEXT_CHILD(root, current_height, key);
		current_parent = current_node->parent;
    
		if (deleted)  // child node was deleted
		{ 
			current_node->children[next_child] = NULL;
			bitmap_clear(current_node->bitmap, next_child);
			current_node->count--;
      
			for (i = 0; i < NB_TAGS; ++i)
				TAG_CLEAR(current_node, i, next_child);
		}
		else
		{
			struct radix_node_s* temp_node = current_node->children[next_child];
			for (i = 0; i < NB_TAGS; ++i)
			{
				if (temp_node->tags[i])
					TAG_SET(current_node, i, next_child);
				else
					TAG_CLEAR(current_node, i, next_child);
			}
		}
		deleted = 0;
		if (current_node->count == 0)
		{
			req.ptr = current_node;
			kmem_free(&req);
			deleted = 1;
		}
		current_node = current_parent;
		current_height--;
	}

	// check if we can shrink the tree
	shrink_tree(root);
	return 0;
}

void radix_tree_init(struct radix_s* root)
{
	root->root_node = NULL;
	root->height  = 0;
	root->max_key = 0;
}

error_t radix_item_info_apply(struct radix_s *root, 
			      radix_item_info_t *info, 
			      radix_info_op_t operation,
			      void *new_item)
{
	switch(operation)
	{
	case RADIX_INFO_SET:
		info->node->children[info->index] = new_item;
		return 0;

	case RADIX_INFO_DELETE:
		return radix_tree_remove_item(root,info);

	case RADIX_INFO_INSERT:
		info->node->children[info->index] = new_item;
		info->node->count ++;
		bitmap_set(info->node->bitmap, info->index);
		return 0;

	default:
		printk(WARNING, "WARNING: %s: invalid operation on item (%d,%d)\n", 
		       __FUNCTION__, 
		       info->key, 
		       info->index);
		return EINVAL;
	}
}


bool_t radix_item_info_lookup(struct radix_s *root, uint_t key, radix_item_info_t *info)
{
	struct radix_node_s* current_node;
	int current_height;
	int next_child;
 
	if (root->root_node == NULL)
		return false;
  
	if (key > root->max_key)
		return false;

	// finding the item, going down tree
	current_height = 0;
	current_node = root->root_node;
 
	while (current_height < root->height-1)
	{
		next_child = NEXT_CHILD(root, current_height, key);
		current_node = (struct radix_node_s*) current_node->children[next_child];

		if (current_node == NULL)
			return false;

		current_height++;
	}

	next_child = NEXT_CHILD(root, current_height, key);
	info->item = current_node->children[next_child];
	info->node = current_node;
	info->key = key;
	info->index = next_child;
	info->height = current_height;

	return true;
}

void* radix_tree_lookup(struct radix_s*	root, unsigned long key)
{
	radix_item_info_t info;
	bool_t found;

	found = radix_item_info_lookup(root, key, &info);
	return (found) ? info.node->children[info.index] : NULL;
}

int radix_tree_insert(struct radix_s* root, unsigned long key, void* item)
{
	int out;
	int err;
	int current_height;
	int next_child;
	struct radix_node_s* current_node;

	// expanding the tree until we have enough space to store the key
	while ((root->height == 0) || (root->max_key < key))
	{
		if ((out = expand_tree(root)))
			return out;
	}

	current_height = 0;
	current_node   = root->root_node;
	
	// finding the leaf node
	while (current_height+1 < root->height)
	{
		next_child = NEXT_CHILD(root, current_height, key);

		if (current_node->children[next_child] == NULL)   // no node found, creating one
		{ 
			if ((err = create_node(current_node, next_child)))
				return err;
		}
    
		//next_child = NEXT_CHILD(root, current_height, key);
		current_node = (struct radix_node_s*) current_node->children[next_child];
		current_height++;
	}

	next_child = NEXT_CHILD(root, current_height, key);

	// inserting the item
	if (current_node->children[next_child] != NULL)
		return -EEXIST;
  
	current_node->children[next_child] = item;
	current_node->count++;
	bitmap_set(current_node->bitmap, next_child);
	return 0;
}

void* radix_tree_delete(struct radix_s*	root, unsigned long key)
{
	radix_item_info_t info;
	bool_t found;
	void* item;
	error_t err;

	found = radix_item_info_lookup(root, key, &info);

	if(found == false) 
		return NULL;

	item = info.node->children[info.index];
  
	if(item == NULL)
	{
		printk(WARNING,"WARNING: %s: No item of key %d as may expected\n", 
		       __FUNCTION__, 
		       key); 

		return NULL; // no item
	}

	err  = radix_tree_remove_item(root, &info);  
	return (err != 0) ? NULL : item;
}


unsigned int radix_tree_gang_lookup(struct radix_s*	root,
				    void**		results,
				    unsigned long	first_index,
				    unsigned int	max_items)
{
	if (root->root_node == NULL)
		return 0;
  
	if (first_index > root->max_key)
		return 0;
  
	if (max_items == 0)
		return 0;

	// finding the first item, going down tree
	uint_t current_height = 0;
	struct radix_node_s* current_node = root->root_node;
	sint_t next_child;
	uint_t child_order[6];

	while (current_height < root->height-1)
	{
		struct radix_node_s *temp_current_node;

		next_child = NEXT_CHILD(root, current_height, first_index);
		child_order[current_height] = next_child;
		temp_current_node = (struct radix_node_s*) current_node->children[next_child];
		if (temp_current_node == NULL)
			break;
		current_node = temp_current_node;
		current_height++;
	}

	unsigned int out = 0;
	next_child = NEXT_CHILD(root, current_height, first_index);
	child_order[current_height] = next_child;
  
	if ( (current_height == root->height-1) && (current_node->children[next_child] != NULL) )
	{
		results[out] = current_node->children[next_child];
		out++;
	}

	next_child = ++ child_order[current_height];
  
	while (out < max_items)
	{    
		if (next_child == 64) // end of current node, going up
		{      
			if (current_node->parent == NULL) // end of the tree
				break;
      
			current_node = current_node->parent;
			current_height--;
			next_child = ++child_order[current_height];
			continue;
		}

		if (current_node->children[next_child] == NULL)
		{
			// nothing in this child, moving along
			goto UPDATE_LOOP;
		}

		if (current_height+1 == root->height)
		{
			// we're in a leaf, adding next item if it exists
			if (current_node->children[next_child] != NULL)
			{
				results[out] = current_node->children[next_child];
				out++;
			}
		}
		else
		{
			// else, not in a leaf, going down, if child exists
			if (current_node->children[next_child] != NULL)
			{
				current_node = current_node->children[next_child];
				current_height ++;
				child_order[current_height] = 0;
				next_child = 0;
				continue;
			}
		}

	UPDATE_LOOP:
		next_child = bitmap_ffs2(current_node->bitmap, next_child+1, 8);
		next_child = (next_child < 0) ? 64 : next_child;
	}

	return out;
}

void* radix_tree_tag_set(struct radix_s* root, unsigned long index, unsigned int tag)
{
	if (root->root_node == NULL)
		return NULL;

	if (index > root->max_key)
		return NULL;
  
	if (tag >= NB_TAGS)
		return NULL;

	// find the item, go down tree
	int current_height = 0;
	struct radix_node_s* current_node = root->root_node;
	int next_child;

	while (current_height < root->height-1) 
	{
		next_child = NEXT_CHILD(root, current_height, index);
		current_node = (struct radix_node_s*) current_node->children[next_child];
		if (current_node == NULL)
			return NULL;
		current_height++;
	}

	next_child = NEXT_CHILD(root, current_height, index);
	if (current_node->children[next_child] == NULL)
		return NULL;
	TAG_SET(current_node, tag, next_child);

	struct radix_node_s* out = current_node->children[next_child];

	//going back up, updating tags
	current_height--;
	current_node = current_node->parent;
	while (current_node != NULL)
	{
		next_child = NEXT_CHILD(root, current_height, index);
		TAG_SET(current_node, tag, next_child);
		current_node = current_node->parent;
		current_height--;
	}
	return out;
}

void* radix_tree_tag_clear(struct radix_s* root, unsigned long index, unsigned int tag)
{
	if (root->root_node == NULL)
		return NULL;

	if (index > root->max_key)
		return NULL;

	if (tag >= NB_TAGS)
		return NULL;

	// find the item, go down tree
	int current_height = 0;
	struct radix_node_s* current_node = root->root_node;
	int next_child;

	while (current_height < root->height-1) 
	{
		next_child = NEXT_CHILD(root, current_height, index);
		current_node = (struct radix_node_s*) current_node->children[next_child];

		if (current_node == NULL)
			return NULL;

		current_height++;
	}

	next_child = NEXT_CHILD(root, current_height, index);

	if (current_node->children[next_child] == NULL)
		return NULL;

	TAG_CLEAR(current_node, tag, next_child);

	struct radix_node_s* out = current_node->children[next_child];

	//going back up, updating tags
	current_height--;
	current_node = current_node->parent;
  
	while (current_node != NULL)
	{
		next_child = NEXT_CHILD(root, current_height, index);
		TAG_CLEAR(current_node, tag, next_child);
		current_node = current_node->parent;
		current_height--;
	}
	return out;
}

int radix_tree_tag_get(struct radix_s* root, unsigned long index, unsigned int tag)
{
	if (root->root_node == NULL)
		return -EINVAL;

	if (index > root->max_key)
		return -EINVAL;
 
	if (tag >= NB_TAGS)
		return -EINVAL;

	// find the item, go down tree
	int current_height = 0;
	struct radix_node_s* current_node = root->root_node;
	int next_child;

	while (current_height < root->height-1)
	{
		next_child = NEXT_CHILD(root, current_height, index);
		current_node = (struct radix_node_s*) current_node->children[next_child];
		if (current_node == NULL)
			return -ENOTFOUND;
		current_height++;
	}

	next_child = NEXT_CHILD(root, current_height, index);
	if (current_node->children[next_child] == NULL)
		return -ENOTFOUND;
	return TAG_GET(current_node, tag, next_child);
}


unsigned int radix_tree_gang_lookup_tag(struct radix_s*	root,
					void**		results,
					unsigned long	first_index,
					unsigned int	max_items,
					unsigned int	tag)
{
	if (root->root_node == NULL)
		return 0;
  
	if (first_index > root->max_key)
		return 0;
  
	if (max_items == 0)
		return 0;

	// finding the first item, going down tree
	int current_height = 0;
	struct radix_node_s* current_node = root->root_node;
	int next_child;
	int child_order[6];

	while (current_height < root->height-1)
	{
		next_child = NEXT_CHILD(root, current_height, first_index);
		child_order[current_height] = next_child;
		current_node = (struct radix_node_s*) current_node->children[next_child];
		if (current_node == NULL)
			break;
		current_height++;
	}

	unsigned int out = 0;
	next_child = NEXT_CHILD(root, current_height, first_index);
	child_order[current_height] = next_child;

	child_order[current_height]++;
	while (out < max_items)
	{
		if (child_order[current_height] == 64) 
		{
			// end of current node, going up
			if (current_node->parent == NULL)
				// end of the tree
				break;
			current_node = current_node->parent;
			current_height--;
			child_order[current_height]++;
		}

		if (current_node->children[child_order[current_height]] == NULL)
		{
			// nothing in this child, move along
			child_order[current_height]++;
			continue;
		}

		if (current_height+1 == root->height)
		{
			// we're in a leaf
			// adding next item if the tag is set
			if ((current_node->children[child_order[current_height]] != NULL) 
			    && (TAG_GET(current_node, tag, next_child)) ) 
			{
				results[out] = current_node->children[child_order[current_height]];
				out++;
			}
			child_order[current_height]++;
		} 
		else 
		{
			// else, going down, is the node exists and the tag is set
			if ( (current_node->children[child_order[current_height]] != NULL) &&
			     (TAG_GET(current_node, tag, next_child)) ) 
			{
				current_node = current_node->children[child_order[current_height]];
				current_height++;
				child_order[current_height] = 0;
			} 
			else
				child_order[current_height]++;
		}
	}

	return out;
}

unsigned int radix_tree_gang_lookup_tag_slot(struct radix_s*	root,
					     void***		results,
					     unsigned long	first_index,
					     unsigned int	max_items,
					     unsigned int	tag)
{

	if (root->root_node == NULL)
		return 0;
  
	if (first_index > root->max_key)
		return 0;
  
	if (max_items == 0)
		return 0;

	// finding the first item, going down tree
	int current_height = 0;
	struct radix_node_s* current_node = root->root_node;
	int next_child;
	int child_order[6];

	while (current_height < root->height-1) 
	{
		next_child = NEXT_CHILD(root, current_height, first_index);
		child_order[current_height] = next_child;
		current_node = (struct radix_node_s*) current_node->children[next_child];
		if (current_node == NULL)
			break;
		current_height++;
	}

	unsigned int out = 0;
	next_child = NEXT_CHILD(root, current_height, first_index);
	child_order[current_height] = next_child;

	while (out < max_items)
	{
		if (child_order[current_height] == 64) 
		{
			// end of current node, going up
			if (current_node->parent == NULL)
				// end of the tree
				break;
			current_node = current_node->parent;
			current_height--;
			child_order[current_height]++;
		}

		if (current_node->children[child_order[current_height]] == NULL) {
			// nothing in this child, move along
			child_order[current_height]++;
			continue;
		}

		if (current_height+1 == root->height) {
			// we're in a leaf
			// adding next item if the tag is set
			if ( (current_node->children[child_order[current_height]] != NULL) &&
			     (TAG_GET(current_node, tag, next_child)) ) {
				results[out] = current_node->children[child_order[current_height]];
				out++;
			}
			child_order[current_height]++;
		} else {
			// else, going down, is the node exists and the tag is set
			if ( (current_node->children[child_order[current_height]] != NULL) &&
			     (TAG_GET(current_node, tag, next_child)) ) {
				current_node = current_node->children[child_order[current_height]];
				current_height++;
				child_order[current_height] = 0;
			} else
				child_order[current_height]++;
		}
	}

	return out;
}

int radix_tree_tagged(struct radix_s* root, unsigned int tag)
{
	if (root->height == 0)
		return -EINVAL;
  
	if (tag >= NB_TAGS)
		return -EINVAL;
  
	return (root->root_node->tags[tag]);
}
