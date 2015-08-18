/*
 * fat32/fat32_access.c - fat32 helper functions
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

#include <driver.h>
#include <device.h>
#include <string.h>
#include <rwlock.h>
#include <page.h>
#include <pmm.h>
#include <thread.h>
#include <cluster.h>
#include <mapper.h>
#include <vfs.h>

#include <fat32.h>
#include "fat32-private.h"

#if VFAT_INSTRUMENT
uint32_t blk_rd_count ;
uint32_t rd_count     ;
uint32_t wr_count     ;
#endif

RPC_DECLARE(__vfat_cluster_count, RPC_RET(RPC_RET_PTR(int, count)), 
			RPC_ARG(RPC_ARG_VAL(struct vfat_context_s*, ctx),
			RPC_ARG_VAL(vfat_cluster_t, cluster_index)))
{
	int cnt;
	vfat_cluster_t next_cluster_index = 0;

	cnt = 0;
	while(next_cluster_index < 0x0FFFFFF8) {
		vfat_query_fat(ctx, cluster_index, &next_cluster_index);
		cluster_index = next_cluster_index;
		cnt++;
	}
	*count = cnt;
}

int vfat_cluster_count(struct vfat_context_s *ctx, vfat_cluster_t cluster_index) 
{
	int count;
	RCPC(ctx->fat_cid, RPC_PRIO_FS, __vfat_cluster_count, 
					RPC_RECV(RPC_RECV_OBJ(count)), 
					RPC_SEND(RPC_SEND_OBJ(ctx),
					RPC_SEND_OBJ(cluster_index)));
	return count;
}

RPC_DECLARE(__vfat_query_fat, RPC_RET(RPC_RET_PTR(error_t, err), 
			RPC_RET_PTR(vfat_cluster_t, next_cluster_index)),

			RPC_ARG(RPC_ARG_VAL(struct vfat_context_s*, ctx),
			RPC_ARG_VAL(vfat_cluster_t, cluster_index)))
			
{
	vfat_cluster_t *data;
	vfat_cluster_t cluster_offset;
	struct page_s *page;
	struct mapper_s *mapper;
	uint32_t page_id;

	mapper = ctx->mapper;

	cluster_offset = ctx->fat_begin_lba * ctx->bytes_per_sector + (cluster_index * 4);
	page_id = cluster_offset >> PMM_PAGE_SHIFT;

	vfat_dmsg(1, "%s: TID %x, Query FAT at page_id %d to get next cluster of %u\n",
		  __FUNCTION__, current_thread, page_id, cluster_index);

	if ((page = mapper_get_page(mapper, page_id, MAPPER_SYNC_OP)) == NULL)
	{
		*err = -1;	
		return;
	}

	vfat_dmsg(1, "%s: cluster offset %d, offset %d\n", __FUNCTION__, cluster_offset, cluster_offset % PMM_PAGE_SIZE);

	data = (vfat_sector_t*) ppm_page2addr(page);
	data = (vfat_sector_t*)((uint8_t*) data + (cluster_offset % PMM_PAGE_SIZE));
	*next_cluster_index = *data & 0x0FFFFFFF;

	vfat_dmsg(1, "%s: next cluster for %u is %u\n", __FUNCTION__, cluster_index, *next_cluster_index);
	*err = 0;
}

error_t vfat_query_fat(struct vfat_context_s* ctx,
		vfat_cluster_t cluster_index,
		vfat_cluster_t *next_cluster_index)
{
	error_t err;
	vfat_cluster_t next;

	RCPC(ctx->fat_cid, RPC_PRIO_FS, __vfat_query_fat, 
				RPC_RECV(RPC_RECV_OBJ(err), RPC_RECV_OBJ(next)),
				RPC_SEND(RPC_SEND_OBJ(ctx), RPC_SEND_OBJ(cluster_index)));

	*next_cluster_index = next;
	return err;
}


RPC_DECLARE(__vfat_alloc_fat_entry, RPC_RET(RPC_RET_PTR(error_t, err), RPC_RET_PTR(vfat_cluster_t,new_cluster)), 
					RPC_ARG(RPC_ARG_VAL(struct vfat_context_s*, ctx)))
{
	struct page_s *page;
	struct mapper_s *mapper;
	vfat_sector_t EOF_sector;
	vfat_sector_t last_allocated_sector;
	vfat_sector_t sector;
	uint32_t last_allocated_index;
	uint32_t index;
	uint32_t sectors_per_page;
	uint32_t *buffer;
	uint32_t found;
	uint32_t sector_size;
	uint32_t page_id;
	uint32_t old_page_id;

	sector_size = ctx->bytes_per_sector;
	sectors_per_page = PMM_PAGE_SIZE / sector_size;
	EOF_sector = ctx->fat_begin_lba + ctx->fat_blk_count;
	*new_cluster = 0;
	*err = 0;
	found = 0;
	old_page_id = 0xFFFFFFFF;
	mapper = ctx->mapper;
	page = NULL;

	rwlock_wrlock(&ctx->lock);
	last_allocated_sector = ctx->last_allocated_sector;
	last_allocated_index = ctx->last_allocated_index;
	vfat_dmsg(2, "%s: Started, last allocated sector %u\n", __FUNCTION__, last_allocated_sector);

	for(sector = last_allocated_sector; (sector < EOF_sector) && (!found); sector ++)
	{
		vfat_dmsg(2, "%s: sector %u\n", __FUNCTION__, sector);
		page_id = sector / sectors_per_page;

		if (page_id != old_page_id) // Let's not reload the same page everytime
		{  
			vfat_dmsg(2, "%s: current page #%d, next page #%d\n", __FUNCTION__, old_page_id, page_id);

			page = mapper_get_page(mapper, page_id, MAPPER_SYNC_OP);

			if(page == NULL)
			{
				printk(WARNING, "WARNING: alloc_fat: error reading sector %d\n", sector);
				*err = VFS_IO_ERR;
				goto VFAT_ALLOC_CLUSTER_ERROR;
			}
		}

		buffer = ppm_page2addr(page);
		buffer = buffer + ((sector % sectors_per_page) * (sector_size / 4));

		for(index = last_allocated_index; index < (sector_size/4); index ++)
		{
			vfat_dmsg(2, "%s: index %d\n", __FUNCTION__, index);

			if((buffer[index] & 0x0FFFFFFF) == 0x00000000)
			{
				found = 1;

				page_lock(page);
				buffer[index] = 0x0FFFFFF8;
				mapper->m_ops->set_page_dirty(page);
				page_unlock(page);

#if VFAT_INSTRUMENT
				wr_count ++;
#endif
				vfat_dmsg(1,"%s: page #%d (@%x) is set to delayed write\n", __FUNCTION__, page->index, page);

				if(index != ((sector_size/4) - 1))
				{
					ctx->last_allocated_sector = sector;
					ctx->last_allocated_index = (index + 1) % (sector_size/4);
				} 
				else
				{
					last_allocated_sector = sector + 1;

					if(last_allocated_sector == EOF_sector) 
					{
						ctx->last_allocated_sector = ctx->fat_begin_lba;
						ctx->last_allocated_index = 2;
					} 
					else 
					{
						ctx->last_allocated_sector = last_allocated_sector;
						ctx->last_allocated_index = (index + 1) % (sector_size/4);
					}
				}

				index += (sector - ctx->fat_begin_lba) * (sector_size/4);
				vfat_dmsg(2, "%s: found: cluster %u\n",  __FUNCTION__, index);
				*new_cluster = index;
				break;
			}
		}

		last_allocated_index = 0;
		old_page_id = page_id;
	}

	//if(page)
	// page_refcount_down(page);

VFAT_ALLOC_CLUSTER_ERROR:
	rwlock_unlock(&ctx->lock);
	vfat_dmsg(2, "%s: new_cluster %u, hasError ? %d\n", __FUNCTION__, *new_cluster, *err);
}


error_t vfat_alloc_fat_entry(struct vfat_context_s* ctx, vfat_cluster_t *new_cluster)
{
	error_t err;
	vfat_cluster_t new;

	RCPC(ctx->fat_cid, RPC_PRIO_FS, __vfat_alloc_fat_entry, 
				RPC_RECV(RPC_RECV_OBJ(err), 
					RPC_RECV_OBJ(new)),
				RPC_SEND(RPC_SEND_OBJ(ctx)));

	*new_cluster=new;

	return err;
}


RPC_DECLARE(__vfat_extend_cluster, RPC_RET(RPC_RET_PTR(error_t, err), 
					RPC_RET_PTR(vfat_cluster_t, next_cluster)), 
					RPC_ARG(RPC_ARG_VAL(struct vfat_context_s*, ctx), 
					RPC_ARG_VAL(vfat_cluster_t, current_vfat_cluster)))
{
	struct page_s *page;
	struct mapper_s *mapper;
	vfat_sector_t lba;
	vfat_cluster_t val;
	size_t sector_size;
	uint32_t sectors_per_page;
	uint32_t *buffer;

	sector_size = ctx->bytes_per_sector;
	sectors_per_page = PMM_PAGE_SIZE / sector_size;
	mapper = ctx->mapper;

	if((*err = vfat_alloc_fat_entry(ctx, next_cluster)))
		return;

	if(*next_cluster == 0)
	{
		*err=0;
	}

	lba = ctx->fat_begin_lba + ((current_vfat_cluster *4) / sector_size);

	page = mapper_get_page(mapper, lba / sectors_per_page, MAPPER_SYNC_OP);

	if(page == NULL) goto VFAT_EXTEND_CLUSTER_ERROR;

	vfat_dmsg(1, "%s: extending cluster %u\n", __FUNCTION__, current_vfat_cluster);

	buffer = (uint32_t*) ppm_page2addr(page);
	buffer += (lba % sectors_per_page) * (sector_size/4);

	page_lock(page);

	val = buffer[current_vfat_cluster % (sector_size/4)] & 0x0FFFFFFF;

	if( val >= 0x0FFFFFF8)
		buffer[current_vfat_cluster % (sector_size/4)] = *next_cluster;
	else
	{
		vfat_dmsg(1,"%s: %u already extended !\n", __FUNCTION__, current_vfat_cluster);
		vfat_dmsg(1,"%s: freeing %u\n", __FUNCTION__, *next_cluster);

		page_unlock(page);
		//page_refcount_down(page);

		lba = ctx->fat_begin_lba + ((*next_cluster *4) / sector_size);

		if((page = mapper_get_page(mapper, lba / sectors_per_page, MAPPER_SYNC_OP)) == NULL)
			goto VFAT_EXTEND_CLUSTER_ERROR;

		buffer = (uint32_t*) ppm_page2addr(page);
		buffer += (lba % sectors_per_page) * (sector_size/4);
		page_lock(page);
		buffer[*next_cluster % (sector_size/4)] = 0x00;
	}

#if VFAT_INSTRUMENT
	wr_count ++;
#endif

	mapper->m_ops->set_page_dirty(page);

	page_unlock(page);
	vfat_dmsg(1,"%s: page #%d (@%x) is set to delayed write\n", __FUNCTION__, page->index, page);


	vfat_dmsg(1,"%s: cluster %u's FAT entry is set to %u, FAT's sector %u is set as delayed write\n",
		  __FUNCTION__, 
		  current_vfat_cluster, 
		  *next_cluster,
		  lba);

	vfat_dmsg(1,"%s: allocated cluster %u for asked cluster %u\n",
		  __FUNCTION__, 
		  *next_cluster, 
		  current_vfat_cluster);
	*err = 0;
	return;

VFAT_EXTEND_CLUSTER_ERROR:
	vfat_free_fat_entry(ctx,*next_cluster);
	*next_cluster = 0;
	*err = -1;
}

error_t vfat_extend_cluster(struct vfat_context_s* ctx,
		vfat_cluster_t current_vfat_cluster,
		vfat_cluster_t *next_cluster)
{
	error_t err;
	vfat_cluster_t next;

	RCPC(ctx->fat_cid, RPC_PRIO_FS, __vfat_extend_cluster, 
					RPC_RECV(RPC_RECV_OBJ(err), 
						RPC_RECV_OBJ(next)),
					RPC_SEND(RPC_SEND_OBJ(ctx), 
						RPC_SEND_OBJ(current_vfat_cluster)));

	*next_cluster=next;

	return err;
}


RPC_DECLARE(__vfat_free_fat_entry, RPC_RET(RPC_RET_PTR(error_t, err)), 
			RPC_ARG(RPC_ARG_VAL(struct vfat_context_s*, ctx),
			RPC_ARG_VAL(vfat_cluster_t, start_cluster)))
{
	struct page_s *page;
	struct mapper_s *mapper;
	vfat_sector_t *sector;
	vfat_cluster_t current_index;
	vfat_cluster_t next_index;
	vfat_sector_t lba;
	size_t sector_size;
	uint32_t sectors_per_page;

	sector_size = ctx->bytes_per_sector;
	sectors_per_page = PMM_PAGE_SIZE / sector_size;
	current_index = start_cluster;
	mapper = ctx->mapper;

	vfat_dmsg(1,"%s: freeling fat entries starting by %u\n",  __FUNCTION__, start_cluster);

	while((current_index > 0) && (current_index < 0x0FFFFFF7))
	{
		lba =  ctx->fat_begin_lba + ((current_index *4) / sector_size);

		vfat_dmsg(1,"%s: query FAT at %d lba to get next cluster index of %u\n",
			  __FUNCTION__, lba, current_index);

		page = mapper_get_page(mapper, lba / sectors_per_page, MAPPER_SYNC_OP);

		if(page == NULL)
		{
			*err= VFS_IO_ERR;
			return;
		}

		sector = (vfat_sector_t*) ppm_page2addr(page);
		sector += (lba%sectors_per_page) * (sector_size/4);

		page_lock(page);
		next_index = sector[current_index % (sector_size/4)] & 0x0FFFFFFF;
		sector[current_index % (sector_size/4)] = 0x00;
		mapper->m_ops->set_page_dirty(page);
		page_unlock(page);
		vfat_dmsg(1,"%s: page #%d (@%x) is set to delayed write\n", __FUNCTION__, page->index, page);
		//page_refcount_down(page);

#if VFAT_INSTRUMENT
		wr_count ++;
#endif

		vfat_dmsg(1,"%s: content of %d is set to 0, next cluster in the list is %d\n",
				__FUNCTION__, current_index, next_index);
		current_index = next_index;
	}
	*err = 0;
}


error_t vfat_free_fat_entry(struct vfat_context_s* ctx, vfat_cluster_t start_cluster)
{
	error_t err;
	RCPC(ctx->fat_cid, RPC_PRIO_FS, __vfat_free_fat_entry, 
					RPC_RECV(RPC_RECV_OBJ(err)), 
					RPC_SEND(RPC_SEND_OBJ(ctx),
					RPC_SEND_OBJ(start_cluster)));
	return err;
}

//RPC_DECLARE(__vfat_locate_entry, RPC_RET(RPC_RET_PTR(error_t, err), RPC_RET_PTR(uint_t, entry_index)), 
//	RPC_ARG(RPC_ARG_PTR(char, entry_name), RPC_ARG_PTR(struct vfat_entry_request_s, rq)))
error_t vfat_locate_entry(struct vfat_entry_request_s *rq)
{
	struct vfat_context_s *ctx;
	struct vfat_DirEntry_s *dir;
	struct page_s *page;
	struct mapper_s *mapper;
	struct vfs_inode_s *parent;
	struct vfat_inode_s *parent_info;
	uint_t entries_nr;
	uint_t entry;
	uint_t entry_id;
	uint_t current_page;
	size_t current_offset;
	char name[VFS_MAX_NAME_LENGTH + 1]; /* +1 for string null termination */
	uint32_t found;

	ctx = rq->ctx;
	dir = NULL;
	page = NULL;
	parent = rq->parent;
	mapper = parent->i_mapper;
	parent_info = parent->i_pv;
	entries_nr = PMM_PAGE_SIZE / sizeof(struct vfat_DirEntry_s);
	current_page = 0;
	current_offset = 0;
	found = 0;
	entry_id = 0;

	vfat_dmsg(1,"%s: started, node %s\n", __FUNCTION__, rq->entry_name);

	while(!found) 
	{
		page = mapper_get_page(mapper, current_page, MAPPER_SYNC_OP);

		if(page == NULL){ return VFS_IO_ERR;}

		page_lock(page);

		dir = (struct vfat_DirEntry_s*) ppm_page2addr(page);

		for(entry=0; entry < entries_nr; entry ++)
		{
			if(dir[entry].DIR_Name[0] == 0x00)
			{
				//page_refcount_down(page);
				page_unlock(page);
				return VFS_NOT_FOUND;
			}

			if(dir[entry].DIR_Name[0] == 0xE5)
				continue;

			if(dir[entry].DIR_Name[0] == '.')
				continue;

			if(rq->entry_name == NULL)
			{
				page_unlock(page);
				return VFS_ENOTEMPTY;
			}

			if(dir[entry].DIR_Attr == 0x0F)
				continue;

			vfat_getshortname((char *)dir[entry].DIR_Name, name);

			if(!strcmp(name,rq->entry_name))
			{
				found = 1;

				if (rq->entry != NULL)
					memcpy(rq->entry, &dir[entry], sizeof(*dir));

				vfat_dmsg(1, "%s: entries_nr %d, entry_id %d, entry %d\n",
						__FUNCTION__,
						entries_nr,
						entry_id,
						entry);

				*rq->entry_index = (entry_id * entries_nr) + entry;
				break;
			}

			current_offset += sizeof (struct vfat_DirEntry_s);
		}

		entry_id++;
		current_page++;
		page_unlock(page);
	}

	vfat_dmsg(1,"%s: found %d, entry_index %d\n", __FUNCTION__, found, *rq->entry_index);

	return (found) ? VFS_FOUND : VFS_NOT_FOUND;
}


