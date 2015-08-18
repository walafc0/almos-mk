/*
 * vfs/vfs_cache.c - vfs cache operations
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

/* FIXME
 * Synchro rules:
 * The root inode has a reference count of 1+n one for
 * the ctx pointer to it and n for his dirent childrens.
 *
 * A "not open" dirent has a reference count of one as
 * long as he is in the parent inode hash table.
 * His inode has a reference count of 1+n, one for
 * the dentry pointer and n for his children.
 *
 * An open dirent has a refcount 1+n where n is the 
 * number of openner. His inode r.f. is the same as the one 
 * of the "not open" dirent. (note that we don't count the
 * inode pointer to his dentry as a reference.
 * 
 * A "not open" dirent is put in the freelist without 
 * removing it from the parent htable and by locking 
 * only the freelist lock. If the dentry is to be opened
 * no need to remove it from the freelist: lazy list ?
 *
 * A search of the dirent htable is done by locking the 
 * parent lock. Also, to remove/insert a dentry from 
 * his parent we lock the parent.
 *
 * order of locks: free_list lock then parent lock
 *
 * Open ref on dirent or on inode ?
 */


#include <atomic.h>
#include <vfs.h>
#include <task.h>
#include <htable.h>
#include <kmem.h>
#include <vfs-private.h>
#include <thread.h>
#include <cluster.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////// COMMON //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

error_t vfs_dcache_init(uint_t length);
error_t vfs_icache_init(uint_t length);

