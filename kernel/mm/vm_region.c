/*
 * mm/vm_region.c - virtual memory region related operations
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

#include <types.h>
#include <list.h>
#include <errno.h>
#include <libk.h>
#include <bits.h>
#include <thread.h>
#include <task.h>
#include <ppm.h>
#include <pmm.h>
#include <mapper.h>
#include <spinlock.h>
#include <vfs.h>
#include <page.h>
#include <ppn.h>
#include <vmm.h>
#include <kmem.h>
#include <vm_region.h>

static void vm_region_ctor(struct kcm_s *kcm, void *ptr)
{
	struct vm_region_s *region;
  
	region = (struct vm_region_s*)ptr;
	//spinlock_init(&region->vm_lock, "VM Region");
	mcs_lock_init(&region->vm_lock, "VM Region");
}

KMEM_OBJATTR_INIT(vm_region_kmem_init)
{
	attr->type   = KMEM_VM_REGION;
	attr->name   = "KCM VM REGION";
	attr->size   = sizeof(struct vm_region_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VM_REGION_MIN;
	attr->max    = CONFIG_VM_REGION_MAX;
	attr->ctor   = &vm_region_ctor;
	attr->dtor   = NULL;
	return 0;
}

#if CONFIG_VMM_REGION_DEBUG
static int browse_rb(struct rb_root *root);
#endif

error_t vm_region_init(struct vm_region_s *region,
		       uint_t vma_start, 
		       uint_t vma_end, 
		       uint_t prot,
		       uint_t offset,
		       uint_t flags)
{
	register uint_t pgprot;
	register uint_t regsize;
  
	atomic_init(&region->vm_refcount, 1);
	regsize           = 1 << CONFIG_VM_REGION_KEYWIDTH;
	region->vm_begin  = ARROUND_DOWN(vma_start, regsize);
	region->vm_start  = vma_start;
	region->vm_limit  = vma_end;
	region->vm_flags  = flags;
	region->vm_offset = offset;
	region->vm_op     = NULL;
	region->vm_mapper = (const struct mapper_s){ 0 };
	region->vm_file   = (const struct vfs_file_s){ 0 };

	if(flags & VM_REG_HEAP)
		region->vm_end = ARROUND_UP(CONFIG_TASK_HEAP_MAX_SIZE, regsize);
	else
		region->vm_end = ARROUND_UP(vma_end, regsize);

	pgprot = PMM_USER | PMM_CACHED | PMM_ACCESSED | PMM_DIRTY;

	if(prot & VM_REG_WR)
	{
		prot   |= VM_REG_RD;
		pgprot |= PMM_WRITE;
	}

	if(prot & VM_REG_RD)
		pgprot |= PMM_READ;
  
	if(prot & VM_REG_EX)
		pgprot |= PMM_EXECUTE;

	if(prot != VM_REG_NON)
		pgprot |= PMM_PRESENT;
  
	if(flags & VM_REG_HUGETLB)
		pgprot |= PMM_HUGE;

	region->vm_prot   = prot;
	region->vm_pgprot = pgprot;

	return 0;
}

error_t vm_region_destroy(struct vm_region_s *region)
{  
	return 0;
}

struct vm_region_s* vm_region_find_list(struct vmm_s *vmm, uint_t vaddr)
{
	struct vm_region_s *region;
	struct list_entry *iter;

	list_foreach_forward(&vmm->regions_root, iter)
	{
		region = list_element(iter, struct vm_region_s, vm_list);

		if(vaddr < region->vm_end)
		{
			vmm->last_region = region;
			return region;
		}
	}

	return NULL;
}

/* This function has been adapted from Linux source code */
static struct vm_region_s* vm_region_find_prepare(struct vmm_s *vmm, uint_t addr,
						  struct vm_region_s **pprev,
						  struct rb_node ***rb_link,
						  struct rb_node ** rb_parent)
{
	struct vm_region_s * vma;
	struct rb_node ** __rb_link, * __rb_parent, * rb_prev;

	__rb_link = &vmm->regions_tree.rb_node;
	rb_prev = __rb_parent = NULL;
	vma = NULL;

