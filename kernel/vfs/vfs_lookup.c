/*
 * vfs/vfs_lookup.c - vfs lookup operation
 *
 * Copyright (c) 2015 UPMC Sorbonne Universites
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

#include <vfs-private.h>
#include <vfs-params.h>
#include <ku_transfert.h>
#include <cpu-trace.h>
#include <ppm.h>
#include <pmm.h>
#include <vfs.h>
#include <task.h>
#include <thread.h>

#if (VFS_MAX_PATH > PMM_PAGE_SIZE)
#error VFS_MAX_PATH must be less or equal to page size
#endif

struct vfs_context_s *vfs_pipe_ctx;

static uint_t vfs_dir_count(char *path) 
{
	uint_t count   = 0;
	char *path_ptr = path;

	while((path_ptr = strchr(path_ptr, '/')) != NULL) 
	{
		path_ptr ++;
		count    ++;
	}

	return (count == 0) ? 1 : count;
}


static uint_t vfs_split_path(char *path, char **dirs) 
{
	uint_t i=0;
	char *path_ptr;

	path_ptr = path;
	dirs[0]  = path_ptr;
	dirs[1]  = NULL;

	if((path_ptr[0] == '/') && (path_ptr[1] == '\0'))
		return 0;

	path_ptr = (path_ptr[0] == '/') ? path_ptr + 1 : path_ptr;
	dirs[0]  = path_ptr;
	i++;

	while((path_ptr = strchr(path_ptr, '/')) != NULL) 
	{
		*path_ptr = 0;
		path_ptr++;
		if(*path_ptr)
			dirs[i++] = path_ptr;
	}

	dirs[i] = NULL;

	return i-1;
}

//TODO: remove unecessary '..' path components
error_t __vfs_get_upath(struct ku_obj *ku_path, struct page_s **k_path_pg, char **k_path, uint_t *len)
{
	kmem_req_t req;
	struct page_s *page;
	char *path;
	uint_t count;
	uint_t rcount;
	error_t err;

	req.type  = KMEM_PAGE;
	req.size  = 0;
	req.flags = AF_KERNEL; 
	count     = 0;
	rcount    = 0;
	page      = kmem_alloc(&req);
  
	if(page == NULL) 
		return ENOMEM;
  
	path = ppm_page2addr(page);
	count = ku_path->get_size(ku_path);
  
	if(!count)
	{ 
		err = EINVAL;
		goto fail_len;
	}
  
	if(count > VFS_MAX_PATH)
	{
		err = ERANGE;
		goto fail_rng;
	}

	rcount = ku_path->scpy_from_buff(ku_path, path, count);
  
	if(rcount != count) 
	{
		err = EINVAL;
		goto fail_cpy;
	}

	path[count] = 0;

	if(len != NULL)
		*len = count;

#if CONFIG_ROOTFS_IS_VFAT	/* FIXME: ugly way for now */
	uint_t i;
	for(i=0; i<count; i++)
		path[i] = toupper(path[i]);
#endif

	vfs_dmsg(1, "%s: path [%s]\n", __FUNCTION__, path);
	*k_path_pg = page;
	*k_path    = path;
	return 0;

fail_cpy:
fail_len:
fail_rng:
	req.ptr = page;
	kmem_free(&req);
	return err;
}


