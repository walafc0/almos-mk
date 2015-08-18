/*
 * mm/mapper.h - mapping object and its related operations
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

#ifndef _MAPPER_H_
#define _MAPPER_H_

#include <types.h>
#include <radix.h>
#include <mcs_sync.h>
#include <rwlock.h>
#include <atomic.h>
#include <list.h>

struct vfs_file_s;
struct page_s;

#define MAPPER_SYNC_OP              0x01

#define MAPPER_READ_PAGE(n)         error_t (n) (struct page_s *page, uint_t flags)
#define MAPPER_WRITE_PAGE(n)        error_t (n) (struct page_s *page, uint_t flags)
#define MAPPER_SYNC_PAGE(n)         error_t (n) (struct page_s *page)
#define MAPPER_RELEASE_PAGE(n)      error_t (n) (struct page_s *page)
#define MAPPER_SET_PAGE_DIRTY(n)    error_t (n) (struct page_s *page)
#define MAPPER_CLEAR_PAGE_DIRTY(n)  error_t (n) (struct page_s *page)

typedef MAPPER_READ_PAGE(mapper_read_page_t);
typedef MAPPER_WRITE_PAGE(mapper_write_page_t);
typedef MAPPER_SYNC_PAGE(mapper_sync_page_t);
typedef MAPPER_RELEASE_PAGE(mapper_release_page_t);
typedef MAPPER_SET_PAGE_DIRTY(mapper_set_page_dirty_t);
typedef MAPPER_CLEAR_PAGE_DIRTY(mapper_clear_page_dirty_t);

/* caller must have exclusive access to the page! */
struct mapper_op_s 
{
	/* read page from mapper's backend*/
	mapper_read_page_t	    *readpage;

	/* write page to mapper's backend */
	mapper_write_page_t	    *writepage;

	/* sync page if it's dirty */
	mapper_sync_page_t          *sync_page;
  
	/* set page dirty attribute, return true if this dirtied it */
	mapper_set_page_dirty_t     *set_page_dirty;

	/* clear page's dirty attribute, return true if this cleared it */
	mapper_clear_page_dirty_t   *clear_page_dirty;

	/* Release a page from mapper */
	mapper_release_page_t	    *releasepage;
};


struct mapper_cnt_s
{
	spinlock_t		     	mc_cache_lock;
	mcs_lock_t	             	mc_lock;
	struct vfs_inode_s*		mc_inode;	// owner
	const struct mapper_op_s*    	mc_ops;		// operations
	atomic_t			mc_refcount;
	radix_t			     	mc_radix;	// pages depot
	struct rwlock_s			mc_reg_lock;
	struct list_entry		mc_reg_root;
	void*				mc_data;		// private data
};

#define MAPPER_IS_NULL(mapper) ((mapper.m_cnt == NULL) && (mapper.m_home == NULL))

struct mapper_s 
{
	//union{
		//if m_home_cid is local then m_cnt else m_home
		struct mapper_cnt_s*	m_cnt;
		struct mapper_s*	m_home;
	//};
	cid_t			m_home_cid;
	#define m_inode m_cnt->mc_inode
	#define m_ops m_cnt->mc_ops
	#define m_lock m_cnt->mc_lock
	#define m_cache_lock m_cnt->mc_cache_lock
	#define m_refcount m_cnt->mc_refcount
	#define m_radix m_cnt->mc_radix
	#define m_reg_lock m_cnt->mc_reg_lock
	#define m_reg_root m_cnt->mc_reg_root
	#define m_data m_cnt->mc_data
};

//FIXME: file_remote?
struct mapper_buff_s{
	struct vfs_file_remote_s *file;
	size_t size;
	size_t data_offset;
	size_t buff_offset;
	size_t max_ppns;
	ppn_t *buff_ppns;
};


#define MAPPER_IS_MIGRATABLE(mapper)	\
	(MAPPER_IS_HOME(mapper) \
	&& ((mapper)->m_inode == NULL))

#define MAPPER_IS_HOME(mapper)		\
	((mapper)->m_home_cid == current_cid)

// MAPPER API

/**
 * Inititalizes the kmem-cache for the mapper structure
 */
KMEM_OBJATTR_INIT(mapper_kmem_init);

/**
 * Init new mapper object
 *
 * @mapper      mapper object to initialize
 * @ops         mapper's operations
 * @node        mapper's associated node if any
 * @data        Private data if any
 */
error_t mapper_init(struct mapper_s *mapper, 
		    const struct mapper_op_s *ops, 
		    struct vfs_inode_s *inode,
		    void *data);

//similar to mapper_get_page but also hold the refcount!
ppn_t mapper_get_ppn(struct mapper_s* mapper, uint_t index, uint_t flags);