error_t vfs_cache_init()
{
	error_t err;

	err = vfs_dcache_init(VFS_MAX_NODE_NUMBER);
	if(err) return err;
	err = vfs_icache_init(VFS_MAX_NODE_NUMBER);

	return err;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Inode cache /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
	
spinlock_t icache_lock;
struct hheader_s icache;
struct list_entry inode_freelist;

struct icache_key
{
	uint_t inum;
	struct vfs_context_s *ctx;
};

//TODO: better hash ?
hash_t inode_hhash(void* key)
{
	uint_t next;
	struct icache_key *ikey; 
	ikey = key;

	next = ikey->inum * (uint_t)ikey->ctx;
	next = next * 1103515245 + 12345;
        return((unsigned)(next/65536) % 32768);
}

bool_t inode_hcompare(struct hnode_s *hn, void* key)
{
	struct vfs_inode_s *inode;
	struct icache_key *ikey; 

	ikey = key;
	
	assert(ikey->ctx);

	inode = container_of(hn, struct vfs_inode_s, i_hnode);

	if((inode->i_number == ikey->inum) && (inode->i_ctx == ikey->ctx))
		return true;
	else
		return false;
}

error_t vfs_icache_init(uint_t length)
{
	error_t err;
	if((err = hhalloc(&icache, AF_BOOT)))
		return err;
	if((err = hhinit(&icache, inode_hhash, inode_hcompare)))
		return err;

	spinlock_init(&icache_lock, "ICache");

	list_root_init(&inode_freelist);
	
	return 0;
}

void vfs_icache_lock()
{
	spinlock_lock(&icache_lock);
}

void vfs_icache_unlock()
{
	spinlock_unlock(&icache_lock);
}

error_t vfs_icache_add(struct vfs_inode_s *inode)
{
	struct icache_key ikey;
	error_t err;
	
	ikey.inum = inode->i_number;
	ikey.ctx = inode->i_ctx;

	assert(atomic_get(&inode->i_count) == 0);

	vfs_icache_lock();
	err = hinsert(&icache, &inode->i_hnode, (void*)&ikey);
	if(!err)
                list_add_last(&inode_freelist, &inode->i_freelist);
	vfs_icache_unlock();

	return err;
}

error_t __vfs_icache_del(struct vfs_inode_s *inode)
{
	struct icache_key ikey;
	bool_t done;

	ikey.inum = inode->i_number;
	ikey.ctx = inode->i_ctx;

	done = hremove(&icache, (void*)&ikey);

	return !done;
}

error_t vfs_icache_del(struct vfs_inode_s *inode)
{
	struct icache_key ikey;
	error_t err;

	ikey.inum = inode->i_number;
	ikey.ctx = inode->i_ctx;

	vfs_icache_lock();
	err = hremove(&icache, (void*)&ikey);
	vfs_icache_unlock();

	return err;
}




// Return an new inode number.
// Each cluster has it's own range...
uint32_t vfs_inum_new(struct vfs_context_s *ctx)
{
	uint32_t new;
	uint32_t nbits;
	uint32_t inode_number_max;

	assert(sizeof(cid_t) < sizeof(uint32_t));
	nbits = (sizeof(uint32_t) - sizeof(cid_t)) * 8;


	new = atomic_add(&ctx->ctx_inum_count, 1);
	new |= current_cid << nbits;//put current cid in MSB

	inode_number_max = ((current_cid+1) << nbits) - 1;
	assert(new < inode_number_max);//FIXME: properly handle this case (return an error: max file reached ?) 
						//or dynamically allocate inode range ...
	
	vfat_dmsg(1,"%s: (cid %d) New inode number 0x%x\n", __FUNCTION__, current_cid, new);

	return new;
}

void vfs_inumber_free(struct vfs_context_s *ctx, uint_t inum)
{
	//FIXME: use a table to free the inode numbers ?
}

//FIXME: put in constructor ?
error_t vfs_inode_init(struct vfs_inode_s *inode, struct vfs_context_s *ctx)
{
	inode->i_flags  = 0;
	inode->i_attr   = 0;
	inode->i_state  = 0;
	inode->i_size   = 0;
	inode->i_links  = 0;
	inode->i_gc     = 0;
	//inode->i_readers = 0;
	//inode->i_writers = 0;
	inode->i_op     = ctx->ctx_inode_op;
	inode->i_ctx    = ctx;
	inode->i_dirent = NULL;
	inode->i_pcid   = CID_NULL;
	inode->i_parent = NULL;
	inode->i_pv     = NULL;
	inode->i_type   = ctx->ctx_type;
	inode->i_mount_ctx  = NULL;

	atomic_init(&inode->i_count, 0);

	return inode->i_op->init(inode);
}

//TODO
struct vfs_inode_s* vfs_inode_new(struct vfs_context_s* ctx)
{
	struct vfs_inode_s *inode;

	kmem_req_t req;
	req.type  = KMEM_VFS_INODE;
	req.size  = sizeof(*inode);
	req.flags = AF_USR;//?

	inode = kmem_alloc(&req);
	if(!inode) return NULL;

	vfs_dmsg(1, "New inode %p\n", inode);

	if(vfs_inode_init(inode, ctx))
		return NULL;

	return inode;
}

void vfs_inode_free(struct vfs_inode_s *inode)
{
	kmem_req_t req;
	req.type  = KMEM_VFS_INODE;
	req.size  = sizeof(*inode);
	req.ptr = inode;
	
	kmem_free(&req);
}


static void vfs_inode_ctor(struct kcm_s *kcm, void *ptr)
{
	struct vfs_inode_s *inode;
	inode = (struct vfs_inode_s*)ptr;

	hninit(&inode->i_hnode);
	rwlock_init(&inode->i_rwlock);
	wait_queue_init(&inode->i_wait_queue, "VFS Inode");
	metafs_init(&inode->i_meta, "Inode dirent cache");
}

KMEM_OBJATTR_INIT(vfs_kmem_inode_init)
{
	attr->type   = KMEM_VFS_INODE;
	attr->name   = "KCM VFS Inode";
	attr->size   = sizeof(struct vfs_inode_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VFS_INODE_MIN;
	attr->max    = CONFIG_VFS_INODE_MAX;
	attr->ctor   = vfs_inode_ctor;
	attr->dtor   = NULL;

	return 0;
}

error_t vfs_inode_trunc(struct vfs_inode_s *inode)
{
	error_t err;

	err = 0;
  
	if((inode->i_size == 0)       ||
	   (inode->i_attr & VFS_DEV)  || 
	   (inode->i_attr & VFS_FIFO) || 
	   (inode->i_attr & VFS_DIR))
		return 0;			/* Ingored */

	rwlock_wrlock(&inode->i_rwlock);
	//FIXME: synchro and free cache page ?
	inode->i_size = 0;
	if(inode->i_op->trunc)
		err = inode->i_op->trunc(inode);
	rwlock_unlock(&inode->i_rwlock);
  

	return err;
}


void __vfs_inode_up(struct vfs_inode_s *inode)
{
	sint_t count;

	if(!inode) return;

	count = atomic_inc(&inode->i_count);	
	if(count == 0)
		list_unlink(&inode->i_freelist);
}

void vfs_inode_up(struct vfs_inode_s *inode)
{
	vfs_icache_lock();
	__vfs_inode_up(inode);
	vfs_icache_unlock();
}

struct vfs_inode_s* __vfs_inode_get(uint_t inumber, struct vfs_context_s *ctx)
{
	struct vfs_inode_s *inode;
	struct icache_key ikey;
	struct hnode_s *hn;

	inode = NULL;
	ikey.inum = inumber;
	ikey.ctx = ctx;

	hn = hfind(&icache,(void*) &ikey);
	if(!hn) return NULL;
	return list_element(hn, struct vfs_inode_s, i_hnode);
}

struct vfs_inode_s* vfs_inode_get(uint_t inumber, struct vfs_context_s *ctx)
{
	struct vfs_inode_s *inode;
	vfs_icache_lock();
	inode = __vfs_inode_get(inumber, ctx);
	__vfs_inode_up(inode);
	vfs_icache_unlock();

	return inode;
}

void vfs_inode_down(struct vfs_inode_s *inode)
{
	sint_t count;
	
	if(!inode) return;

	vfs_icache_lock();
	count = atomic_dec(&inode->i_count);
	assert(count > 0);
	if(count == 1)
	{
		list_add_last(&inode_freelist, &inode->i_freelist);
	}
	vfs_icache_unlock();
}

//put inode file(?)
RPC_DECLARE(__inode_down, 
		RPC_RET_PTR(error_t, err), 
		RPC_ARG_VAL(struct vfs_inode_s*, inode))
{
	vfs_inode_down(inode);
	
}

//put inode file(?)
error_t vfs_inode_down_remote(struct vfs_inode_s *inode, cid_t inode_cid)
{
	error_t err;
	RCPC(inode_cid, RPC_PRIO_FS, __inode_down, 
			RPC_RECV(RPC_RECV_OBJ(err)), 
			RPC_SEND(RPC_SEND_OBJ(inode)));

	return err;
}

error_t vfs_inode_hold(struct vfs_inode_s *inode, gc_t igc)
{
	vfs_inode_up(inode);
	if(inode->i_gc != igc)
	{
		vfs_inode_down(inode);
		return -1;
	}
	return 0;
}

error_t vfs_inode_check(struct vfs_inode_s *inode, gc_t igc)
{
	return inode->i_gc != igc;
}

error_t vfs_inode_lock(struct vfs_inode_s *inode)
{
	rwlock_wrlock(&inode->i_rwlock);
	return 0;
}

error_t vfs_inode_unlock(struct vfs_inode_s *inode)
{
	rwlock_unlock(&inode->i_rwlock);
	return 0;
}

size_t vfs_inode_size_get(struct vfs_inode_s *inode)
{
	return inode->i_size;	
}

//put inode file(?)
RPC_DECLARE(__inode_size_get, 
		RPC_RET(RPC_RET_PTR(size_t, sz)), 
		RPC_ARG(RPC_ARG_VAL(struct vfs_inode_s*, inode)))
{
	*sz = vfs_inode_size_get(inode);
}

//put inode file(?)
size_t vfs_inode_size_get_remote(struct vfs_inode_s *inode, cid_t inode_cid)
{
	size_t size;
	RCPC(inode_cid, RPC_PRIO_FS, __inode_size_get,
				RPC_RECV(RPC_RECV_OBJ(size)), 
				RPC_SEND(RPC_SEND_OBJ(inode)));

	return size;
}

cid_t vfs_inode_get_cid(uint_t ino)
{
	return ino%current_cluster->clstr_wram_nr;
}

RPC_DECLARE(__vfs_delete_inode, 
		RPC_RET(RPC_RET_PTR(error_t, err)),
		RPC_ARG(
		RPC_ARG_VAL(uint_t, inum),
		RPC_ARG_VAL(struct vfs_context_s*, ctx)
		))
{
	struct vfs_inode_s *inode;
	*err = 0;

	vfs_icache_lock();
	inode = __vfs_inode_get(inum, ctx);
	if(!inode) goto UNLK_ERR_EXIT;

	if(atomic_get(&inode->i_count) != 0)
	{
		*err = EBUSY;
		goto UNLK_ERR_EXIT;
	}
	
	if(--inode->i_links == 0)
	{
		if((*err = inode->i_op->delete(inode)))
			goto UNLK_ERR_EXIT;

		//TODO: set inode inload flag to avoid locking the hole cache	
		if(__vfs_icache_del(inode))
			assert("This should not happen since the cache is locked" && 0);
		
	}

UNLK_ERR_EXIT:	
	vfs_icache_unlock();
}

//TODO: use ptr rather than inumber
error_t vfs_delete_inode(uint_t inum, struct vfs_context_s *ctx, cid_t icid)
{
	error_t err;
	RCPC(icid, RPC_PRIO_FS_STAT, __vfs_delete_inode,
			RPC_RECV(RPC_RECV_OBJ(err)),
			RPC_SEND(RPC_SEND_OBJ(inum), RPC_SEND_OBJ(ctx)));
	return err;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Dirent cache ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

struct vfs_freelist_s dirent_freelist;
error_t vfs_freelist_init(struct vfs_freelist_s *freelist, uint_t length)
{
       list_root_init(&freelist->root);
       spinlock_init(&freelist->lock, "VFS Dirent Freelist");
       return 0;
}

error_t vfs_dcache_init(uint_t length)
{
	return vfs_freelist_init(&dirent_freelist, length);
}

void vfs_dirent_free(struct vfs_dirent_s *dirent)
{
	//TODO
}

void vfs_dirent_init(struct vfs_dirent_s *dirent, struct vfs_context_s *ctx, char* name)
{
	dirent->d_op     = ctx->ctx_dirent_op;
	dirent->d_ctx    = ctx;
	dirent->d_parent = NULL;
	dirent->d_inode  = (const struct vfs_inode_ref_s){ 0 };
	dirent->d_mounted = NULL;

	dirent->d_inum	 = 0;
	dirent->d_attr	 = 0;

	atomic_init(&dirent->d_count, 1);
	list_entry_init(&dirent->d_freelist);
	assert(strlen(name) <= VFS_MAX_NAME_LENGTH); 
	strcpy(&dirent->d_name[0], name);
	metafs_init(&dirent->d_meta, dirent->d_name);
}

struct vfs_dirent_s* vfs_dirent_new(struct vfs_context_s *ctx, char* name)
{
	struct vfs_dirent_s *dirent;
//TODO
#if 0
	struct vfs_context_s *ctx;
	struct list_entry *iter;

	ctx = inode->i_ctx;

	spinlock_lock(dirent_freelist.lock):

	list_for_each(&dirent_freelist.root, iter)
	{
		dirent = list_element(iter, struct vfs_dirent_s, d_freelist);
		if

	}
	
	spinlock_unlock(dirent_freelist.lock):

	return dirent;
#endif
	kmem_req_t req;
	req.type  = KMEM_VFS_DIRENT;
	req.size  = sizeof(struct vfs_dirent_s);
	req.flags = AF_USR;//?

	dirent = kmem_alloc(&req);
	if(!dirent) return NULL;

	vfs_dirent_init(dirent, ctx, name);
	return dirent;
}

static void vfs_dirent_ctor(struct kcm_s *kcm, void *ptr)
{
	//struct vfs_dirent_s *dirent;

	//dirent = (struct vfs_dirent_s*)ptr;
	//rwlock_init(&dirent->d_rwlock);
	//wait_queue_init(&dirent->d_wait_queue, "VFS Dirent");
}

KMEM_OBJATTR_INIT(vfs_kmem_dirent_init)
{
	attr->type   = KMEM_VFS_DIRENT;
	attr->name   = "KCM VFS Dirent";
	attr->size   = sizeof(struct vfs_dirent_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VFS_INODE_MIN;
	attr->max    = CONFIG_VFS_INODE_MAX;
	attr->ctor   = vfs_dirent_ctor;
	attr->dtor   = NULL;

	return 0;
}

void vfs_dirent_up(struct vfs_dirent_s *dirent)
{
	sint_t count;

	if(!dirent) return;

	spinlock_lock(&dirent_freelist.lock);
	count = atomic_inc(&dirent->d_count);
	if(count == 0)
		list_unlink(&dirent->d_freelist);
	spinlock_unlock(&dirent_freelist.lock);
}

void vfs_dirent_down(struct vfs_dirent_s *dirent)
{
	uint_t count;

	if(!dirent) return;

	spinlock_lock(&dirent_freelist.lock);
	count = atomic_dec(&dirent->d_count);
	if(count == 1) 
		list_add_last(&dirent_freelist.root, &dirent->d_freelist);
	spinlock_unlock(&dirent_freelist.lock);
}

void vfs_dirent_set_inload(struct vfs_dirent_s *dirent)
{
	VFS_SET(dirent->d_state, VFS_INLOAD);
}


bool_t vfs_dirent_is_inload(struct vfs_dirent_s *dirent)
{
	return VFS_IS(dirent->d_state, VFS_INLOAD);//lock dirent?
}

void vfs_dirent_clear_inload(struct vfs_dirent_s *dirent)
{
	VFS_CLEAR(dirent->d_state, VFS_INLOAD);//lock dirent?
}


