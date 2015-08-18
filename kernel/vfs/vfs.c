/*
 * vfs/vfs.c - Virtual File System services
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

#include <scheduler.h>
#include <rwlock.h>
#include <string.h>
#include <stdint.h>
#include <vfs.h>
#include <vfs-private.h>
#include <ku_transfert.h>
#include <thread.h>
#include <task.h>
#include <page.h>
#include <ppm.h>
#include <cpu-trace.h>
#include <kmem.h>


struct __vfs_stat_s
{
	struct vfs_stat_s buff;
	uint32_t lookup_flags;
};

RPC_DECLARE(__vfs_stat, RPC_RET(RPC_RET_PTR(error_t, err), 
		RPC_RET_PTR(struct __vfs_stat_s, stat)),  
		RPC_ARG(
		RPC_ARG_PTR(struct vfs_inode_ref_s, ref))
		)
{
	struct vfs_inode_s *inode;
		
	inode = ref->ptr;
	if(vfs_inode_hold(inode, ref->gc))
	{
		VFS_SET(stat->lookup_flags, VFS_LOOKUP_RESTART);
		return;
	}

	if(inode->i_op->stat == NULL)
	{
		*err = ENOSYS;
		return;
	}

	*err = inode->i_op->stat(inode);

	if(*err) return;

	memcpy(&stat->buff, &inode->i_stat, sizeof(inode->i_stat));
	
	vfs_lookup_put_ref(ref, inode);
	*err = 0;
}
	

error_t vfs_stat(struct vfs_file_s *cwd, struct ku_obj *path, struct vfs_stat_s* ustat, struct vfs_file_s *file)
{
	struct vfs_lookup_response_s lkr;
	struct __vfs_stat_s stat;
	cid_t inode_cid;
	error_t err;

	vfs_dmsg(1,"%s: called, isByPath %d\n", __FUNCTION__, !!file);

	do{
		if(!file)
		{
			err = vfs_lookup(cwd, path, 0, 0, 0, &lkr);
	 
			if(err) return err;

			inode_cid = lkr.inode.cid;
		}else
		{
			lkr.inode = file->f_inode;
			inode_cid = file->f_inode.cid;
		}
	  
		RCPC(inode_cid, RPC_PRIO_FS_STAT, __vfs_stat,
				RPC_RECV(RPC_RECV_OBJ(stat), 
				RPC_RECV_OBJ(err)), 
				RPC_SEND(RPC_SEND_OBJ(lkr.inode)));

		if(err) return err;

	}while(VFS_IS(stat.lookup_flags, VFS_LOOKUP_RESTART));

	err = cpu_copy_to_uspace(ustat, &stat.buff, sizeof(struct vfs_stat_s));
  
	return err;
}

error_t __vfs_lseek(struct vfs_file_remote_s *fremote, 
	size_t offset, uint_t whence, size_t *new_offset_ptr);

struct __vfs_open_s
{
	uint_t attr;
	uint_t lookup_flags;
	struct mapper_s *mapper;
	struct vfs_context_s *ctx;
	struct vfs_inode_ref_s inode;
	struct vfs_file_remote_s *fremote;
	union vfs_file_private_s priv;
};

//TODO check right
RPC_DECLARE(__vfs_open, 
		RPC_RET(RPC_RET_PTR(error_t, err), 
		RPC_RET_PTR(struct __vfs_open_s, open)),  

		RPC_ARG(
		RPC_ARG_PTR(struct vfs_inode_ref_s, ref),
		RPC_ARG_VAL(uint_t, flags)
		))
{
	struct vfs_file_remote_s *fremote;
	struct vfs_inode_s *inode;

	open->lookup_flags = 0;
	*err = 0;
	
	inode = ref->ptr;
	if(vfs_inode_hold(inode, ref->gc))
	{
		VFS_SET(open->lookup_flags, VFS_LOOKUP_RESTART);
		return;
	}

	if((flags & VFS_DIR) && !(inode->i_attr & VFS_DIR))
	{
		*err = ENOTDIR;
		goto OPEN_OUT_ERROR;
	}
	
	if(!(fremote = vfs_file_remote_get(inode)))
	{
		*err = ENOMEM;
		goto OPEN_OUT_ERROR;
	}

	/*
	file_ptr->f_flags = flags & 0xFFFF0000;
	file_ptr->f_mode  = mode;
	file_ptr->f_attr  = attr;

	if(VFS_IS(attr, VFS_DEV_BLK | VFS_DEV_CHR))
		VFS_SET(file_ptr->f_flags, VFS_O_DEV);
	*/

	if((*err = fremote->fr_op->open(fremote, &open->priv)))
		goto VFS_OPEN_ERROR;

	if(VFS_IS(flags, VFS_O_APPEND))
		if((*err = __vfs_lseek(fremote, 0, VFS_SEEK_END, NULL)))
			goto VFS_OPEN_ERROR;
	
	if(VFS_IS(flags, VFS_DIR))
	{
		fremote->fr_offset = 0;
		//VFS_SET(fremote->f_flags, VFS_O_DIRECTORY);
	}

	//FIXME: do this only if we are sure that the file will opened
	if(VFS_IS(flags, VFS_O_TRUNC))
	{
		//FIXME: update fr_offset ?
		*err = vfs_inode_trunc(inode);
		if(*err)
		{
			goto VFS_OPEN_ERROR;
		}
	}

	//FIXME: why sent back the arg ?
	open->inode = *ref;
	open->fremote = fremote;
	open->attr = inode->i_attr;
	open->ctx = inode->i_ctx;
	open->mapper = inode->i_mapper;
	vfs_dmsg(1, "%s; inode %p, cid %d\n", __FUNCTION__, inode, current_cid);
	return;

