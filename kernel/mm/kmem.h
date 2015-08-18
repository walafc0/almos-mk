/*
 * mm/kmem.h - kernel unified memory allocator interface
 *
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 *
 * This file is part of ALMOS.
 *
 * ALMOS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Maintainers: Ghassan <ghassan.almaless@lip6.fr>
 */

#ifndef _KMEM_H_
#define _KMEM_H_

#include <types.h>
#include <kcm.h>

//FIXME: numbers
/* KMEM Managed Types */
typedef enum
{
  KMEM_PAGE = 0,	       /* RESERVED */
  KMEM_GENERIC,                /* RESERVED */
  KMEM_BLKIO,
  KMEM_RADIX_NODE,
  KMEM_DMA_REQUEST,
  KMEM_MAPPER,
  KMEM_TASK,
  KMEM_FDINFO,
  KMEM_DEVFS_CTX,
  KMEM_DEVFS_FILE,		/* 10 */
  KMEM_VFAT_CTX,
  KMEM_VFAT_NODE,
  KMEM_SYSFS_NODE,
  KMEM_SYSFS_FILE,
  KMEM_VFS_CTX,
  KMEM_VFS_INODE,
  KMEM_VFS_DIRENT,
  KMEM_VFS_FILE_REMOTE,
  KMEM_VM_REGION,		/* 20 */
  KMEM_SEM,			
  KMEM_CV,
  KMEM_BARRIER,
  KMEM_RWLOCK,
  KMEM_WQDB,			/* 25 */
  KMEM_KEYSREC,			
  //KMEM_EXT2_CTX,
  //KMEM_EXT2_NODE,
  //KMEM_EXT2_FILE,
  KMEM_TYPES_NR
}kmem_types_t;


/** Kernel object attributes */
struct kmem_objattr_s
{
  kmem_types_t type;
  char *name;
  uint_t size;
  uint_t min;
  uint_t max;
  uint_t aligne;
  kcm_init_destroy_t *ctor;
  kcm_init_destroy_t *dtor;
};

/**
 * Supplies object attributes. 
 * Each kernel object type has to supply its attributes.
 * 
 * @attr              Pointer to object's attributes
 * @retrun            Error code
 **/
#define KMEM_OBJATTR_INIT(n)  error_t (n)(struct kmem_objattr_s *attr)

/* Allocation Hints */
#define AF_HINT_MASK  0x0000FFFF
#define AF_TTL_MASK   0x000000FF
#define AF_TTL_LOW    0x00000008
#define AF_TTL_NORM   0x00000020
#define AF_TTL_AVRG   0x00000040
#define AF_TTL_HIGH   0x00000080

/* Allocation Flags */
#define AF_NONE       0x00000000
#define AF_SYS        0x00010000
#define AF_USR        0x00020000
#define AF_ZERO       0x00040000
#define AF_URGENT     0x00080000
#define AF_PRIO       0x00100000
#define AF_NORMAL     0x00200000
#define AF_REMOTE     0x00400000
#define AF_AFFINITY   0x00800000

/* Predefined Allocation Flags */
#define AF_BOOT      (AF_SYS | AF_URGENT)
#define AF_EXCEPT    (AF_SYS | AF_URGENT   | AF_TTL_LOW)
#define AF_KERNEL    (AF_SYS | AF_PRIO     | AF_TTL_NORM)
#define AF_PGFAULT   (AF_USR | AF_AFFINITY | AF_TTL_AVRG)
#define AF_USER      (AF_USR | AF_NORMAL   | AF_TTL_AVRG)


/* Hints GET/SET Macros */
#define AF_GETTTL(_FLAGS)           (((_FLAGS) & AF_TTL_MASK))

/* Kernel Memory Request */
typedef struct kmem_req_s
{
  /* Object Type */
  kmem_types_t type;
  
  /* Object Size */
  uint_t size;

  /* Allocation Falgs */
  uint_t flags;

  /* Object/Cluster Pointer */
  void *ptr;
} kmem_req_t;


/** Generic Kernel Allocator API **/

/**
 * Allocates new object
 *
 * @req              Pointer to allocation request
 *                   The ptr field of req can be used
 *                   to explicitly indicate the best
 *                   cluster to allocate from.
 *
 * @return           Pointer to new object, NULL otherwise
 **/
void* kmem_alloc(kmem_req_t *req);


/**
 * Frees previously allocated object
 *
 * @req              Pointer to free request
 *                   The ptr field of req must be set
 *                   to the same value gotten from 
 *                   kmem_alloc. The type field must.
 *
 * @return           Pointer to new object, NULL otherwise
 **/
void  kmem_free (kmem_req_t *req);


#endif	/* _KMEM_H_ */
