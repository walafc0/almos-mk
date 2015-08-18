/*
 * mm/vmm.h - virtual memory management related operations
 *
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
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

#ifndef _VMM_H_
#define _VMM_H_

#include <types.h>
#include <list.h>
#include <rbtree.h>
#include <spinlock.h>
#include <rwlock.h>
#include <pmm.h>
#include <vm_region.h>
#include <keysdb.h>

struct task_s;
struct vfs_file_s;
struct page_s;
struct vmm_s;
struct vm_region_s;
struct vm_region_op_s;
struct mapper_s;

typedef enum
{
	VMM_ERESOLVED = 0,
	VMM_ECHECKUSPACE,
	VMM_ESIGSEGV,
	VMM_ESIGBUS
}vmm_error_t;

/* Virtual Memory Manager */
struct vmm_s
{
	spinlock_t lock;
	struct rwlock_s rwlock;

	/* Physical Memory Manager */
	struct pmm_s pmm;

	/* Attached Virtual Regions */
	struct vm_region_s *last_region;
	struct rb_root regions_tree;
	struct list_entry regions_root;
	struct keysdb_s regions_db;
	uint_t last_mmap;
	uint_t limit_addr;
	uint_t devreg_addr;
	
	/* Accuonting & Limits */
	uint_t pgfault_nr;
	uint_t spurious_pgfault_nr;
	uint_t remote_pages_nr;
	uint_t pages_nr;
	uint_t locked_nr;
	uint_t regions_nr;
	uint_t pages_limit;
	uint_t u_err_nr;
	uint_t m_err_nr;

	/* Task Image Information */
	uint_t text_start;
	uint_t text_end;
	uint_t data_start;
	uint_t data_end;
	uint_t heap_start;
	uint_t heap_current;
	uint_t entry_point;
	struct vm_region_s *heap_region;
};

#define MADV_NORMAL        0x0
#define MADV_RANDOM        0x1
#define MADV_SEQUENTIAL    0x2
#define MADV_WILLNEED      0x3
#define MADV_DONTNEED      0x4
#define MADV_MIGRATE       0x5

#define MGRT_DEFAULT       0x0
#define MGRT_STACK         0x1

typedef struct mmap_attr_s
{
	void *addr;
	uint_t length; 
	uint_t prot;
	uint_t flags;
	uint_t fd;
	off_t offset;
}mmap_attr_t;

error_t vmm_init(struct vmm_s *vmm);

error_t vmm_dup(struct vmm_s *dst, struct vmm_s *src);

error_t vmm_destroy(struct vmm_s *vmm);

#define vmm_get_task(vmm)

inline error_t vmm_check_address(char *objname, struct task_s *task, void *addr, uint_t size);

int sys_mmap(mmap_attr_t *mattr);
int sys_madvise(void *start, size_t length, uint_t advice);
int sys_sbrk(uint_t current_heap_ptr, uint_t size);

error_t vmm_sbrk(struct vmm_s *vmm, uint_t current, uint_t size);

void *vmm_mmap(struct task_s *task,
	       struct vfs_file_s *file, 
	       void *addr, 
	       uint_t length, 
	       uint_t prot, 
	       uint_t flags, 
	       uint_t offset);

error_t vmm_munmap(struct vmm_s *vmm, uint_t addr, uint_t length);

error_t vmm_madvise_migrate(struct vmm_s *vmm, uint_t start, uint_t len);

error_t vmm_madvise_willneed(struct vmm_s *vmm, uint_t start, uint_t len);

error_t vmm_set_auto_migrate(struct vmm_s *vmm, uint_t start, uint_t flags);

/* Hypothesis: the region is shared-anon, mapper list is rdlocked, page is locked */
error_t vmm_broadcast_inval(struct vm_region_s *region, struct page_s *page, struct page_s **new);

/* Hypothesis: the region is shared-anon, mapper list is rdlocked, page is locked */
error_t vmm_migrate_shared_page_seq(struct vm_region_s *region, struct page_s *page, struct page_s **new);

/** Page Fault Handler */
error_t vmm_fault_handler(uint_t bad_vaddr, uint_t flags);


//////////////////////////////////////////////////////////
///                  Private Section                   ///
//////////////////////////////////////////////////////////
#undef vmm_get_task
#define vmm_get_task(_vmm) list_container((_vmm), struct task_s, vmm)

#define vmm_check_object(_addr,_type,_id)				\
	({								\
		_type obj;						\
		error_t err;						\
		err = vmm_check_address(#_id, task_lookup_zero(), (_addr), sizeof(_type)); \
		if(!err)						\
		{							\
			err = cpu_copy_from_uspace(&obj,(_addr), sizeof(_type)); \
			if(!err)					\
				err = (obj.signature == (_id)) ? 0 : EINVAL; \
		}							\
		err;							\
	})


#endif /* _VMM_H_ */