VFS_OPEN_ERROR:
	fremote->fr_inode = NULL;/* to avoid inode_down */
	vfs_file_remote_down(fremote);
		
OPEN_OUT_ERROR:
	vfs_dmsg(1,"%s : error while doing open, code %d\n", __FUNCTION__, err);
	vfs_inode_down(inode);
}

error_t vfs_open(struct vfs_file_s *cwd,
		struct ku_obj *path,
		uint_t flags,
		uint_t mode,
		struct vfs_file_s *file) 
{
	struct vfs_lookup_response_s lkr;
	struct __vfs_open_s open;
	cid_t inode_cid;
	uint_t attr;
	error_t err;
	error_t err2;

	cpu_trace_write(current_cpu, vfs_open);
	
VFS_OPEN_START:


	err = vfs_lookup(cwd, path, flags, mode, VFS_LOOKUP_OPEN, &lkr);
	if(err) return err;
	inode_cid = lkr.inode.cid;

	err2 = RCPC(inode_cid,	RPC_PRIO_FS_LOOKUP, __vfs_open,
				RPC_RECV(RPC_RECV_OBJ(err), 
					RPC_RECV_OBJ(open)),
				RPC_SEND(RPC_SEND_OBJ(lkr.inode),
					RPC_SEND_OBJ(flags)
					));
	if(err || err2)	
		goto VFS_OPEN_ERROR;

	if(VFS_IS(open.lookup_flags, VFS_LOOKUP_RESTART))
	{
		vfs_dmsg(2, "%s : open restarting\n", __FUNCTION__);
		goto VFS_OPEN_START;
	}

	attr = open.attr;

	vfs_dmsg(1, "%s; inode %p, cid %d\n", __FUNCTION__, open.inode.ptr, inode_cid);

	file = vfs_file_get(open.ctx, file, open.fremote, 
				open.mapper, &open.inode, 
				&open.priv, inode_cid);
	assert(file);//FIXME

	file->f_flags = flags & 0xFFFF0000;
	file->f_mode  = mode;
	file->f_attr  = attr;

	if(VFS_IS(attr, VFS_DEV_BLK | VFS_DEV_CHR))
		VFS_SET(file->f_flags, VFS_O_DEV);

	if(VFS_IS(flags, VFS_DIR))
		VFS_SET(file->f_flags, VFS_O_DIRECTORY);

	vfs_dmsg(1, "%s done; inode %p, cid %d\n",                                      \
                        __FUNCTION__, file->f_inode, file->f_inode.cid);
	return 0;

VFS_OPEN_ERROR:
	return err ? err : err2;
}


