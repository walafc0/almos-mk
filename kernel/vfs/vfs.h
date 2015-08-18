/*
 * vfs/vfs.h - Virtual File System interface
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

#ifndef _VFS_H_
#define _VFS_H_

#include <system.h>
#include <errno.h>
#include <types.h>
#include <atomic.h>
#include <rwlock.h>
#include <list.h>
#include <vfs-errno.h>
#include <vfs-params.h>
#include <vfs-private.h>
#include <ku_transfert.h>
#include <metafs.h>
#include <fat32.h>
#include <mapper.h>
#include <sysfs.h>
#include <devfs.h>
#include <htable.h>

struct vfs_inode_s;
struct vfs_inode_op_s;
struct vfs_dirent_op_s;
struct vfs_file_op_s;
struct vfs_context_op_s;
struct vm_region_s;
struct vfs_dirent_s;
struct vfs_lookup_request_s;
struct vfs_lookup_response_s;

//function to be executed remotly on the found dentry
#define VFS_LOOKUP_FUNC(n) error_t (n)(struct vfs_dirent_s *dint,		\
					struct vfs_lookup_request_s *lkq,	\
					struct  vfs_lookup_response_s *lks)

typedef VFS_LOOKUP_FUNC(vfs_lookup_func_t);

struct vfs_lookup_mount_s
{
	uint32_t type;
	struct device_s *dev;
};

struct vfs_lookup_request_s
{
	vfs_lookup_func_t *func;
	union
	{
		struct vfs_lookup_mount_s mount;
	};
};

#define VFS_INODE_REF_SET(_ref, _inode, _cid) do{_ref.ptr = _inode; _ref.cid = _cid; _ref.gc = _inode->i_gc;}while(0)
#define VFS_INODE_REF_COMPARE(_ref0, _ref1) ((_ref0.ptr == _ref1.ptr) && (_ref0.cid == _ref1.cid) && (_ref0.gc == _ref1.gc))
struct vfs_inode_ref_s
{
	struct vfs_inode_s *ptr; 
	cid_t cid; gc_t gc; 
};

struct vfs_lookup_response_s
{
	struct vfs_inode_ref_s inode;
	struct vfs_context_s *ctx;
	uint_t lookup_flags;
	error_t err;
};

struct vfs_lookup_s
{
	struct vfs_inode_ref_s current;
	uint_t lookup_flags;
	uint_t mode;
	uint_t flags;
	uint_t th_uid;
	uint_t th_gid;
};

struct vfs_usp_dirent_s
{
	uint32_t u_ino;
	uint32_t u_attr;
	uint8_t  u_name[VFS_MAX_NAME_LENGTH];
};

//this struct can migrate across cluster
//avoid local pointer or replicate them or
//precise the cluster in wich they are in
struct vfs_dirent_s
{
	char d_name[VFS_MAX_NAME_LENGTH];
	uint_t d_attr;//or i_attr
	uint_t d_state;//VFS_INLOAD ...
	atomic_t d_count;
	uint_t d_inum;
	struct vfs_inode_ref_s d_inode;
	struct vfs_context_s *d_ctx;
	struct metafs_s d_meta;//allow a search by name
	struct vfs_inode_s *d_parent;//always in same cluster as this dirent, holding the dirent make the parent stable
	struct vfs_context_s *d_mounted;//mounted fs context
	struct list_entry  d_freelist;
	struct vfs_dirent_op_s *d_op;
	//struct hnode_s d_hnode;//contain the ref_count
	//void *d_pv;
	union{
		struct vfat_dirent_s d_vfat;
	};
};

struct vfs_stat_s 
{
	uint_t    st_dev;     /* ID of device containing file */
	uint_t    st_ino;     /* inode number */
	uint_t    st_mode;    /* protection */
	uint_t    st_nlink;   /* number of hard links */
	uint_t    st_uid;     /* user ID of owner */
	uint_t    st_gid;     /* group ID of owner */
	uint_t    st_rdev;    /* device ID (if special file) */
	uint64_t  st_size;    /* total size, in bytes */
	uint_t    st_blksize; /* blocksize for file system I/O */
	uint_t    st_blocks;  /* number of 512B blocks allocated */
	time_t    st_atime;   /* time of last access */
	time_t    st_mtime;   /* time of last modification */
	time_t    st_ctime;   /* time of last status change */
};

