/*
 * fat32/fat32_mapper.c - fat32 mapper related operations
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

#include <stdint.h>
#include <page.h>
#include <ppm.h>
#include <pmm.h>
#include <vfs.h>
#include <thread.h>
#include <driver.h>
#include <blkio.h>
#include <mapper.h>
#include <list.h>

#include <fat32.h>
#include <fat32-private.h>

//TODO: remove 
struct vfat_file_s
{
	struct vfat_context_s *ctx;
	vfat_cluster_t  first_cluster;
	vfat_cluster_t  pg_current_cluster;
	vfat_cluster_t  pg_current_rank;
};

static inline error_t vfat_pgio_fat(struct vfat_context_s* ctx, struct page_s *page, uint_t flags) 
{
	struct blkio_s *blkio;
	struct device_s* dev;
	uint_t sectors_per_page;
	vfat_cluster_t lba_start;
	uint_t vaddr;
	error_t err;

	dev              = ctx->dev;
	sectors_per_page = PMM_PAGE_SIZE / ctx->bytes_per_sector;
	lba_start        = page->index * sectors_per_page;
  
	if((err = blkio_init(dev, page, 1)) != 0)
		return err;

	vaddr = (uint_t) ppm_page2addr(page);

	blkio                 = list_first(&page->root, struct blkio_s, b_list);
	blkio->b_dev_rq.src   = (void*)lba_start;
	blkio->b_dev_rq.dst   = (void*)vaddr;
	blkio->b_dev_rq.count = sectors_per_page;

	err = blkio_sync(page,flags);
	blkio_destroy(page);
	return err;
}

//TODO: remove file info ?
static inline error_t vfat_pgio_reg(struct vfat_file_s *file_info, struct page_s *page, uint_t flags)
{
	struct vfat_context_s* ctx;
	struct slist_entry *iter;
	struct blkio_s *blkio;
	vfat_cluster_t cluster_rank;
	vfat_cluster_t effective_rank;
	vfat_cluster_t current_vfat_cluster;
	uint_t sector_start;
	uint_t sector_count;
	uint_t blkio_nr;
	uint_t clusters_per_page;
	uint_t vaddr;
	error_t err;

	ctx = file_info->ctx;

	clusters_per_page = (ctx->bytes_per_cluster <= PMM_PAGE_SIZE) ? 
		PMM_PAGE_SIZE / ctx->bytes_per_cluster : 1;

	vfat_cluster_t cluster_index[clusters_per_page];
	uint_t extended[clusters_per_page];

	current_vfat_cluster = file_info->pg_current_cluster;
	cluster_rank     = (page->index << PMM_PAGE_SHIFT) / ctx->bytes_per_cluster;
	effective_rank   = cluster_rank;
	cluster_index[0] = current_vfat_cluster;
	extended[0]      = 0;

	vfat_dmsg(1, "%s: %d clusters per page, %d is the first clstr, current clstr %d, rank %d\n",
		  __FUNCTION__, 
		  clusters_per_page, 
		  file_info->first_cluster, 
		  current_vfat_cluster, 
		  cluster_rank);

	if(cluster_rank != file_info->pg_current_rank)
	{
		if(cluster_rank > file_info->pg_current_rank)
			effective_rank -= file_info->pg_current_rank;
		else
			current_vfat_cluster = file_info->first_cluster;

		vfat_dmsg(1, "%s: pgindex %d, cluster_rank %d, effective_rank %d, current_cluster %d\n",
			  __FUNCTION__, 
			  page->index, 
			  cluster_rank, 
			  effective_rank, 
			  current_vfat_cluster);
  
		if(effective_rank != 0)
		{
			err = vfat_cluster_lookup(ctx, 
						  current_vfat_cluster, 
						  effective_rank, 
						  &cluster_index[0], 
						  &extended[0]);

			if(err) return err;
		}

		current_vfat_cluster = cluster_index[0];
	}

	current_vfat_cluster = cluster_index[0];
	vfat_dmsg(1, "%s: %d is the first cluster to be read\n", __FUNCTION__, current_vfat_cluster);
  
	for (blkio_nr = 1; blkio_nr < clusters_per_page; blkio_nr++) 
	{
		if((err = vfat_cluster_lookup(ctx,
					      current_vfat_cluster,
					      1,
					      &cluster_index[blkio_nr],
					      &extended[blkio_nr])) != 0) 
		{
			if (err == VFS_ENOSPC && (flags & BLKIO_RD))
			{
				extended[blkio_nr] = 1; /* to adjust the i counter */
				break;
			}
			return err;
		}
		current_vfat_cluster = cluster_index[blkio_nr];
	}

	uint_t i = clusters_per_page;

	if (flags & BLKIO_RD)
	{
		for (i = 0; i < clusters_per_page; i++)
			if (extended[i])
				break;
	}
  
	current_vfat_cluster = cluster_index[0];
	sector_start = VFAT_CONVERT_CLUSTER(ctx,current_vfat_cluster);
  
	if (i == 0) // Nothing to read because it's a new cluster.
		return 0;

	if(ctx->bytes_per_cluster <= PMM_PAGE_SIZE) 
	{
		// mutliple clusters per page
		sector_count = ctx->sectors_per_cluster;
	}
	else 
	{
		// clusters in multiple pages
		sector_count = PMM_PAGE_SIZE / ctx->bytes_per_sector;
		blkio_nr     = page->index % (ctx->bytes_per_cluster >> PMM_PAGE_SHIFT);
		sector_start = sector_start + blkio_nr * sector_count;
	}
  
	// initializing the blkio structures
	if((err = blkio_init(ctx->dev, page, i)))
		return err;

	vaddr = (uint_t) ppm_page2addr(page);
	i = 0;

	vfat_dmsg(1, "%s, new page %p\n", __FUNCTION__, vaddr);
  
	// we initialize the dev requests for each sector
	list_foreach(&page->root, iter) 
	{
		blkio = list_element(iter, struct blkio_s, b_list);
		blkio->b_dev_rq.src   = (void*)sector_start;
		blkio->b_dev_rq.dst   = (void*)vaddr;
		blkio->b_dev_rq.count = sector_count;
		i++;

		// lookup the next cluster
		if((clusters_per_page > 1) && (i < clusters_per_page))  
		{
			current_vfat_cluster = cluster_index[i];
			sector_start         = VFAT_CONVERT_CLUSTER(ctx,current_vfat_cluster);
			vaddr               += ctx->bytes_per_cluster;
			cluster_rank ++;
		}
	}

	err = blkio_sync(page,flags);
	blkio_destroy(page);
	file_info->pg_current_cluster = current_vfat_cluster;
	file_info->pg_current_rank    = cluster_rank;

	return err;
}