error_t vfs_chdir(struct ku_obj *pathname, struct vfs_file_s *cwd) 
{
	struct vfs_file_s file;
	uint_t flags, mode;
	error_t err;

	cpu_trace_write(current_cpu, vfs_chdir);


	flags = VFS_DIR;
	mode = 0;

	err = vfs_open(cwd, pathname, flags,  mode, &file);

	if(err) return err;

	//close old cwd
	vfs_close(cwd, NULL);

	//copy new cwd
	*cwd = file;

	return 0;
}

RPC_DECLARE(__vfs_chmod, 
		RPC_RET(RPC_RET_PTR(error_t, err), 
		RPC_RET_PTR(uint_t, lookup_flags)),  
		RPC_ARG(
		RPC_ARG_PTR(struct vfs_inode_ref_s, ref),
		RPC_ARG_VAL(uint_t, mode)
		))
{
	struct vfs_inode_s *inode;

	*lookup_flags = 0;
	*err = 0;
	
	inode = ref->ptr;
	if(vfs_inode_hold(inode, ref->gc))
	{
		VFS_SET(*lookup_flags, VFS_LOOKUP_RESTART);
		return;
	}

	vfs_inode_lock(inode);

	inode->i_mode = mode;

	vfs_inode_unlock(inode);

	return;
}

error_t vfs_chmod(struct ku_obj *path, struct vfs_file_s *cwd, uint_t mode) 
{
	struct vfs_lookup_response_s lkr;
	cid_t inode_cid;
	uint_t flags;
	error_t err;
	error_t err2;

	cpu_trace_write(current_thread()->local_cpu, vfs_chmod);

VFS_CHMOD_START:

	flags = 0;
	err = vfs_lookup(cwd, path, flags, mode, VFS_LOOKUP_OPEN, &lkr);
	if(err) return err;
	inode_cid = lkr.inode.cid;

	err2 = RCPC(inode_cid,	RPC_PRIO_FS_LOOKUP, __vfs_chmod,
				RPC_RECV(RPC_RECV_OBJ(err), 
					RPC_RECV_OBJ(flags)),
				RPC_SEND(RPC_SEND_OBJ(lkr.inode),
					RPC_SEND_OBJ(mode)
					));
	if(err || err2)	
		goto VFS_CHMOD_ERROR;

	if(VFS_IS(flags, VFS_LOOKUP_RESTART))
	{
		vfs_dmsg(2, "%s : restarting\n", __FUNCTION__);
		goto VFS_CHMOD_START;
	}

	return 0;

VFS_CHMOD_ERROR:
	return err ? err : err2;
}

RPC_DECLARE(__vfs_get_name, 
		RPC_RET(RPC_RET_PTR(size_t, ret_size), 
			RPC_RET_PTR(struct vfs_dirent_s*, ret_dirent),
			RPC_RET_PTR(cid_t, ret_cid)),
		RPC_ARG(
			RPC_ARG_VAL(struct vfs_inode_s*, inode),
			RPC_ARG_VAL(struct vfs_dirent_s*, dirent),
			RPC_ARG_VAL(char*, tmp),
			RPC_ARG_VAL(size_t, tmp_len)))
{
	size_t len;

	assert(inode || dirent);

	len = 0;

	if(dirent)
	{
		if(!inode) inode = dirent->d_parent;

		len = strlen(dirent->d_name);
		len = MIN(len, tmp_len);
		//copy to the end of the buffer
		remote_memcpy(&tmp[tmp_len - len], RPC_SENDER_CID, dirent->d_name, current_cid, len);
	}

	*ret_dirent = inode->i_dirent;
	*ret_cid = inode->i_pcid;
	*ret_size = len;
}