struct vfs_context_s
{
	uint_t	ctx_type;
	uint_t	ctx_flags;
	atomic_t ctx_count;
	atomic_t ctx_inum_count; /* default inum allocator */
	struct device_s *ctx_dev;
	struct vfs_context_op_s *ctx_op;
	struct vfs_inode_op_s *ctx_inode_op;
	struct vfs_file_op_s *ctx_file_op;
	struct vfs_dirent_op_s *ctx_dirent_op;
	struct vfs_inode_ref_s ctx_root;
	union{
		struct vfat_context_s ctx_vfat;
		struct devfs_context_s ctx_devfs;
		struct sysfs_context_s ctx_sysfs;
	};
};

struct vfs_inode_s
{
	uint_t   i_number;
	uint_t   i_flags;
	uint_t   i_attr;
	uint_t   i_mode;
	uint_t   i_state;//?
	uint64_t i_size;
	uint_t   i_links;
	uint_t   i_type;
	uid_t    i_uid;
	gid_t    i_gid;
	uint_t   i_acl;
	gc_t	 i_gc;
	//Used by get_path)
	//If this is a dir: dirent is the pointer to the only dirent of
	//this dir (not hard links for directories).Note that this ptr 
	//is to be update in case it's renamed (cross-folder only ?)
	//If this is a file dirent point to the first dentrie who created this
	struct vfs_dirent_s *i_dirent; 
	struct vfs_context_s *i_mount_ctx;
	struct vfs_inode_s *i_parent; gc_t i_pgc; cid_t i_pcid;
	//for inode hatable cache
	//struct hnode_s i_hnode;
	//Dirents cache (son dentries; only if inode is dir)
	struct metafs_s i_meta;//allow a search by name
	struct rwlock_s i_rwlock;
	atomic_t i_count; 
	struct hnode_s i_hnode;
	struct wait_queue_s   i_wait_queue;
	struct vfs_inode_op_s *i_op;
	struct vfs_context_s *i_ctx;
	struct mapper_s   *i_mapper;
	struct list_entry  i_freelist;
	struct vfs_stat_s  i_stat;
	void    *i_pv;
};


struct vfs_file_remote_s
{
	uint_t fr_offset;
	uint_t fr_version;
	atomic_t fr_count;
	struct rwlock_s fr_rwlock;
	struct vfs_inode_s *fr_inode;
	struct vfs_file_op_s *fr_op;
	void *fr_pv;
};

union vfs_file_private_s
{
	void* dev;
};

#define VFS_FILE_IS_NULL(file) ((file).f_ctx == NULL)
/* All adresses in this struct must be valid cross clusters */
struct vfs_file_s
{
	uint_t f_flags;
	uint_t f_mode;
	uint_t f_attr;
	uint_t f_version;
	struct vfs_file_remote_s *f_remote;
	struct vfs_inode_ref_s f_inode;
	struct vfs_context_s *f_ctx;
	struct vfs_file_op_s *f_op;
	struct mapper_s f_mapper;
	union vfs_file_private_s f_private;
};


#define VFS_CREATE_CONTEXT(n)   error_t (n) (struct vfs_context_s *context)
#define VFS_DESTROY_CONTEXT(n)  error_t (n) (struct vfs_context_s *context)
#define VFS_READ_ROOT(n)        error_t (n) (struct vfs_context_s *context, struct vfs_inode_s *root)
#define VFS_REPLI_ROOT(n)       error_t (n) (struct vfs_context_s *context, struct vfs_inode_s *root)
#define VFS_WRITE_ROOT(n)       error_t (n) (struct vfs_context_s *context, struct vfs_inode_s *root)

typedef VFS_CREATE_CONTEXT(vfs_create_context_t);
typedef VFS_DESTROY_CONTEXT(vfs_destroy_context_t);
typedef VFS_READ_ROOT(vfs_read_root_t);
typedef VFS_REPLI_ROOT(vfs_repli_root_t);
typedef VFS_WRITE_ROOT(vfs_write_root_t);

struct vfs_context_op_s
{
	vfs_create_context_t *create;
	vfs_destroy_context_t *destroy;
	vfs_read_root_t *read_root;
	vfs_read_root_t *repli_root;
	vfs_write_root_t *write_root;
};