//Atomically read from the mapper content
size_t mapper_read(struct mapper_s* mapper, struct mapper_buff_s *buff_tbl, size_t nb_buff, uint_t flags);

//Atomically write to the mapper content
size_t mapper_write(struct mapper_s* mapper, struct mapper_buff_s *buff_tbl, size_t nb_buff, uint_t flags);


//
error_t mapper_replicate(struct mapper_s* mapper,
			struct mapper_s *mapper_home, 
			cid_t mapper_home_cid);

/**
 * Set Auto-Migrate Strategy
 *
 * @mapper     mapper to apply
 * @retrun     number of touched pages
  */
uint_t mapper_set_auto_migrate(struct mapper_s* mapper);

/**
 * Finds and gets a page reference.
 *
 * @mapper	mapper to search-in
 * @index	page index
 * @return	page looked up, NULL if not present
 */
struct page_s* mapper_find_page(struct mapper_s* mapper, uint_t index);

/**
 * Finds and gets up to @nr_pages pages references,
 * starting at @start and stores them in @pages.
 *
 * @mapper	mapper to search
 * @start	first page index to be looked up
 * @nr_pages	max pages to look up for
 * @pages	pages looked up
 * @return	number of pages actually in @pages
 */
uint_t mapper_find_pages(struct mapper_s* mapper, 
			 uint_t start, 
			 uint_t nr_pages, 
			 struct page_s** pages);

/**
 * Finds and gets up to @nr_pages pages references,
 * starting at @start and stores them in @pages.
 * Ensures that the returned pages are contiguous.
 *
 * @mapper	mapper to search
 * @start	first page index to be looked up
 * @nr_pages	max pages to look up for
 * @pages	pages looked up
 * @return	number of pages actually in @pages
 */
uint_t mapper_find_pages_contig(struct mapper_s* mapper, 
				uint_t start, 
				uint_t nr_pages, 
				struct page_s** pages);

/**
 * Finds and gets up to @nr_pages pages references,
 * with the tag @tag set, starting at @start
 * and stores them in @pages.
 *
 * @mapper	mapper object to search
 * @start	first page index to be looked up
 * @tag		tag to look for
 * @nr_pages	max pages to look up for
 * @pages	pages looked up
 * @return	number of pages actually in @pages
 */
uint_t mapper_find_pages_by_tag(struct mapper_s* mapper, 
				uint_t start, 
				uint_t tag, 
				uint_t nr_pages, 
				struct page_s** pages);

/**
 * Adds newly allocated pagecache pages.
 *
 * @mapper	mapper object
 * @page	page to add
 * @index	page index
 * @return	error code
 */
error_t mapper_add_page(struct mapper_s *mapper, struct page_s* page, uint_t index);

/**
 * Removes a page from the pagecache and frees it.
 *
 * @page	page to remove
 */
void mapper_remove_page(struct page_s* page);

/**
 * Reads into the pagecache, fills it if needed.
 * If the page already exists, ensures that it is
 * up to date.
 *
 * @mapper	mapper for the page
 * @index	page index
 * @flags       to be set to MAPPER_OP_SYNC for blocking mode
 * @data        opaque parameter
 * @return	the page read
 */
struct page_s* mapper_get_page(struct mapper_s*	mapper, uint_t index, uint_t flags);


/**
 * Writes and frees all the dirty pages from a mapper.
 *
 * @mapper	mapper to be destroyed
 * @doSync      Sync each dirty page before removing it
 */
void mapper_destroy(struct mapper_s *mapper, bool_t doSync);

/**
 * Generic method to read page (zeroed).
 * @page        buffer page to be zeroed
 * @flags       to be set to MAPPER_SYNC_OP if blocking request
 * @data        not used
 */
MAPPER_READ_PAGE(mapper_default_read_page);

/**
 * Generic method to write page (do nothing).
 * @page        not used
 * @flags       not used
 * @data        not used
 */
MAPPER_READ_PAGE(mapper_default_write_page);

/**
 * Generic method to release a page.
 *
 * @page	buffer page to be released
 * @return	error code, 0 if OK
 */
MAPPER_RELEASE_PAGE(mapper_default_release_page);

/**
 * Generic method to sync a page.
 *
 * @page	buffer page to be synced
 * @return	error code, 0 if OK
 */
MAPPER_SYNC_PAGE(mapper_default_sync_page);

/**
 * Generic method to dirty a page.
 *
 * @page	buffer page to be dirtied
 * @return	non-zero if the page got dirtied
 */
MAPPER_SET_PAGE_DIRTY(mapper_default_set_page_dirty);

/**
 * Generic method to clear a dirty page.
 *
 * @page	buffer page to be cleared
 * @return	non-zero if the page had been cleared
 */
MAPPER_CLEAR_PAGE_DIRTY(mapper_default_clear_page_dirty);

// MAPPER API END

#endif /* _MAPPER_H_ */