error_t vfs_get_path(struct vfs_file_s *file, struct ku_obj *buff)
{
	struct vfs_inode_s *current_inode;
	struct vfs_dirent_s *current_dirent;
	struct page_s *page;
	char* tmp_path;
	size_t tmp_size;
	cid_t next_cid;
	kmem_req_t req;
	error_t err;
	size_t len;
	size_t count;


	//allocate a temporary buffer
	req.type  = KMEM_PAGE;
	req.flags = AF_USER;
	req.size  = 0;

	if((page = kmem_alloc(&req)) == NULL)
	{
		return ENOMEM;
	}

	//prepare loop
	tmp_path = (char*)ppm_page2addr(page);
	tmp_path[PMM_PAGE_SIZE-1] = 0;
	tmp_size = PMM_PAGE_SIZE-1;
	count	 = 1;
	len	 = 0;
	err	 = 0;

	current_dirent = NULL;
	next_cid = file->f_inode.cid;
	current_inode = file->f_inode.ptr;

	//default result
	if(buff->get_size(buff) < 2)
		return EINVAL;//return value ?

	err = buff->scpy_to_buff(buff, "/", 2);

	do{
		err = RCPC(next_cid, RPC_PRIO_FS_LOOKUP, __vfs_get_name,
					RPC_RECV(RPC_RECV_OBJ(len), 
						RPC_RECV_OBJ(current_dirent),
						RPC_RECV_OBJ(next_cid)),
					RPC_SEND(RPC_SEND_OBJ(current_inode),
						RPC_SEND_OBJ(current_dirent),
						RPC_SEND_OBJ(tmp_path),
						RPC_SEND_OBJ(tmp_size)));

		//set variables for next loop
		current_inode = NULL;
		if(!len) continue;
		len++;
		tmp_size -= len;
		count += len;
		tmp_path[tmp_size] = '/';
	}while(current_dirent && !err);

	//copy to ku_obj
	if(!err)
		buff->scpy_to_buff(buff, &tmp_path[tmp_size], count);

	req.ptr = page;
	kmem_free(&req);
	return err;
}



//secondary
error_t vfs_pipe(struct vfs_file_s *pipefd[2]) {
#ifdef CONFIG_DRIVER_FS_PIPE
	/* FIXME: CODE HERE IS SOMEHOW OBSOLET */
	struct vfs_inode_s *node;
	struct vfs_file_s *fd_in;
	struct vfs_file_s *fd_out;

	spinlock_lock(&vfs_node_freelist.lock);

	if((node = vfs_node_freelist_get(vfs_pipe_ctx)) == NULL)
	{
		spinlock_unlock(&vfs_node_freelist.lock);
		return ENOMEM;
	}

	spinlock_unlock(&vfs_node_freelist.lock);

	VFS_SET(node->i_attr,VFS_PIPE);

	vfs_dmsg(1,"vfs_pipe: got a node, do n_op->init(node)\n");
	vfs_dmsg(1,"vfs_pipe: do n_op->init(node) OK\n");

	vfs_node_up(node);
	vfs_node_up(node);

	node->i_readers = 1;
	node->i_writers = 1;

	if((fd_in = vfs_file_get(node)) == NULL)
		goto VFS_PIPE_ERROR_FILE;

	if((fd_out = vfs_file_freelist_get(node)) == NULL) {
		vfs_file_freelist_add(fd_in);
		goto VFS_PIPE_ERROR_FILE;
	}

	VFS_SET(fd_in->f_flags, VFS_O_PIPE | VFS_O_WRONLY);
	VFS_SET(fd_out->f_flags,VFS_O_PIPE | VFS_O_RDONLY);
	pipefd[0] = fd_out;
	pipefd[1] = fd_in;
	return 0;

VFS_PIPE_ERROR_FILE:
	spinlock_lock(&vfs_node_freelist.lock);
	vfs_node_freelist_add(node,1);
	spinlock_unlock(&vfs_node_freelist.lock);
	return ENOMEM;

#else
	return ENOTSUPPORTED;
#endif
}


//secondary
error_t vfs_mkfifo(struct vfs_file_s *cwd, struct ku_obj *pathname, uint_t mode) {
#ifdef CONFIG_DRIVER_FS_PIPE
	struct vfs_inode_s *node;
	uint_t flags;
	error_t err;
	char str[VFS_MAX_PATH];
	char *dirs_ptr[vfs_dir_count(pathname) + 1];
	uint_t isAbsolutePath;

	err   = 0;
	flags = 0;
	VFS_SET(flags, VFS_O_CREATE | VFS_O_EXCL | VFS_FIFO);
	strcpy(str,pathname);
	vfs_split_path(str,dirs_ptr);
	isAbsolutePath = (pathname[0] == '/') ? 1 : 0 ;

	if((err = vfs_node_load(cwd,dirs_ptr, flags, isAbsolutePath, &node)))
		return err;

	spinlcok_lock(&vfs_node_freelist.lock);
	vfs_node_down(node);
	spinlock_unlock(&vfs_node_freelist.lock);
	return 0;
#else
	return ENOTSUPPORTED;
#endif
}