#define VFS_INIT_NODE(n)     error_t (n) (struct vfs_inode_s *inode)
#define VFS_WRITE_NODE(n)    error_t (n) (struct vfs_inode_s *inode)
#define VFS_RELEASE_NODE(n)  error_t (n) (struct vfs_inode_s *inode)
#define VFS_STAT_NODE(n)     error_t (n) (struct vfs_inode_s *inode)
#define VFS_TRUNC_NODE(n)    error_t (n) (struct vfs_inode_s *inode)
#define VFS_DELETE_NODE(n)   error_t (n) (struct vfs_inode_s *inode)
#define VFS_CREATE_NODE(n)   error_t (n) (struct vfs_inode_s *parent, \
					struct vfs_dirent_s *dirent)
#define VFS_LOOKUP_NODE(n)   error_t (n) (struct vfs_inode_s *parent, \
					struct vfs_dirent_s *dirent)
#define VFS_UNLINK_NODE(n)   error_t (n) (struct vfs_inode_s *parent, \
					struct vfs_dirent_s *dirent,  \
					uint_t flags)

typedef VFS_INIT_NODE(vfs_init_inode_t);
typedef VFS_CREATE_NODE(vfs_create_inode_t);
typedef VFS_LOOKUP_NODE(vfs_lookup_inode_t);
typedef VFS_WRITE_NODE(vfs_write_inode_t);
typedef VFS_RELEASE_NODE(vfs_release_inode_t);
typedef VFS_UNLINK_NODE(vfs_unlink_inode_t);
typedef VFS_STAT_NODE(vfs_stat_inode_t);
typedef VFS_TRUNC_NODE(vfs_trunc_inode_t);
typedef VFS_DELETE_NODE(vfs_delete_inode_t);

struct vfs_inode_op_s
{
	vfs_init_inode_t *init;
	vfs_create_inode_t *create;
	vfs_lookup_inode_t *lookup;
	vfs_write_inode_t *write;
	vfs_release_inode_t *release;
	vfs_unlink_inode_t *unlink;
	vfs_delete_inode_t *delete;
	vfs_stat_inode_t *stat;
	vfs_trunc_inode_t *trunc;
};

#define VFS_COMPARE_DIRENT(n) error_t (n) (char *first, char *second)

typedef VFS_COMPARE_DIRENT(vfs_compare_dirent_t) ;

struct vfs_dirent_op_s
{
	vfs_compare_dirent_t *compare;
};

#define VFS_OPEN_FILE(n)    error_t (n) (struct vfs_file_remote_s *file, union vfs_file_private_s *priv)
#define VFS_CLOSE_FILE(n)   error_t (n) (struct vfs_file_remote_s *file)
#define VFS_RELEASE_FILE(n) error_t (n) (struct vfs_file_remote_s *file)
#define VFS_LSEEK_FILE(n)   error_t (n) (struct vfs_file_remote_s *file)
#define VFS_READ_DIR(n)     error_t (n) (struct vfs_file_remote_s *file, \
					struct vfs_usp_dirent_s *dirent)

#define VFS_READ_FILE(n)    ssize_t (n) (struct vfs_file_s *file, struct ku_obj *buffer)
#define VFS_WRITE_FILE(n)   ssize_t (n) (struct vfs_file_s *file, struct ku_obj *buffer)
#define VFS_MMAP_FILE(n)    error_t (n) (struct vfs_file_s *file, struct vm_region_s *region)
#define VFS_MUNMAP_FILE(n)  error_t (n) (struct vfs_file_s *file, struct vm_region_s *region)


typedef VFS_OPEN_FILE(vfs_open_file_t);
typedef VFS_READ_FILE(vfs_read_file_t);
typedef VFS_WRITE_FILE(vfs_write_file_t);
typedef VFS_LSEEK_FILE(vfs_lseek_file_t);
typedef VFS_CLOSE_FILE(vfs_close_file_t);
typedef VFS_RELEASE_FILE(vfs_release_file_t);
typedef VFS_READ_DIR(vfs_read_dir_t);
typedef VFS_MMAP_FILE(vfs_mmap_file_t);
typedef VFS_MUNMAP_FILE(vfs_munmap_file_t);

struct vfs_file_op_s
{
	vfs_open_file_t *open;
	vfs_read_file_t *read;
	vfs_write_file_t *write;
	vfs_lseek_file_t *lseek;
	vfs_read_dir_t *readdir;
	vfs_close_file_t *close;
	vfs_release_file_t *release;
	vfs_mmap_file_t *mmap;
	vfs_munmap_file_t *munmap;
};