//TODO 
//if no dirent avialable set this as a negatif dirent
//This set dirent->inode!
//dirent->d_parent = parent;
error_t vfs_real_lookup(struct vfs_inode_s *parent, uint32_t flags, 
			uint32_t isLast, struct vfs_dirent_s *dirent)
{
	error_t err = 0;
	struct thread_s *this;

	this = current_thread;

	cpu_trace_write(current_cpu, vfs_real_lookup);

	err = parent->i_op->lookup(parent, dirent);

	vfs_dmsg(1,"[ thread: %x || GID: %x ] i_op->lookup (parent %d, dirent %s) : err %d\n", 
		 this, this->lcpu->gid,
		 parent->i_number, dirent->d_name, err);

	if((err != VFS_NOT_FOUND) && (err != VFS_FOUND))
		return err;

	if((err == VFS_FOUND) && (flags & VFS_O_EXCL) && (flags & VFS_O_CREATE) && (isLast))

	if((err == VFS_FOUND) && (flags & VFS_O_EXCL) && (flags & VFS_O_CREATE) && (isLast))
		return EEXIST;

	vfs_dmsg(1,"[ %x :: %d ] node %s, found ? %d, isLast ? %d, VFS_O_CREATE ? %d, VFS_FIFO? %d\n",
		 this, this->lcpu->gid, dirent->d_name,err,
		 isLast,flags & VFS_O_CREATE, dirent->d_attr & VFS_FIFO);

	if((err == VFS_NOT_FOUND) && (flags & VFS_O_CREATE) && (isLast))
	{
		if((err=parent->i_op->create(parent, dirent)))
			return err;
	}

	return err;
}

struct vfs_dirent_s* vfs_inode_lookup(struct vfs_inode_s *inode, char *name)
{
	struct metafs_s *child;

	cpu_trace_write(current_cpu, vfs_inode_lookup);

	if( (((name[0] == '/') || (name[0] == '.')) && (name[1] == '\0')) ||
		((name[0] == '.') && (name[1] == '.')) )
	{
		printk(ERROR, "%s: This should not happen\n", __FUNCTION__);
		while(1);
	}

	if((child = metafs_lookup(&inode->i_meta, name)) == NULL)
	{
		vfs_dmsg(1,"[ thread: %x || GID: %x ] metafs_lookup (inode %d, dirent %s) not found\n", 
		current_thread, current_thread->lcpu->gid,
		inode->i_number, name);
		return NULL;
	}
	
	vfs_dmsg(1,"[ thread: %x || GID: %x ] metafs_lookup (inode %d, dirent %s) found\n", 
		 current_thread, current_thread->lcpu->gid,
		 inode->i_number, name);

	return metafs_container(child, struct vfs_dirent_s, d_meta);
}

struct vfs_inode_s* __vfs_lookup_gh_ref(struct vfs_inode_ref_s *ref, bool_t hold)
{
	if(vfs_inode_hold(ref->ptr, ref->gc))
		return NULL;
	return ref->ptr;
}

//a hold dos not require a put
struct vfs_inode_s* vfs_lookup_hold_ref(struct vfs_inode_ref_s *ref)
{
	return __vfs_lookup_gh_ref(ref, true);
}

//a get require a put
struct vfs_inode_s* vfs_lookup_get_ref(struct vfs_inode_ref_s *ref)
{
	return __vfs_lookup_gh_ref(ref, false);
}

void vfs_lookup_put_ref(struct vfs_inode_ref_s *ref, struct vfs_inode_s *inode)
{
	vfs_inode_down(inode);
}