RPC_DECLARE(__vfs_delete_dirent, 
		RPC_RET(RPC_RET_PTR(error_t, err),
			RPC_RET_PTR(uint_t, lookup_flags)),
		RPC_ARG(
		RPC_ARG_PTR(struct vfs_inode_ref_s, ref),
		RPC_ARG_PTR(char, name),
		RPC_ARG_VAL(uint_t, flags)
		))
{
	struct vfs_dirent_s *dirent;
	struct vfs_inode_s *parent;

	parent = ref->ptr;

	if(vfs_inode_hold(ref->ptr, ref->gc))
	{
		VFS_SET(*lookup_flags, VFS_LOOKUP_RESTART);
		goto VFS_DIRENT_DEL_EXIT2;
	}
	
	if((*err = vfs_load_dirent(parent, name, 0, true, true, &dirent, lookup_flags)))
		goto VFS_DIRENT_DEL_EXIT2;

	//FIXME: also check no root (in case of chroot)
	if(dirent->d_mounted)//mount point cannot be deleted
	{
		*err = EBUSY;
		goto VFS_DIRENT_DEL_EXIT;
	}

	if((*err = parent->i_op->unlink(parent, dirent, flags)))
	{
		vfs_dirent_clear_inload(dirent);
		cpu_wbflush();
		goto VFS_DIRENT_DEL_EXIT;
	}
	
	vfs_inode_lock(parent);
	metafs_unregister(&parent->i_meta, &dirent->d_meta);
	vfs_inode_unlock(parent);

VFS_DIRENT_DEL_EXIT:
	vfs_dirent_down(dirent);
VFS_DIRENT_DEL_EXIT2:
	vfs_lookup_put_ref(ref, parent);
}

error_t __vfs_unlink(struct vfs_file_s *cwd, struct ku_obj *path, uint_t flags)
{
	struct vfs_lookup_path_s lkp_path;
	struct vfs_lookup_response_s lkr;
	uint_t lookup_flags;
	uint_t last;
	uint_t mode;
	error_t err;
	char *name;

	mode = 0;//?

	err = vfs_lookup_path_init(&lkp_path, path, flags, mode, &last);

	if(((lkp_path.path_ptrs[last][0] == '.') && (lkp_path.path_ptrs[last][1] == '\0'))
	|| ((lkp_path.path_ptrs[last][0] == '.') && (lkp_path.path_ptrs[last][1] == '.')))
	{
		if((lkp_path.path_ptrs[last][1] == '\0')) err = EINVAL;
		else err = ENOTEMPTY;
		goto UNLK_ERR_EXIT;
	}

	if((lkp_path.path_ptrs[0][0] == '/') && (lkp_path.path_ptrs[0][1] == '\0'))
	{
		err = EBUSY;
		goto UNLK_ERR_EXIT;
	}

START_DELETE:
	err = vfs_lookup_path(cwd, &lkp_path, VFS_LOOKUP_PARENT, &lkr);
	if(err) goto UNLK_ERR_EXIT;

	name = *lkp_path.cptr;

RETRY_DELETE:
	RCPC(lkr.inode.cid, 
			RPC_PRIO_FS, 
			__vfs_delete_dirent, 
			RPC_RECV(RPC_RECV_OBJ(err), 
			RPC_RECV_OBJ(lookup_flags)),

			RPC_SEND(RPC_SEND_OBJ(lkr.inode),
			RPC_SEND_MEM(name, (strlen(name)+1)), 
			RPC_SEND_OBJ(flags))
			);

	if(VFS_IS(lookup_flags, VFS_LOOKUP_RETRY))
		goto RETRY_DELETE;

	if(VFS_IS(lookup_flags, VFS_LOOKUP_RESTART))
		goto START_DELETE;

UNLK_ERR_EXIT:	
	vfs_lookup_path_put(&lkp_path);
	return err;
}