RPC_DECLARE(__vfat_cluster_lookup, RPC_RET(RPC_RET_PTR(error_t, err), 
			RPC_RET_PTR(vfat_cluster_t, cluster_index),
			RPC_RET_PTR(vfat_cluster_t, extended)),

			RPC_ARG(RPC_ARG_VAL(struct vfat_context_s*, ctx),
			RPC_ARG_VAL(vfat_cluster_t, node_cluster),
			RPC_ARG_VAL(vfat_cluster_t, cluster_rank)))
			
{
	struct page_s* page;
	struct mapper_s* mapper;
	register vfat_cluster_t i, current_vfat_cluster;
	vfat_cluster_t next_cluster;
	vfat_cluster_t cluster_offset, old_cluster_offset;
	vfat_sector_t *sector;
	uint32_t sectors_per_page;
	uint32_t old_page_id, page_id;

	sectors_per_page = PMM_PAGE_SIZE / ctx->bytes_per_cluster;
	*extended = 0;
	current_vfat_cluster = node_cluster;
	next_cluster = 0;

	old_cluster_offset = ctx->fat_begin_lba*(ctx->bytes_per_sector/4) + current_vfat_cluster;
	old_page_id = old_cluster_offset / (PMM_PAGE_SIZE/4);

	mapper = ctx->mapper;
	page = mapper_get_page(mapper, old_page_id, MAPPER_SYNC_OP);

	if(page == NULL) {*err = VFS_IO_ERR; return;}

	sector = (vfat_sector_t*) ppm_page2addr(page);

	vfat_dmsg(1,"%s: started, first cluster %u, cluster rank %d\n",__FUNCTION__, node_cluster, cluster_rank);

	for(i=0; i < cluster_rank; i++) 
	{
		vfat_dmsg(5,"%s: loop %d/%d\n",__FUNCTION__,i+1,cluster_rank);
		next_cluster = sector[old_cluster_offset % (PMM_PAGE_SIZE/4)] & 0x0FFFFFFF;
		vfat_dmsg(5,"%s: next cluster found %u\n",__FUNCTION__, next_cluster);

		if(next_cluster == 0x0FFFFFF7)  // bad block
			{*err = VFS_EBADBLK; return;}

		if(next_cluster >= 0x0FFFFFF8)
		{ 
			if(vfat_extend_cluster(ctx, current_vfat_cluster, &next_cluster))
				// error while trying to extend
				{*err = VFS_IO_ERR; return;}

			if(next_cluster == 0)
				// no more space for another cluster
				{*err = VFS_ENOSPC; return;}

			*extended = 1;
		}

		current_vfat_cluster = next_cluster;
		cluster_offset = ctx->fat_begin_lba * (ctx->bytes_per_sector/4) + current_vfat_cluster;
		page_id = cluster_offset / (PMM_PAGE_SIZE/4);

		if(page_id != old_page_id)
		{
			//page_refcount_down(page);

			page = mapper_get_page(mapper, page_id, MAPPER_SYNC_OP);

			if(page == NULL) {*err = VFS_IO_ERR; return;}

			sector = (vfat_sector_t*) ppm_page2addr(page);

			vfat_dmsg(3, "%s: loading page %d for cluster %u\n", __FUNCTION__, page_id, next_cluster);
			old_page_id = page_id;
		}

		old_cluster_offset = cluster_offset;
	}

	vfat_dmsg(1, "%s: cluster found: %u\n", __FUNCTION__, current_vfat_cluster);
	//page_refcount_down(page);
	*cluster_index = current_vfat_cluster;
	*err = 0;
}

error_t vfat_cluster_lookup(struct vfat_context_s* ctx,
			    vfat_cluster_t node_cluster,
			    vfat_cluster_t cluster_rank,
			    vfat_cluster_t *cluster_index,
			    uint_t *extended)
{
	error_t err;
	vfat_cluster_t index;
	uint_t extd;

	RCPC(ctx->fat_cid, RPC_PRIO_FS, __vfat_cluster_lookup, 
					RPC_RECV(RPC_RECV_OBJ(err), 
						RPC_RECV_OBJ(index),
						RPC_RECV_OBJ(extd)), 

					RPC_SEND(RPC_SEND_OBJ(ctx),
						RPC_SEND_OBJ(node_cluster), 
						RPC_SEND_OBJ(cluster_rank)));
	*extended = extd;
	*cluster_index = index;
	return err;
}
