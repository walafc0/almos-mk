/*
   This file is part of MutekP.
  
   MutekP is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   MutekP is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with MutekP; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2008
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <kminiShell.h>
#include <system.h>
#include <ppm.h>
#include <kdmsg.h>
#include <page.h>
#include <thread.h>
#include <cluster.h>

error_t ppm_func(void *param)
{
#if 0
	uint_t i,j;
	register struct page_s *pages_tbl;
	struct ppm_s *ppm;
	uint_t cluster_nr;

	cluster_nr = arch_onln_cluster_nr();
#endif

	//for(i=0; i < cluster_nr; i++)
		ppm_print(&current_cluster->ppm);

//FIXME
#if 0
	for(j=0; j < cluster_nr; j++)
	{
		ppm = &clusters_tbl[j].cluster->ppm;
		pages_tbl = ppm->pages_tbl;
	
		for(i=0; i < ppm->pages_nr; i++)
		{
			if(pages_tbl[i].flags != 0x5)
	printk(WARNING,"WARNING: unexpected flags %x for page %x [i=%d]\n",
	       pages_tbl[i].flags,ppm_page2addr(&pages_tbl[i]),i);
		}
	
		for(i=0; i < KMEM_TYPES_NR; i++)
		{
			if(clusters_tbl[j].cluster->keys_tbl[i] != NULL)
	kcm_print(clusters_tbl[j].cluster->keys_tbl[i]);
		}
	}
#endif
	return 0;
}
