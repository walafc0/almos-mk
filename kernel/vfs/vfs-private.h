/*
 * vfs/vfs-private.h - vfs private helper functions
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

#ifndef _VFS_PRIVATE_H_
#define _VFS_PRIVATE_H_

#include <spinlock.h>
#include <list.h>
#include <mapper.h>
#include <vfs-params.h>

struct vfs_inode_ref_s;
struct vfs_lookup_response_s;
struct vfs_context_s;
struct vfs_dirent_s;
struct vfs_inode_s;
struct vfs_file_s;
union vfs_file_private_s;

struct vfs_freelist_s 
{
	spinlock_t lock;			/* FIXME: should be spin_rwlock */
	struct list_entry root;
};

extern struct vfs_freelist_s vfs_freelist;

KMEM_OBJATTR_INIT(vfs_kmem_context_init);
KMEM_OBJATTR_INIT(vfs_kmem_inode_init);
KMEM_OBJATTR_INIT(vfs_kmem_dirent_init);
KMEM_OBJATTR_INIT(vfs_kmem_file_remote_init);

/* VFS NODE FREELIST MANIPULATION */
error_t vfs_cache_init();
struct vfs_dirent_s* vfs_dirent_new(struct vfs_context_s *ctx, char* name);
//void vfs_node_freelist_add (struct vfs_inode_s *node, uint_t hasError);
//struct vfs_inode_s* vfs_node_freelist_get (struct vfs_context_s* parent_ctx);
//void vfs_node_freelist_unlink(struct vfs_inode_s *node);

struct vfs_file_freelist_s
{
	spinlock_t lock;
	struct list_entry root;
};

extern struct list_entry vfs_filelist_root;
extern struct vfs_file_freelist_s vfs_file_freelist;

/* VFS FILE FREELIST MANIPULATION */
struct vfs_file_s* vfs_file_get(struct vfs_context_s *ctx,
				struct vfs_file_s *file,
				struct vfs_file_remote_s *fremote,
				struct mapper_s *mapper,
				struct vfs_inode_ref_s *inode, 
				union vfs_file_private_s *priv, 
					cid_t home_cid);
struct vfs_file_remote_s* 
vfs_file_remote_get(struct vfs_inode_s *inode);

sint_t vfs_file_remote_down(struct vfs_file_remote_s *file);

/* dirent/inodes references count */
void vfs_dirent_up(struct vfs_dirent_s *dirent);
void vfs_dirent_down(struct vfs_dirent_s *dirent);
void vfs_inode_up(struct vfs_inode_s *inode);
void vfs_inode_down(struct vfs_inode_s *inode);
size_t vfs_inode_size_get(struct vfs_inode_s *inode);
error_t vfs_inode_down_remote(struct vfs_inode_s *inode, cid_t inode_cid);


size_t vfs_inode_size_get_remote(struct vfs_inode_s *inode, cid_t inode_cid);

#if 0
#define vfs_node_up_atomic(_node)				\
	do{							\
		spinlock_lock(&vfs_node_freelist.lock);		\
		vfs_node_up(_node);				\
		spinlock_unlock(&vfs_node_freelist.lock);	\
	}while(0)

#define vfs_node_down_atomic(_node)				\
	do{							\
		spinlock_lock(&vfs_node_freelist.lock);		\
		vfs_node_down(_node);				\
		spinlock_unlock(&vfs_node_freelist.lock);	\
	}while(0)
#endif

/************************* Lookup *******************************/
struct ku_obj;


struct vfs_lookup_path_s
{
	char *str;
	char **cptr;
	uint_t mode;
	uint_t flags;
	char **path_ptrs;
	bool_t isAbsolute;

	//containers
	char *__path_ptrs[VFS_MAX_PATH_DEPTH];
	struct page_s *page;
};

error_t vfs_lookup(struct vfs_file_s *cwd, 
			struct ku_obj *path, uint_t flags, 
			uint_t mode, uint32_t lookup_flags, 
			struct vfs_lookup_response_s *lkr);

error_t vfs_lookup_path(struct vfs_file_s *cwd, 
			struct vfs_lookup_path_s *path, 
			uint32_t lookup_flags, 
			struct vfs_lookup_response_s *lkr);

//a _hold does not require a put, while a _get require a put
struct vfs_inode_s* vfs_lookup_hold_ref(struct vfs_inode_ref_s *ref);
struct vfs_inode_s* vfs_lookup_get_ref(struct vfs_inode_ref_s *ref);
void vfs_lookup_put_ref(struct vfs_inode_ref_s *lkp, struct vfs_inode_s *inode);

error_t vfs_load_dirent(struct vfs_inode_s *parent, char* name, 
				uint_t flags, char isLast, char setLoad,
				struct vfs_dirent_s **ret_dirent, 
				uint_t* lookup_flags);

error_t vfs_lookup_path(struct vfs_file_s *cwd, 
			struct vfs_lookup_path_s *path, 
			uint32_t lookup_flags, 
			struct vfs_lookup_response_s *lkr);
error_t vfs_lookup_path_init(struct vfs_lookup_path_s *lkp_path, 
				struct ku_obj *path, uint_t flags, 
					uint_t mode, uint_t* last);
void vfs_lookup_path_put(struct vfs_lookup_path_s *lkp_path);


#endif	/* _VFS_PRIVATE_H_ */