	while (*__rb_link) {
		struct vm_region_s *vma_tmp;

		__rb_parent = *__rb_link;
		vma_tmp = rb_entry(__rb_parent, struct vm_region_s, vm_node);

		if (vma_tmp->vm_end > addr) {
			vma = vma_tmp;
			if (vma_tmp->vm_begin <= addr)
				break;
			__rb_link = &__rb_parent->rb_left;
		} else {
			rb_prev = __rb_parent;
			__rb_link = &__rb_parent->rb_right;
		}
	}

	*pprev = NULL;
	if (rb_prev)
		*pprev = rb_entry(rb_prev, struct vm_region_s, vm_node);
	*rb_link = __rb_link;
	*rb_parent = __rb_parent;
	return vma;
}

/* This function has been adapted from Linux source code */
static void __vma_link_rb(struct vmm_s *mm, struct vm_region_s *vma,
		struct rb_node **rb_link, struct rb_node *rb_parent)
{
	rb_link_node(&vma->vm_node, rb_parent, rb_link);
	rb_insert_color(&vma->vm_node, &mm->regions_tree);
}

/* This function has been adapted from Linux source code */
struct vm_region_s* vm_region_find(struct vmm_s *vmm, uint_t vaddr)
{
	struct vm_region_s *reg;
	struct vm_region_s *ptr;
	struct rb_node *node;

	reg  = NULL;
	node = vmm->regions_tree.rb_node;

	while(node != NULL)
	{
		ptr = rb_entry(node, struct vm_region_s, vm_node);

		if(vaddr < ptr->vm_end)
		{
			reg = ptr;

			if (vaddr >= ptr->vm_begin)
				break;

			node = node->rb_left;
		}
		else
			node = node->rb_right;
	}

	return reg;
}

static error_t vm_region_solve(struct vmm_s *vmm, struct vm_region_s *region)
{
	struct rb_node **rb_link, *rb_parent;
	struct vm_region_s *reg, *prev;
	uint_t start;
	uint_t size;
	uint_t limit;

	size  = region->vm_end;
	limit = CONFIG_USR_LIMIT - size;
	reg   = NULL;
	start = vmm->last_mmap;

	while(start <= limit)
	{
		reg = vm_region_find_prepare(vmm, start, &prev, &rb_link, &rb_parent);

		if((reg == NULL) || ((start + size) < reg->vm_begin))
			break;

		start = reg->vm_end;
	}

	if(start > limit)
		return ENOMEM;

	region->vm_begin  = start;
	region->vm_start  = start;
	region->vm_limit += start;
	region->vm_end    = start + size;

	__vma_link_rb(vmm, region, rb_link, rb_parent);

	if(reg != NULL)
	{
		if(region->vm_begin < reg->vm_begin)
			list_add_pred(&reg->vm_list, &region->vm_list);
		else
			list_add_next(&reg->vm_list, &region->vm_list);
	}
	else
		list_add_last(&vmm->regions_root, &region->vm_list);

	return 0;
}

static error_t vm_region_attach_rbtree(struct vmm_s *vmm, struct vm_region_s *region)
{
	struct rb_node **rb_link, *rb_parent;
	struct vm_region_s *reg, *prev;

	region->vmm = vmm;

	if(region->vm_flags & VM_REG_FIXED)
	{
		reg = vm_region_find_prepare(vmm, region->vm_begin, &prev, &rb_link, &rb_parent);

		if((reg != NULL) && (region->vm_begin >= reg->vm_begin))
			return EEXIST;

		if(reg == NULL)
			list_add_last(&vmm->regions_root, &region->vm_list);
		else
			list_add_pred(&reg->vm_list, &region->vm_list);

		__vma_link_rb(vmm, region, rb_link, rb_parent);
		return 0;
	}

	return vm_region_solve(vmm, region);
}

error_t vm_region_attach(struct vmm_s *vmm, struct vm_region_s *region)
{
	error_t	err = vm_region_attach_rbtree(vmm, region);

	if(err) return err;

	vmm->last_mmap = region->vm_begin;

#if CONFIG_VMM_REGION_DEBUG
	uint_t count = browse_rb(&vmm->regions_tree);
	printk(INFO, "%s: found %d regions\n", __FUNCTION__, count);
#endif
	return 0;
}

