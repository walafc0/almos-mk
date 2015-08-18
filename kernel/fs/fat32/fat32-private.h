/*
 * fat32/fat32-private.h - fat32 partition descriptors & helper functions
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

#ifndef __VFAT_PRIVATE_H__
#define __VFAT_PRIVATE_H__

#include <stdint.h>
#include <rwlock.h>

#define VFAT_DEBUG      CONFIG_VFAT_DEBUG
#define VFAT_INSTRUMENT CONFIG_VFAT_INSTRUMENT

struct vfat_bpb_s
{
	uint8_t         BS_jmpBoot[3];
	uint8_t         BS_OEMName[8];
	uint16_t	BPB_BytsPerSec;
	uint8_t	        BPB_SecPerClus;
	uint16_t	BPB_RsvdSecCnt;
	uint8_t	        BPB_NumFATs;
	uint16_t	BPB_RootEntCnt;
	uint16_t	BPB_TotSec16;
	uint8_t	        BPB_Media;
	uint16_t	BPB_FATSz16;
	uint16_t	BPB_SecPerTrk;
	uint16_t	BPB_NumHeads;
	uint32_t	BPB_HiddSec;
	uint32_t	BPB_TotSec32;
	uint32_t	BPB_FATSz32;
	uint16_t	BPB_ExtFlags;
	uint16_t	BPB_VFVer;
	uint32_t	BPB_RootClus;
	uint16_t	BPB_FSInfo;
	uint16_t	BPB_BkBootSec;
	uint8_t	        BPB_Reserved[12];
	uint8_t	        BS_DrvNum;
	uint8_t	        BS_Reserved1;
	uint8_t	        BS_BootSig;
	uint32_t	BS_VolID;
	uint8_t	        BS_VolLab[11];
	uint8_t	        BS_FilSysType[8];
} __attribute__ ((packed));


struct vfat_DirEntry_s
{
	uint8_t	        DIR_Name[11];
	uint8_t	        DIR_Attr;
	uint8_t	        DIR_NTRes;
	uint8_t	        DIR_CrtTimeTenth;
	uint16_t	DIR_CrtTime;
	uint16_t	DIR_CrtDate;
	uint16_t	DIR_LstAccTime;
	uint16_t	DIR_FstClusHI;
	uint16_t	DIR_WrtTime;
	uint16_t	DIR_WrtDate;
	uint16_t	DIR_FstClusLO;
	uint32_t	DIR_FileSize;
} __attribute__ ((packed));


#define VFAT_ATTR_READ_ONLY	0x01
#define VFAT_ATTR_HIDDEN	0x02
#define VFAT_ATTR_SYSTEM	0x04
#define VFAT_ATTR_VOLUME_ID	0x08
#define VFAT_ATTR_DIRECTORY	0x10
#define VFAT_ATTR_ARCHIVE	0x20
#define VFAT_ATTR_LONG_NAME	0x0F


struct device_s;
struct rwlock_s;


struct vfat_inode_s
{
	uint32_t flags;
	vfat_cluster_t first_cluster;
};

struct vfat_entry_request_s
{
	struct vfat_DirEntry_s *entry;
	struct vfat_context_s *ctx;
	struct vfs_inode_s *parent;
	char *entry_name;
	uint_t *entry_index;
};

extern const struct vfs_inode_op_s vfat_i_op;
extern const struct vfs_dirent_op_s vfat_d_op;
extern const struct vfs_file_op_s vfat_f_op;
extern const struct mapper_op_s vfat_file_mapper_op;
extern const struct mapper_op_s vfat_inode_mapper_op;


/**
 * Gives the LBA of the first sector of a vfat cluster.
 *
 * @ctx		vfat_context
 * @cluster	cluster to be converted
 */
#define VFAT_CONVERT_CLUSTER(ctx,cluster) (ctx)->cluster_begin_lba +	\
					       ((cluster) - 2) * (ctx)->sectors_per_cluster

int vfat_cluster_count(struct vfat_context_s *ctx, vfat_cluster_t cluster_index);

error_t vfat_query_fat(struct vfat_context_s* ctx,
		       vfat_cluster_t cluster_index,
		       vfat_cluster_t *next_cluster_index);

error_t vfat_alloc_fat_entry(struct vfat_context_s* ctx,
			     vfat_cluster_t *new_cluster);

error_t vfat_extend_cluster(struct vfat_context_s* ctx,
			    vfat_cluster_t current_vfat_cluster,
			    vfat_cluster_t *next_cluster);

error_t vfat_free_fat_entry(struct vfat_context_s* ctx,
			    vfat_cluster_t start_cluster);

error_t vfat_locate_entry(struct vfat_entry_request_s *rq);

inline void vfat_getshortname(char *from, char *to);

error_t vfat_cluster_lookup(struct vfat_context_s* ctx,
			    vfat_cluster_t node_cluster,
			    vfat_cluster_t cluster_rank,
			    vfat_cluster_t *cluster_index,
			    uint_t *extended);


#if VFAT_INSTRUMENT
extern uint32_t blk_rd_count;
extern uint32_t rd_count;
extern uint32_t wr_count;
#endif


#endif