/* Default/generic methods */
VFS_MMAP_FILE(vfs_default_mmap_file);
VFS_MMAP_FILE(vfs_default_munmap_file);
VFS_READ_FILE(vfs_default_read);
VFS_WRITE_FILE(vfs_default_write);


sint_t vfs_file_up(struct vfs_file_s *file);
sint_t vfs_file_down(struct vfs_file_s *file);

/** Kernel VFS Daemon */
extern void* kvfsd(void*);

/** Initialize VFS Sub-System */
error_t vfs_mount_fs_root(struct device_s *device,
			 uint_t fs_type,
			 struct task_s *task0);

/** Generic file related operations */
error_t vfs_create(struct vfs_file_s *cwd,
		   struct ku_obj *path,
		   uint_t flags,
		   uint_t mode,
		   struct vfs_file_s *file);

//TODO: change f_op interface to use f_remote!
//TODO: for file of type DEV_FS : re-open the file at remote location
error_t vfs_open(struct vfs_file_s *cwd,
		 struct ku_obj *path,
		 uint_t flags,
		 uint_t mode,
		 struct vfs_file_s *file);

ssize_t vfs_read(struct vfs_file_s *file, struct ku_obj *buff);//ok: need cleaning
ssize_t vfs_write (struct vfs_file_s *file, struct ku_obj *buff);//ok
error_t vfs_lseek(struct vfs_file_s *file, size_t offset, uint_t whence, size_t *new_offset_ptr);//ok
error_t vfs_close(struct vfs_file_s *file, uint_t *refcount);//ok
error_t vfs_unlink(struct vfs_file_s *cwd, struct ku_obj *path);//ok
//error_t vfs_stat(struct vfs_file_s *cwd, struct ku_obj *path, struct vfs_inode_s **inode);
error_t vfs_stat(struct vfs_file_s *cwd, struct ku_obj *path, struct vfs_stat_s* ustat, struct vfs_file_s *file);//ok

/** Generic directory related operations */
error_t vfs_opendir(struct vfs_file_s *cwd, struct ku_obj *path, uint_t mode, struct vfs_file_s *file);//ok
error_t vfs_readdir(struct vfs_file_s *file, struct ku_obj *dirent);//ok: fat32_readdir need cleaning
error_t vfs_mkdir(struct vfs_file_s *cwd, struct ku_obj *path, uint_t mode);//ok
error_t vfs_rmdir(struct vfs_file_s *cwd, struct ku_obj *path);//ok
error_t vfs_chdir(struct ku_obj *pathname, struct vfs_file_s *cwd);//ok
error_t vfs_chmod(struct ku_obj *pathname, struct vfs_file_s *cwd, uint_t mode);
error_t vfs_get_path(struct vfs_file_s *file, struct ku_obj  *buff);//ok
error_t vfs_closedir(struct vfs_file_s *file, uint_t *refcount);//ok

/** Generic FIFO operations */
error_t vfs_pipe(struct vfs_file_s *pipefd[2]);
error_t vfs_mkfifo(struct vfs_file_s *cwd, struct ku_obj *path, uint_t mode);

/** Caches functions **/
struct vfs_inode_s* vfs_inode_new(struct vfs_context_s* ctx);
cid_t vfs_inode_get_cid(uint_t ino);
error_t vfs_inode_hold(struct vfs_inode_s *inode, gc_t igc);
error_t vfs_inode_check(struct vfs_inode_s *inode, gc_t igc);
error_t vfs_inode_trunc(struct vfs_inode_s *inode);
error_t vfs_inode_lock(struct vfs_inode_s *inode);
error_t vfs_inode_unlock(struct vfs_inode_s *inode);
void vfs_dirent_set_inload(struct vfs_dirent_s *dirent);
void vfs_dirent_clear_inload(struct vfs_dirent_s *dirent);
bool_t vfs_dirent_is_inload(struct vfs_dirent_s *dirent);
struct vfs_inode_s* vfs_inode_get(uint_t inumber, struct vfs_context_s *ctx);
error_t vfs_icache_add(struct vfs_inode_s *inode);
error_t vfs_icache_del(struct vfs_inode_s *inode);
//Default inum allocation funcs
uint32_t vfs_inum_new(struct vfs_context_s *ctx);
void vfs_inum_free(struct vfs_context_s *ctx, uint_t inum);

//Helper functions
error_t vfs_delete_inode(uint_t inum, struct vfs_context_s *ctx, cid_t icid);

#endif	/* _VFS_H_ */
