/*
 * arch-dqdt.c - build the Distributed Quaternary Decision Tree
 * (see kern/dqdt.h)
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

#include <errno.h>
#include <types.h>
#include <cpu.h>
#include <bits.h>
#include <thread.h>
#include <system.h>
#include <kmem.h>
#include <arch-bib.h>
#include <boot-info.h>
#include <cluster.h>
#include <kdmsg.h>
#include <dqdt.h>

#if 0
uint_t arch_dqdt_distance(struct dqdt_cluster_s *c1, struct dqdt_cluster_s *c2, struct dqdt_attr_s *attr)
{
	register sint_t x1,y1,x2,y2,d;
  
	switch(attr->d_attr)
	{
	case DQDT_DIST_DEFAULT:
		x1 = c1->home->x_coord;
		y1 = c1->home->y_coord;
		x2 = c2->home->x_coord;
		y2 = c2->home->y_coord;
		d = ABS((x1 - x2)) + ABS((y1 - y2));
		break;

	case DQDT_DIST_RANDOM:
		//srand(cpu_time_stamp());
		d = rand() % 0xFFF;
		break;

	default:
		d = 1;
		break;
	}

	return d;
}

static uint_t __cluster_index(uint_t x, uint_t y, uint_t ymax)
{
	return x*ymax + y;
}

error_t arch_dqdt_chip_init(struct dqdt_cluster_s ***chip,
			    struct cluster_entry_s *clusters_tbl, 
			    uint_t xmax, 
			    uint_t ymax)
{
	struct dqdt_cluster_s *ptr;
	kmem_req_t req;
	uint_t chip_xmax;
	uint_t chip_ymax;
	uint_t x;
	uint_t y;
	uint_t cid;

	dqdt_dmsg(1, "%s: real (%d x %d)\n", __FUNCTION__, xmax, ymax);

	req.type  = KMEM_GENERIC;
	req.size  = sizeof(*ptr);
	chip_xmax = CONFIG_MAX_CLUSTER_ROOT;
	chip_ymax = chip_xmax;

	for(x = 0; x < chip_xmax; x++)
		for(y = 0; y < chip_ymax; y++)
			chip[x][y] = NULL;

	req.flags = AF_BOOT | AF_REMOTE | AF_ZERO;

	for(x = 0; x < xmax; x++)
	{
		for(y = 0; y < ymax; y++)
		{
			cid = __cluster_index(x, y, ymax);
      
			if(clusters_tbl[cid].cluster == NULL)
				continue;

			req.ptr = clusters_tbl[cid].cluster;
			ptr     = kmem_alloc(&req);

			if(ptr == NULL) 
				return ENOMEM;

			ptr->cores_nr            = clusters_tbl[cid].cluster->onln_cpu_nr;
			ptr->flags               = DQDT_CLUSTER_UP;
			ptr->home                = clusters_tbl[cid].cluster;
			ptr->home->levels_tbl[0] = ptr;
			ptr->home->x_coord       = x;
			ptr->home->y_coord       = y;
			ptr->home->z_coord       = 0;
			ptr->home->chip_id       = 0;
			chip[x][y]               = ptr;

			dqdt_dmsg(1, "%s: found cid %d, x %d, y %d, ptr %x\n", 
				  __FUNCTION__, 
				  cid, 
				  x, 
				  y, 
				  ptr);
		}
	}

	req.flags = AF_BOOT | AF_ZERO;

	for(x = 0; x < chip_xmax; x++)
	{
		for(y = 0; y < chip_ymax; y++)
		{
			if(chip[x][y] != NULL)
				continue;
      
			ptr = kmem_alloc(&req);
      
			if(ptr == NULL) 
				return ENOMEM;

			chip[x][y] = ptr;
		}
	}

	return 0;
}

static struct dqdt_cluster_s *** arch_dqdt_alloc_matrix(uint_t xmax, uint_t ymax)
{
	uint_t x;
	kmem_req_t req;
	struct dqdt_cluster_s ***matrix;

	dqdt_dmsg(1, "%s: %d x %d\n", __FUNCTION__, xmax, ymax);
  
	req.type  = KMEM_GENERIC;
	req.size  = xmax * sizeof(*matrix);
	req.flags = AF_BOOT;
  
	matrix = kmem_alloc(&req);

	if(matrix == NULL)
		return NULL;
  
	for(x = 0; x < xmax; x++)
	{
		req.size  = ymax * sizeof(**matrix);
		matrix[x] = kmem_alloc(&req);

		if(matrix[x] == NULL)
			return NULL;
	}
  
	dqdt_dmsg(1, "%s: %d x %d, done\n", __FUNCTION__, xmax, ymax);
	return matrix;
}

void arch_dqdt_free_matrix(struct dqdt_cluster_s ***matrix, uint_t xmax, uint_t ymax)
{
	uint_t x;
	kmem_req_t req;
  
	req.type = KMEM_GENERIC;
    
	for(x = 0; x < xmax; x++)
	{
		req.ptr = matrix[x];
		kmem_free(&req);
	}
  
	req.ptr = matrix;
	kmem_free(&req);
}

struct dqdt_cluster_s * arch_dqdt_make_cluster(struct dqdt_cluster_s ***matrix,
					       uint_t level,
					       uint_t x, 
					       uint_t y)
{
	kmem_req_t req;
	struct cluster_s *cluster;
	struct dqdt_cluster_s *ptr;
	struct dqdt_cluster_s *child0;
	struct dqdt_cluster_s *child1;
	struct dqdt_cluster_s *child2;
	struct dqdt_cluster_s *child3;
	uint_t clstr_x;
	uint_t clstr_y;
	uint_t childs_nr;
	uint_t cores_nr;

	childs_nr = 0;
	cores_nr  = 0;  
	child0    = NULL;
	child1    = NULL;
	child2    = NULL;
	child3    = NULL;
	cluster   = NULL;
	clstr_x   = x;
	clstr_y   = y;

	dqdt_dmsg(1, "%s: level %d, x %d, y %d\n", __FUNCTION__, level, x, y);

	if(matrix[x][y]->flags & DQDT_CLUSTER_UP)
	{
		child0  = matrix[x][y];
		cluster = matrix[x][y]->home;
		clstr_x = x;
		clstr_y = y;
		childs_nr ++;
		cores_nr += child0->cores_nr;
	}

	if(matrix[x+1][y]->flags & DQDT_CLUSTER_UP) 
	{
		child1  = matrix[x+1][y];
		cluster = matrix[x+1][y]->home;
		clstr_x = x+1;
		clstr_y = y;
		childs_nr ++;
		cores_nr += child1->cores_nr;
	}
   
	if(matrix[x][y+1]->flags & DQDT_CLUSTER_UP) 
	{
		child2  = matrix[x][y+1];
		cluster = matrix[x][y+1]->home;
		clstr_x = x;
		clstr_y = y+1;
		childs_nr ++;
		cores_nr += child2->cores_nr;
	}
  
	if(matrix[x+1][y+1]->flags & DQDT_CLUSTER_UP)
	{
		child3  = matrix[x+1][y+1];
		cluster = matrix[x+1][y+1]->home;
		clstr_x = x+1;
		clstr_y = y+1;
		childs_nr ++;
		cores_nr += child3->cores_nr;
	}
  
	req.type  = KMEM_GENERIC;
	req.size  = sizeof(*ptr);
	req.flags = AF_BOOT | AF_REMOTE | AF_ZERO;
	req.ptr   = (cluster == NULL) ? current_cluster : cluster;

	ptr = kmem_alloc(&req);
  
	if(ptr == NULL) 
		return NULL;

	ptr->level = level;
	ptr->home  = matrix[clstr_x][clstr_y]->home;
	ptr->flags = matrix[clstr_x][clstr_y]->flags;

	if(cluster != NULL)
	{
		cluster->levels_tbl[level] = ptr;

		ptr->children[0] = child0;
		ptr->children[1] = child1;
		ptr->children[2] = child2;
		ptr->children[3] = child3;

		matrix[x][y]->index     = 0;
		matrix[x+1][y]->index   = 1;
		matrix[x][y+1]->index   = 2;
		matrix[x+1][y+1]->index = 3;
	}

	if(child0 == NULL)
	{
		req.ptr = matrix[x][y];
		kmem_free(&req);
	}
	else
		child0->parent = ptr;
  
	if(child1 == NULL)
	{
		req.ptr = matrix[x+1][y];
		kmem_free(&req);
	}
	else
		child1->parent = ptr;

	if(child2 == NULL)
	{
		req.ptr = matrix[x][y+1];
		kmem_free(&req);
	}
	else
		child2->parent = ptr;

	if(child3 == NULL)
	{
		req.ptr = matrix[x+1][y+1];
		kmem_free(&req);
	}
	else
		child3->parent = ptr;

	ptr->cores_nr  = cores_nr;
	ptr->childs_nr = childs_nr;
	
	dqdt_dmsg(1, "%s: level %d, x %d, y %d, childs_nr %d, cores_nr %d, isUp %s, done\n", 
		  __FUNCTION__, 
		  level, 
		  x, 
		  y,
		  cores_nr,
		  childs_nr,
		  (cluster == NULL) ? "No" : "Yes");

	return ptr;
}

error_t arch_dqdt_build(struct boot_info_s *info)
{
	struct arch_bib_header_s *header;
	struct dqdt_cluster_s ***current;
	struct dqdt_cluster_s ***next;
	uint_t xmax;
	uint_t ymax;
	uint_t level;
	uint_t x;
	uint_t y;
	error_t err;

	header = (struct arch_bib_header_s*) info->arch_info;
	xmax   = CONFIG_MAX_CLUSTER_ROOT;
	ymax   = xmax;
	level  = 1;
  
	current = arch_dqdt_alloc_matrix(xmax, ymax);

	if(current == NULL)
		return -1;
  
	err = arch_dqdt_chip_init(current, 
				  &clusters_tbl[0], 
				  header->x_max, 
				  header->y_max);
  
	if(err) return err;
  
	while(xmax > 2)
	{
		dqdt_dmsg(1, "%s: current level %d, current xmax %d\n", 
			  __FUNCTION__, 
			  level, 
			  xmax);

		next = arch_dqdt_alloc_matrix(xmax/2, ymax/2);
    
		if(next == NULL)
			return -xmax;
    
		for(x = 0; x < xmax; x += 2)
		{
			for(y = 0; y < ymax; y += 2)
			{
				next[x/2][y/2] = arch_dqdt_make_cluster(current, level, x, y);

				if(next[x/2][y/2] == NULL)
					return -xmax;
			}
		}

		level ++;
		arch_dqdt_free_matrix(current, xmax, ymax);
		current = next;
		xmax = xmax/2;
		ymax = ymax/2;
	}

	dqdt_dmsg(1, "%s: making dqdt_root, level %d\n", __FUNCTION__, level);

	dqdt_root = arch_dqdt_make_cluster(current, level, 0, 0);
	arch_dqdt_free_matrix(current, xmax, ymax);
 
	return 0;
}

#endif