//FIXME: PPN could be remote => page remote
/* TODO: compute LAZY flag */
error_t vm_region_unmap(struct vm_region_s *region)
{
	register uint_t vaddr;
	register uint_t count;
	register bool_t isLazy;
	struct pmm_s *pmm;
	struct ppm_s *ppm;
	pmm_page_info_t info;
	uint_t attr;
	error_t err;
	ppn_t ppn;
  
	count  = (region->vm_limit - region->vm_start) >> PMM_PAGE_SHIFT;
	vaddr  = region->vm_start;
	pmm    = &region->vmm->pmm;
	isLazy = (region->vm_flags & VM_REG_LAZY) ? true : false;

	while(count)
	{
		if((err = pmm_get_page(pmm, vaddr, &info)))
			goto NEXT;

		if(info.attr & PMM_PRESENT)
		{
			ppn          = info.ppn;
			attr         = info.attr;
			ppm          = pmm_ppn2ppm(ppn);
			info.attr    = 0;
			info.ppn     = 0;
			info.cluster = NULL;

			if(isLazy == false)
				pmm_set_page(pmm, vaddr, &info);

			//if(page->mapper == NULL) 
			ppn_refcount_down(ppn);//also free the pag is count reach 0
      
		}

	NEXT:
		vaddr += PMM_PAGE_SIZE;
		count --;
	}
  
	return 0;
}


/* TODO: Informe the pmm layer about this event */
/* vmm lock must be taken (wrlock) before calling this function */
error_t vm_region_detach(struct vmm_s *vmm, struct vm_region_s *region)
{
	struct vfs_file_s *file;
	kmem_req_t req;
	uint_t regsize;
	uint_t key_start;
	uint_t count;
	uint_t mapper_count;
	bool_t isSharedAnon;

	list_unlink(&region->vm_list);
	rb_erase(&region->vm_node, &vmm->regions_tree);
	
	if(vmm->last_region == region)
		vmm->last_region = NULL;

	if(region->vm_end < vmm->last_mmap)
		vmm->last_mmap = region->vm_begin;

	if(!(region->vm_flags & VM_REG_LAZY))
	{
		regsize   = region->vm_end - region->vm_begin;
		key_start = region->vm_begin >> CONFIG_VM_REGION_KEYWIDTH;
		keysdb_unbind(&vmm->regions_db, key_start, regsize >> CONFIG_VM_REGION_KEYWIDTH);
	}
	rwlock_unlock(&vmm->rwlock);

	isSharedAnon = ((region->vm_flags & VM_REG_SHARED) && (region->vm_flags & VM_REG_ANON));
	mapper_count = 0;

	if(isSharedAnon)
	{
		assert(!VFS_FILE_IS_NULL(region->vm_file));
		assert(region->vm_mapper.m_home_cid == current_cid);

		mapper_count = atomic_add(&region->vm_mapper.m_refcount, -1);
		rwlock_wrlock(&region->vm_mapper.m_reg_lock);
		list_unlink(&region->vm_shared_list);
		rwlock_unlock(&region->vm_mapper.m_reg_lock);
	}

	while((count = atomic_get(&region->vm_refcount)) > 1)
		sched_yield(current_thread);

  
	if(!VFS_FILE_IS_NULL(region->vm_file))
	{
		file = &region->vm_file;
		file->f_op->munmap(file, region);
		vfs_close(file, NULL);
	}

	vm_region_unmap(region);

	if((isSharedAnon) && (mapper_count == 1))
		mapper_destroy(&region->vm_mapper, false);

	req.type = KMEM_VM_REGION;
	req.ptr  = region;
	kmem_free(&req);
  
	rwlock_wrlock(&vmm->rwlock);
	return 0;
}

error_t vm_region_resize(struct vmm_s *vmm, struct vm_region_s *region, uint_t start, uint_t end)
{
	return 0;
}