error_t vfs_unlink(struct vfs_file_s *cwd, struct ku_obj *pathname) 
{
	uint_t flags;

	flags = 0;
	return __vfs_unlink(cwd, pathname, flags);
}

error_t vfs_rmdir(struct vfs_file_s *cwd, struct ku_obj *pathname) 
{
	uint_t flags;

	flags = VFS_DIR;
	return __vfs_unlink(cwd, pathname, flags);
}


error_t vfs_close(struct vfs_file_s *file, uint_t *refcount)
{
	uint_t count;

	cpu_trace_write(current_cpu, vfs_close);

	assert(file != NULL);

	count = vfs_file_down(file);

	if(refcount != NULL)
		*refcount = count;

	return 0;
}

error_t vfs_closedir(struct vfs_file_s *file, uint_t *refcount) 
{
	if(!(VFS_IS(file->f_flags, VFS_O_DIRECTORY)))
		return EBADF;

	return vfs_close(file, refcount);
}

error_t vfs_create(struct vfs_file_s *cwd,
		   struct ku_obj *path,
		   uint_t flags,
		   uint_t mode,
		   struct vfs_file_s *file) 
{
	flags &= 0xFFF80000;
	VFS_SET(flags, VFS_O_CREATE); 
	VFS_SET(flags, VFS_O_WRONLY);
	VFS_SET(flags, VFS_O_TRUNC);
	return vfs_open(cwd, path, flags, mode, file);
}

error_t vfs_mkdir(struct vfs_file_s *cwd, struct ku_obj *pathname, uint_t mode) 
{
	struct vfs_lookup_response_s lkr;
	uint_t flags;
	error_t err;

	cpu_trace_write(current_thread()->local_cpu, vfs_mkdir);

	flags=0;
	//FIXME: VFS_DIR or VFS_O_DIR (also fix at lookup)
	VFS_SET(flags, VFS_O_DIRECTORY | VFS_O_CREATE | VFS_O_EXCL | VFS_DIR);
	
	err = vfs_lookup(cwd, pathname, flags, mode, 0, &lkr);
	if(err) return err;

	return 0;
}

error_t vfs_opendir(struct vfs_file_s *cwd, struct ku_obj *path, uint_t mode, struct vfs_file_s *file) 
{
	error_t err;
	uint_t flags;

	cpu_trace_write(current_cpu, vfs_opendir);

	err   = 0;
	flags = 0x00;
	VFS_SET(flags, VFS_DIR);

	if((err = vfs_open(cwd,path,flags,mode,file)))
		return err;

	VFS_SET(file->f_flags, VFS_O_DIRECTORY);
	return 0;
}

RPC_DECLARE(__vfs_readdir, RPC_RET(RPC_RET_PTR(error_t, err)),  
	RPC_ARG(RPC_ARG_VAL(struct vfs_file_remote_s*, file),
		RPC_ARG_VAL(struct vfs_usp_dirent_s*, dir)))
{
	struct vfs_usp_dirent_s tdir;
	*err = file->fr_op->readdir(file, &tdir);
	if(!*err)
		remote_memcpy(dir, RPC_SENDER_CID, 
					&tdir, current_cid, 
					sizeof(tdir));
}

error_t _vfs_readdir(struct vfs_file_s *file, struct vfs_usp_dirent_s *dir)
{
	error_t err;
	cid_t cid;
	
	cid = file->f_inode.cid;

	if(cid == current_cid)
		err = file->f_op->readdir(file->f_remote, dir);
	else{
		RCPC(cid, RPC_PRIO_FS, __vfs_readdir,
				RPC_RECV(RPC_RECV_OBJ(err)), 
				RPC_SEND(RPC_SEND_OBJ(file->f_remote),
				RPC_SEND_OBJ(dir)));
	}

	return err;
}

error_t vfs_readdir(struct vfs_file_s *file, struct ku_obj *dirent) 
{
	error_t err = 0;
	struct vfs_usp_dirent_s tdir;

	cpu_trace_write(current_cpu, vfs_readdir);

	if(!(VFS_IS(file->f_flags, VFS_O_DIRECTORY)))
		return EBADF;

	err = _vfs_readdir(file, &tdir);
		
	if(!err)
		dirent->copy_to_buff(dirent, &tdir);

	return err;
}