//return dirent up
error_t vfs_load_dirent(struct vfs_inode_s *parent, char* name, 
				uint_t flags, char isLast, char setLoad,
				struct vfs_dirent_s **ret_dirent, 
				uint_t* lookup_flags)
{
	error_t err;
	uint_t isInLoad;
	struct vfs_dirent_s *dirent;
	struct vfs_dirent_s *new_dirent;

	err = 0;
	*lookup_flags = 0;
	*ret_dirent = NULL;

	vfs_inode_lock(parent);
	dirent = vfs_inode_lookup(parent, name);

VFS_DIRENT_UP:
	vfs_dirent_up(dirent);

	if(!dirent)
	{
		vfs_inode_unlock(parent);

		//new dirent count is one
		new_dirent = vfs_dirent_new(parent->i_ctx, name);
		if(!new_dirent)
		{
			err = ENOMEM;
			goto VFS_DIRENT_LOAD_EXIT2;
		}

		vfs_inode_lock(parent);
	
		dirent = vfs_inode_lookup(parent, name);
		if(dirent) goto VFS_DIRENT_UP;

		dirent = new_dirent;
		dirent->d_parent = parent;
		dirent->d_attr = (isLast) ? flags & 0x0000FFFF : VFS_DIR;

		vfs_dirent_set_inload(dirent); //VFS_SET(dirent->d_state, VFS_INLOAD);
		metafs_register(&parent->i_meta, &dirent->d_meta);
		vfs_inode_unlock(parent);
	
		err = vfs_real_lookup(parent, flags, isLast, dirent);	

		if(err)
		{
			vfs_inode_lock(parent);
			metafs_unregister(&parent->i_meta, &dirent->d_meta);
			vfs_inode_unlock(parent);
			vfs_dirent_down(dirent);
			goto VFS_DIRENT_LOAD_EXIT2;
		}

		vfs_inode_lock(parent);
		isInLoad = true;
	}

	if(!isInLoad)
	{
		//If another thread set it inload
		if(vfs_dirent_is_inload(dirent))
		{
			//TODO: wait rather than retry ?
			vfs_dmsg(1, "node %s is inload\n", dirent->d_name);
			VFS_SET(*lookup_flags, VFS_LOOKUP_RETRY);
			vfs_dirent_down(dirent);
			goto VFS_DIRENT_LOAD_EXIT;
		}

		if(setLoad)
		{
			isInLoad = true;
			vfs_dirent_set_inload(dirent);
		}
	}
		

	*ret_dirent = dirent;

	if(!setLoad && isInLoad) vfs_dirent_clear_inload(dirent);

	if(isLast && ((flags & VFS_DIR) && !(dirent->d_attr & VFS_DIR)))
		err = VFS_ENOTDIR;

VFS_DIRENT_LOAD_EXIT:
	vfs_inode_unlock(parent);
VFS_DIRENT_LOAD_EXIT2:
	return err;
}

//TODO: add the case of negatif entrys
RPC_DECLARE( __vfs_lookup_child, 
		RPC_RET(RPC_RET_PTR(struct vfs_lookup_response_s, lkr)), 
		RPC_ARG(RPC_ARG_PTR(char, name),
		RPC_ARG_PTR(struct vfs_lookup_s, lkp)))
{
	struct vfs_inode_ref_s *parent;
	struct vfs_dirent_s *dirent;
	error_t err;
	char isLast;
	char isHeld;

	err = 0;
	dirent = NULL;
	lkr->lookup_flags = 0;
	parent = &lkp->current;
	isLast = VFS_IS(lkp->lookup_flags, VFS_LOOKUP_LAST);
	isHeld = VFS_IS(lkp->lookup_flags, VFS_LOOKUP_HELD);
	
	if(!isHeld && vfs_inode_hold(parent->ptr, parent->gc))
	{
		/* The gc is no longer valid: simply retry */
		VFS_SET(lkr->lookup_flags, VFS_LOOKUP_RESTART);
		goto VFS_DIRENT_LOAD_ERROR;
	}

	if((err = vfs_load_dirent(parent->ptr, name, lkp->flags, isLast, 0, 
						&dirent, &lkr->lookup_flags)))
		goto VFS_DIRENT_LOAD_EXIT;

	if(!dirent->d_mounted)
	{
		/* This multi-field copy should be safe since 	*
		 * dirent content are fix 			*/
		lkr->inode = dirent->d_inode;
		lkr->ctx = NULL;
	}
	else
	{
		lkr->ctx = dirent->d_mounted;
		lkr->inode = (const struct vfs_inode_ref_s){ 0 };
	}

	vfs_dirent_down(dirent);

VFS_DIRENT_LOAD_EXIT:
	if(!isHeld)
		vfs_inode_down(parent->ptr);

VFS_DIRENT_LOAD_ERROR:
	lkr->err = err;
}