/* TODO: use a marker of last active page in the region, purpose is to reduce time of duplication */
error_t vm_region_dup(struct vm_region_s *dst, struct vm_region_s *src)
{
	struct rb_node **rb_link, *rb_parent;
	struct vm_region_s *prev;
	register struct pmm_s *src_pmm;
	register struct pmm_s *dst_pmm;
	register uint_t vaddr;
	register uint_t count;
	struct task_s *task;
	pmm_page_info_t info;
	ppn_t ppn;
	error_t err;
	uint_t onln_clusters;
	bool_t isFirstReg;

	atomic_init(&dst->vm_refcount, 1);
	dst->vm_begin   = src->vm_begin;
	dst->vm_end     = src->vm_end;
	dst->vm_start   = src->vm_start;
	dst->vm_limit   = src->vm_limit;
	dst->vm_prot    = src->vm_prot;
	dst->vm_pgprot  = src->vm_pgprot;
	dst->vm_flags   = src->vm_flags;
	dst->vm_offset  = src->vm_offset;
	dst->vm_op      = src->vm_op;
	dst->vm_mapper  = src->vm_mapper;
	dst->vm_file    = src->vm_file;
	dst->vm_data    = src->vm_data;
	task            = vmm_get_task(dst->vmm);

	(void) vm_region_find_prepare(dst->vmm, dst->vm_begin, &prev, &rb_link, &rb_parent);
	__vma_link_rb(dst->vmm, dst, rb_link, rb_parent);

	if(!VFS_FILE_IS_NULL(src->vm_file))
	{
		assert(!MAPPER_IS_NULL(src->vm_mapper));
		vfs_file_up(&src->vm_file);
	}
	else if(dst->vm_flags & VM_REG_SHARED)
	{
		isFirstReg = false;

		assert(src->vm_mapper.m_home_cid == current_cid);

		rwlock_wrlock(&dst->vm_mapper.m_reg_lock);

		list_add_last(&dst->vm_mapper.m_reg_root, &dst->vm_shared_list);

		rwlock_unlock(&dst->vm_mapper.m_reg_lock);
		
		//if(!src->vm_file)//if ANONYMOUS && SHARED
		//{

			count = atomic_add(&dst->vm_mapper.m_refcount, 1);

#if CONFIG_MAPPER_AUTO_MGRT
			onln_clusters = arch_onln_cluster_nr();

			if((dst->vm_flags & VM_REG_ANON) && (count == 1) && (onln_clusters > 1))
			{
				count = mapper_set_auto_migrate(&dst->vm_mapper);//?

				printk(INFO, "INFO: cpu %d, pid %d, reg <%x,%x> set auto-migrate for %d pages\n",
				       cpu_get_id(),
				       task->pid,
				       dst->vm_start,
				       dst->vm_limit,
				       count);
			}
		//}
#endif
		//return 0;
	}

	vaddr   = src->vm_start;
	src_pmm = &src->vmm->pmm;
	dst_pmm = &dst->vmm->pmm;
	count   = (src->vm_limit - src->vm_start) >> PMM_PAGE_SHIFT;

	while(count)
	{
		if((err = pmm_get_page(src_pmm, vaddr, &info)))
			goto REG_DUP_ERR;

		ppn  = info.ppn;

		//FIXME: if a page is set to migrate the prensent bit is unset !?
		if(info.attr & PMM_PRESENT)	/* TODO: review this condition on swap */
		{
			info.attr |= PMM_COW;
			info.attr &= ~(PMM_WRITE);
			info.cluster = task->cluster;
			info.cluster = NULL;

			if((err = pmm_set_page(dst_pmm, vaddr, &info)))
				goto REG_DUP_ERR;

			//if(page->mapper == NULL){
				if((err = pmm_set_page(src_pmm, vaddr, &info)))
					goto REG_DUP_ERR;
				ppn_refcount_up(ppn);
			//}
		}

		vaddr += PMM_PAGE_SIZE;
		count --;
	}

	return 0;

REG_DUP_ERR:
	return err;
}