ssize_t vfs_read(struct vfs_file_s *file, struct ku_obj * buffer) 
{
	ssize_t size;

	cpu_trace_write(current_cpu, vfs_read);

	if(VFS_IS(file->f_flags, VFS_O_DIRECTORY))
		return -EISDIR;

	if(!(VFS_IS(file->f_flags, VFS_O_RDONLY)))
		return -EBADF;

	if((size = file->f_op->read(file, buffer)) < 0)
		goto VFS_READ_ERROR;

	vfs_dmsg(1,"vfs_read: size_to_read %d, read %d\n",
			buffer->get_size(buffer), size);

VFS_READ_ERROR:
	return size;
}


ssize_t vfs_write(struct vfs_file_s *file, struct ku_obj *buffer) 
{
	ssize_t size;

	cpu_trace_write(current_cpu, vfs_write);

	if(VFS_IS(file->f_flags,VFS_O_DIRECTORY))
		return EINVAL;

	if(!(VFS_IS(file->f_flags,VFS_O_WRONLY)))
		return EBADF;

	if((size = file->f_op->write(file, buffer)) < 0)
		goto VFS_WRITE_ERROR;

	vfs_dmsg(1,"vfs_write: %d has been wrote\n", size);


VFS_WRITE_ERROR:
	return size;
}

error_t __vfs_lseek(struct vfs_file_remote_s *fremote, 
	size_t offset, uint_t whence, size_t *new_offset_ptr)
{
	uint64_t new_offset;
	size_t old_offset;
	size_t inode_size;
	uint_t err;

	err = 0;
	old_offset = fremote->fr_offset;
	new_offset = fremote->fr_offset;

	rwlock_wrlock(&fremote->fr_rwlock);

	inode_size = vfs_inode_size_get(fremote->fr_inode);

	vfs_dmsg(1,"vfs_lseek: started asked size %d\n",offset);

	switch(whence) 
	{
	case VFS_SEEK_SET:

		if(offset == old_offset)	/* Nothing to do ! */
			goto VFS_LSEEK_ERROR;

		new_offset = offset;
		break;

	case VFS_SEEK_CUR:
		new_offset += offset;
		break;

	case VFS_SEEK_END:
		new_offset = inode_size + offset;
		break;

	default:
		err = EINVAL;
		goto VFS_LSEEK_ERROR;
	}

	if( new_offset >= UINT32_MAX) 
	{
		err = EOVERFLOW;
		goto VFS_LSEEK_ERROR;
	}

	fremote->fr_offset = (size_t) new_offset;

	if((err=fremote->fr_op->lseek(fremote))) 
	{
		err = -err;
		fremote->fr_offset = old_offset;
		goto VFS_LSEEK_ERROR;
	}

	if(new_offset_ptr)
		*new_offset_ptr = new_offset;

VFS_LSEEK_ERROR:
	rwlock_unlock(&fremote->fr_rwlock);
	return err;
}

RPC_DECLARE(_vfs_lseek, RPC_RET(RPC_RET_PTR(error_t, err), 
		RPC_RET_PTR(size_t, new_offset_ptr)),  
		RPC_ARG(
		RPC_ARG_VAL(struct vfs_file_remote_s*, file),
		RPC_ARG_VAL(size_t, offset),
		RPC_ARG_VAL(uint_t, whence))
		)
{
	*err = __vfs_lseek(file, offset, whence, new_offset_ptr);
}


error_t vfs_lseek(struct vfs_file_s *file, size_t offset, uint_t whence, size_t *new_offset_ptr) 
{
	error_t err;

	cpu_trace_write(current_cpu, vfs_lseek);

	if(VFS_IS(file->f_flags, VFS_O_DIRECTORY))
		return EBADF;

	RCPC(file->f_inode.cid, RPC_PRIO_FS, _vfs_lseek,
				RPC_RECV(RPC_RECV_OBJ(err), RPC_RECV_OBJ(*new_offset_ptr)), 
				RPC_SEND(
					RPC_SEND_OBJ(file->f_remote),
					RPC_SEND_OBJ(offset),
					RPC_SEND_OBJ(whence)));

	return err;		
}