RPC_DECLARE( __vfs_lookup_parent, 
		RPC_RET(RPC_RET_PTR(struct vfs_lookup_response_s, lkr)), 
		RPC_ARG(RPC_ARG_PTR(struct vfs_lookup_s, lkp)))
{
	char isHeld;
	struct vfs_inode_s *cinode;
	struct vfs_inode_ref_s *current;

	current = &lkp->current;
	isHeld = VFS_IS(lkp->lookup_flags, VFS_LOOKUP_HELD);

	if((!isHeld) && (vfs_inode_hold(current->ptr, current->gc)))
	{
		/* The gc is no longer valid: simply retry */
		VFS_SET(lkr->lookup_flags, VFS_LOOKUP_RESTART);
		goto VFS_DIRENT_LOAD_EXIT;
	}

	cinode = current->ptr;
	lkr->inode.ptr = cinode->i_parent;
	lkr->inode.cid = cinode->i_pcid;
	lkr->inode.gc = cinode->i_pgc;

	if(!isHeld)
		vfs_inode_down(current->ptr);

VFS_DIRENT_LOAD_EXIT:
	lkr->err = 0;
}

void vfs_lookup_child(struct vfs_lookup_response_s *lkr, 
			struct vfs_lookup_s *lkp, 
			char* name)
{
		RCPC(	lkp->current.cid, 
			RPC_PRIO_FS_LOOKUP, 
			__vfs_lookup_child, 
			RPC_RECV(RPC_RECV_OBJ(*lkr)), 
			RPC_SEND(RPC_SEND_MEM(name, (strlen(name)+1)), 
			RPC_SEND_OBJ(*lkp)));
}

void vfs_lookup_parent(struct vfs_lookup_response_s *lkr, 
			struct vfs_lookup_s *lkp)
{
		RCPC(	lkp->current.cid, 
			RPC_PRIO_FS_LOOKUP, 
			__vfs_lookup_parent, 
			RPC_RECV(RPC_RECV_OBJ(*lkr)), 
			RPC_SEND(RPC_SEND_OBJ(*lkp)));
}

error_t vfs_lookup_elem(struct vfs_lookup_s *lkp, 
			struct vfs_lookup_response_s *lkr, 
			char* name)
{
	char isDot;
	char isRoot;
	char isDotDot;
	struct task_s *task;

	vfs_dmsg(1, "%s : lookup element = %s\n", __FUNCTION__, name);

	task = current_task;
	isRoot = ((name[0] == '/') && (name[1] == '\0'));
	isDot = ((name[0] == '.') && (name[1] == '\0'));
	isDotDot = ((name[0] == '.') && (name[1] == '.')) ? 1 : 0;

	lkr->err = 0;	

	if(isRoot)
	{
		lkr->inode = task->vfs_root.f_inode;
		return 0;
	}

	/* if the requested element is a dot path or the parent of root 
	 * return the current parent */
	if(isDot || (isDotDot && VFS_INODE_REF_COMPARE(lkp->current, task->vfs_root.f_inode)))
	{
		lkr->inode = lkp->current;
		return 0;
	}

	if(isDotDot)
		vfs_lookup_parent(lkr, lkp);
	else
		vfs_lookup_child(lkr, lkp, name);

	return 0;
}

error_t vfs_lookup_next(struct vfs_lookup_path_s * path)
{
	path->cptr++;
	vfs_dmsg(1, "%s : lookup next element = %s\n", __FUNCTION__, *path->cptr);

	return 0;
}

//FIXME: mode and check right!
//FIXME check mode allow this type of openning to be done in...
//TODO check right
error_t vfs_lookup_path_init(struct vfs_lookup_path_s *lkp_path, 
				struct ku_obj *path, uint_t flags, 
					uint_t mode, uint_t* last)
{
	error_t err;

	err = __vfs_get_upath(path, 
				&lkp_path->page, 
				&lkp_path->str, NULL);

	if(err) return err;
	
	err = ENAMETOOLONG;
	if(vfs_dir_count(lkp_path->str) <= VFS_MAX_PATH_DEPTH)
		lkp_path->path_ptrs = (char**)&lkp_path->__path_ptrs;
	else
		goto VFS_LOOKUP_PATH_EXIT;
		
		
	*last = vfs_split_path(lkp_path->str, lkp_path->path_ptrs);
	
	lkp_path->isAbsolute = (lkp_path->str[0] == '/') ? 1 : 0 ;
	lkp_path->cptr = &lkp_path->path_ptrs[0];
	lkp_path->flags = flags;
	lkp_path->mode = mode;
	return 0;

VFS_LOOKUP_PATH_EXIT:
	vfs_lookup_path_put(lkp_path);
	return err;
}