/* FIXME: review this function */
error_t vm_region_split(struct vmm_s *vmm, struct vm_region_s *region, uint_t start_addr, uint_t length)
{
	kmem_req_t req;
	struct vm_region_s *new_region;
	error_t err;

	return ENOSYS;

	req.type  = KMEM_VM_REGION;
	req.size  = sizeof(*region);
	req.flags = AF_KERNEL;

	if((new_region = kmem_alloc(&req)) == NULL)
		return ENOMEM;
  
	new_region->vmm = vmm;
  
	/* FIXME: review this call as the dup has changed */
	if((err = vm_region_dup(new_region, region)))
		return err;

	new_region->vm_limit = start_addr;
  
	return vm_region_resize(vmm, region, start_addr + length, region->vm_limit);
}


error_t vm_region_update(struct vm_region_s *region, uint_t vaddr, uint_t flags)
{
	error_t err;
	bool_t isTraced;
	struct thread_s *this;

	if(pmm_except_isRights(flags))
		goto fail_rights;

	if((pmm_except_isPresent(flags)) && !(region->vm_prot & VM_REG_RD))
		goto fail_read;

	if((pmm_except_isWrite(flags)) && !(region->vm_prot & VM_REG_WR))
		goto fail_write;

	if((pmm_except_isExecute(flags)) && !(region->vm_prot & VM_REG_EX))
		goto fail_execute;

	if(region->vm_flags & VM_REG_INIT)
		goto fail_region;

	this     = current_thread;
	isTraced = this->info.isTraced;
  
#if CONFIG_SHOW_PAGEFAULT
	isTraced = true;
#endif	/* CONFIG_SHOW_PAGEFAULT */

	if(isTraced)
	{
		printk(DEBUG,
		       "DEBUG: %s: cpu %d, pid %d, vaddr %x, flags %x, region [%x,%x]\n",
		       __FUNCTION__,
		       cpu_get_id(),
		       this->task->pid,
		       vaddr,
		       flags, 
		       region->vm_start, 
		       region->vm_limit);
	}
  
	err = region->vm_op->page_fault(region, vaddr, flags);
	goto update_end;

fail_region:
	err = -1005;
	goto update_end;

fail_execute:
	err = -1004;
	goto update_end;

fail_write:
	err = -1003;
	goto update_end;

fail_read:
	err = -1002;
	goto update_end;

fail_rights:
	err = -1001;
  
update_end:
	if(err) 
		printk(INFO, "INFO: %s: cpu %d, vaddr 0x%x flags 0x%x, prot 0x%x, ended with err %d\n", 
		       __FUNCTION__, 
		       cpu_get_id(),
		       vaddr,
		       flags,
		       region->vm_prot,
		       err);

	return err;
}

#if CONFIG_VMM_REGION_DEBUG
/* This function is adapted from Linux source code */
static int browse_rb(struct rb_root *root)
{
	int i = 0, j;
	struct rb_node *nd, *pn = NULL;
	unsigned long prev = 0, pend = 0;

	for (nd = rb_first(root); nd; nd = rb_next(nd))
	{
		struct vm_region_s *vma;

		vma = rb_entry(nd, struct vm_region_s, vm_node);

		if (vma->vm_begin < prev)
		{
			vmm_reg_dmsg(1,"vm_begin %x prev %x\n", vma->vm_begin, prev);
			i = -1;
		}

		if (vma->vm_begin < pend)
			vmm_reg_dmsg(1,"vm_begin %x pend %x\n", vma->vm_begin, pend);

		if (vma->vm_begin > vma->vm_end)
			vmm_reg_dmsg(1,"vm_end %x < vm_start %x\n", vma->vm_end, vma->vm_begin);

		i++;
		pn = nd;
		prev = vma->vm_begin;
		pend = vma->vm_end;
	}
	
	j = 0;
	for (nd = pn; nd; nd = rb_prev(nd)) {
		j++;
	}

	if (i != j)
	{
		vmm_reg_dmsg(1,"backwards %d, forwards %d\n", j, i); i = 0;
	}

	return i;
}
#endif /* CONFIG_VMM_REGION_DEBUG */
