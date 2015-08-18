/*
 * sysfs/sysfs.h - sysfs interface 
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

#ifndef _SYSFS_H_
#define _SYSFS_H_

#include <list.h>
#include <types.h>
#include <metafs.h>
#include <kmem.h>

//////////////////////////////////////////
//           Public Section             //
//////////////////////////////////////////

/** Sysfs maximum supported name length */
#define SYSFS_NAME_LEN      12

/** Sysfs maximum supported output size */
#define SYSFS_BUFFER_SIZE   256

/** VFS context-related operations */
extern const struct vfs_context_op_s sysfs_ctx_op;

struct sysfs_context_s
{
	void *empty;
};


/** Sysfs entry & operations structres declaration */
struct sysfs_op_s;
struct sysfs_entry_s;
typedef struct sysfs_op_s sysfs_op_t;
typedef struct sysfs_entry_s sysfs_entry_t;

/** 
 * Request defintion, where buffer is entirly 
 * read/write by underlying kernel subsystem 
 **/
typedef struct
{
	void *data;
	uint_t count;
	uint8_t buffer[SYSFS_BUFFER_SIZE];
}sysfs_request_t;

typedef error_t (sysfs_request_func_t)(struct sysfs_entry_s *entry, sysfs_request_t *rq, uint_t *offset);

/** 
 * Operations definition must be provided 
 * by underlying kernel subsystem 
 **/
struct sysfs_op_s
{
	sysfs_request_func_t *open;
	sysfs_request_func_t *read;
	sysfs_request_func_t *write;
	sysfs_request_func_t *close;
};

/** Sysfs root entry */
extern sysfs_entry_t sysfs_root_entry;

/** Initialize Sysfs */
static inline void sysfs_root_init();

/** Initialize given Sysfs entry */
static inline void sysfs_entry_init(sysfs_entry_t *entry, sysfs_op_t *op, char *name);

/** Register given Sysfs entry into it's parent list */
static inline void sysfs_entry_register(sysfs_entry_t *parent, sysfs_entry_t *entry);

/** Unregister given Sysfs entry from it's parent list */
static inline void sysfs_entry_unregister(sysfs_entry_t *parent, sysfs_entry_t *entry);

/** Get a pointer to Sysfs entry container data structure */
#define sysfs_container(_entry,_type,_name)

KMEM_OBJATTR_INIT(sysfs_kmem_node_init);
KMEM_OBJATTR_INIT(sysfs_kmem_file_init);

//////////////////////////////////////////
//          Private Section             //
//////////////////////////////////////////

struct sysfs_entry_s
{
	struct sysfs_op_s op;
	struct metafs_s node;
};

static inline void sysfs_entry_init(sysfs_entry_t *entry, sysfs_op_t *op, char *name)
{
	metafs_init(&entry->node, name);

	if(op == NULL) return;
  
	entry->op.open  = op->open;
	entry->op.read  = op->read;
	entry->op.write = op->write;
	entry->op.close = op->close;
}

static inline void sysfs_entry_register(sysfs_entry_t *parent, sysfs_entry_t *entry)
{
	metafs_register(&parent->node, &entry->node);
}

static inline void sysfs_entry_unregister(sysfs_entry_t *parent, sysfs_entry_t *entry)
{
	metafs_unregister(&parent->node, &entry->node);
}

static inline void sysfs_root_init()
{
	metafs_init(&sysfs_root_entry.node, 
#if CONFIG_ROOTFS_IS_VFAT
		    "SYS"
#else
		    "sys"
#endif
		);
}

#undef  sysfs_container
#define sysfs_container(_entry,_type,_name)	\
	list_container(_entry, _type, _name)

#endif	/* _SYSFS_H_ */