void vfs_lookup_path_put(struct vfs_lookup_path_s *lkp_path)
{
	kmem_req_t req;
  
	req.size = 0;
	req.type = KMEM_PAGE;
	req.ptr  = lkp_path->page;

	kmem_free(&req);
}

error_t __vfs_lookup(struct vfs_file_s *cwd, 
			struct vfs_lookup_path_s *lkp_path,
			uint32_t lookup_flags, 
			struct vfs_lookup_response_s *lkr)
{
	struct vfs_lookup_s *lkp; struct vfs_lookup_s _lkp; lkp = &_lkp;
	struct vfs_inode_ref_s *current;

START_LOOKUP:

	if(lkp_path->isAbsolute)
		current = &current_task->vfs_root.f_inode;
	else
		current = &cwd->f_inode;

	lkp->current = *current;
	lkp->flags = lkp_path->flags;
	lkp->mode = lkp_path->mode;
	/* The first element is always held (not in a multi-thread env) */
	lkp->lookup_flags = lookup_flags | VFS_LOOKUP_HELD;
	lkp->th_uid = current_thread->task->uid;
	lkp->th_gid = current_thread->task->gid;;

	lkr->inode.cid = CID_NULL;
	lkr->inode.ptr = NULL;
	lkr->inode.gc = 0;
	lkr->ctx = NULL;
	lkr->err = 0;
	lkr->lookup_flags = VFS_LOOKUP_HELD;
	
	for(; *lkp_path->cptr;)
	{
		if(*(lkp_path->cptr+1) == NULL)
		{
			VFS_SET(lkp->lookup_flags, VFS_LOOKUP_LAST);
			if(lkp->lookup_flags & VFS_LOOKUP_PARENT)
			{
				if(!(lkr->inode.ptr))
					lkr->inode = lkp->current;
				return 0;
			}
		}

		vfs_lookup_elem(lkp, lkr, *lkp_path->cptr);

		if(lkr->err)
		{
			vfs_dmsg(1, "%s : lookup failed, path = %s\n", __FUNCTION__, lkp_path->str);
			return lkr->err;
		}
	
		if(VFS_IS(lkr->lookup_flags, VFS_LOOKUP_RETRY))
			continue;

		if(VFS_IS(lkr->lookup_flags, VFS_LOOKUP_RESTART))
			goto START_LOOKUP;

		if(!lkr->ctx)
			lkp->current = lkr->inode;
		else
			lkp->current = lkr->ctx->ctx_root;
		VFS_CLEAR(lkp->lookup_flags, VFS_LOOKUP_HELD);
		vfs_lookup_next(lkp_path);
	}

        /* FIXME : possible SIGSEGV when dereferencing ptr ? */
	vfs_dmsg(1, "%s : lookup succeded, path = %s, inode num %d\n", 
		__FUNCTION__, lkp_path->str, lkr->inode.ptr->i_number);

	return 0;
}

error_t vfs_lookup_path(struct vfs_file_s *cwd, 
			struct vfs_lookup_path_s *path, 
			uint32_t lookup_flags, 
			struct vfs_lookup_response_s *lkr)
{
	return __vfs_lookup(cwd, path, lookup_flags, lkr);
}

error_t vfs_lookup(struct vfs_file_s *cwd, struct ku_obj *path, uint_t flags, 
					uint_t mode, uint32_t lookup_flags, 
					struct vfs_lookup_response_s *lkr)
{
	struct vfs_lookup_path_s lkp_path;
	error_t err;

	err = vfs_lookup_path_init(&lkp_path, path, flags, mode, NULL);

	if(err) return err;

	err = __vfs_lookup(cwd, &lkp_path, lookup_flags, lkr);

	vfs_lookup_path_put(&lkp_path);

	return err;
}