MAPPER_READ_PAGE(vfat_node_read_page)
{
	struct mapper_s *mapper;
	struct vfs_inode_s *inode;
	struct vfat_inode_s *node_info;
	struct vfat_file_s file_info;
	uint_t op_flags;

	op_flags = (flags & MAPPER_SYNC_OP) ? BLKIO_SYNC | BLKIO_RD : BLKIO_RD;

	mapper = page->mapper;

	if(mapper->m_inode == NULL)
		return vfat_pgio_fat(mapper->m_data, page, op_flags);

	inode                        = mapper->m_inode;
	node_info                    = inode->i_pv;
	file_info.ctx                = &inode->i_ctx->ctx_vfat;
	file_info.first_cluster       = node_info->first_cluster;
	file_info.pg_current_cluster = node_info->first_cluster;
	file_info.pg_current_rank    = 0;

	return vfat_pgio_reg(&file_info, page, op_flags);
}

MAPPER_WRITE_PAGE(vfat_node_write_page) 
{
	struct mapper_s *mapper;
	struct vfs_inode_s *inode;
	struct vfat_inode_s *node_info;
	struct vfat_file_s file_info;
	uint_t op_flags;

	op_flags = (flags & MAPPER_SYNC_OP) ? BLKIO_SYNC : 0;

	mapper = page->mapper;

	if(mapper->m_inode == NULL)
		return vfat_pgio_fat(mapper->m_data, page, op_flags);

	inode                        = page->mapper->m_inode;
	node_info                    = inode->i_pv;
	file_info.ctx                = &inode->i_ctx->ctx_vfat;
	file_info.first_cluster       = node_info->first_cluster;
	file_info.pg_current_cluster = node_info->first_cluster;
	file_info.pg_current_rank    = 0;

	return vfat_pgio_reg(&file_info, page, op_flags);
}

MAPPER_READ_PAGE(vfat_file_read_page) 
{
	uint_t op_flags;
	struct mapper_s *mapper;
	struct vfat_file_s file_info;
	struct vfs_inode_s *inode;
	struct vfat_inode_s *node_info;

	op_flags = (flags & MAPPER_SYNC_OP) ? BLKIO_SYNC | BLKIO_RD : BLKIO_RD;

	mapper = page->mapper;
	inode                        = mapper->m_inode;
	node_info                    = inode->i_pv;
	file_info.ctx                = &inode->i_ctx->ctx_vfat;
	file_info.first_cluster       = node_info->first_cluster;
	file_info.pg_current_cluster = node_info->first_cluster;
	file_info.pg_current_rank    = 0;

	return vfat_pgio_reg(&file_info, page, op_flags);
}

const struct mapper_op_s vfat_file_mapper_op =
{
	.writepage        = vfat_node_write_page,
	.readpage         = vfat_file_read_page,
	.sync_page        = mapper_default_sync_page,
	.set_page_dirty   = mapper_default_set_page_dirty,
	.clear_page_dirty = mapper_default_clear_page_dirty,
	.releasepage      = mapper_default_release_page,
};

const struct mapper_op_s vfat_inode_mapper_op =
{
	.writepage        = vfat_node_write_page,
	.readpage         = vfat_node_read_page,
	.sync_page        = mapper_default_sync_page,
	.set_page_dirty   = mapper_default_set_page_dirty,
	.clear_page_dirty = mapper_default_clear_page_dirty,
	.releasepage      = mapper_default_release_page,
};
