/*
 * kern/sys_mcntl.c - lets the process control some aspect of its memory managment
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

#include <types.h>
#include <errno.h>
#include <syscall.h>
#include <cluster.h>
#include <cpu.h>
#include <vmm.h>
#include <pmm.h>
#include <task.h>
#include <thread.h>
#include <page.h>

/* TODO: implement the MCNTL_MOVE operation */
int sys_mcntl(int op, uint_t vaddr, size_t len, minfo_t *pinfo)
{
	minfo_t uinfo;
	pmm_page_info_t info;
	struct ppm_s *ppm;
	struct cluster_s *cluster;
	error_t err;

#if 0
	err = cpu_copy_from_uspace(&uinfo, pinfo, sizeof(*pinfo));

	if(err) goto SYS_MCNTL_ERR;
#endif

	if(op == MCNTL_L1_iFLUSH)
	{
		pmm_cache_flush(PMM_TEXT);
		return 0;
	}

	if((vaddr == 0) || (op >= MCNTL_OPS_NR) || (pinfo == NULL))
	{
		err = EINVAL;
		goto SYS_MCNTL_ERR;
	}

	vaddr = ARROUND_DOWN(vaddr, PMM_PAGE_SIZE);

	if(NOT_IN_USPACE((uint_t)vaddr + (len * PMM_PAGE_SIZE)) || 
	   NOT_IN_USPACE((uint_t)pinfo + sizeof(*pinfo)))
	{
		err = EACCES;
		goto SYS_MCNTL_ERR;
	}

	err = pmm_get_page(&current_task->vmm.pmm, vaddr, &info);

	if(err) goto SYS_MCNTL_ERR;

	ppm = pmm_ppn2ppm(info.ppn);

	if(ppm->signature != PPM_ID)
	{
		err = EIO;
		goto SYS_MCNTL_ERR;
	}

	cluster = ppm_get_cluster(ppm);

	uinfo.mi_cid = cluster->id;
	uinfo.mi_cx  = cluster->x_coord;
	uinfo.mi_cy  = cluster->y_coord;
	uinfo.mi_cz  = cluster->z_coord;

	printk(INFO, "%s: cid %d, cx %d, cy %d, cz %d\n", 
	       __FUNCTION__,
	       uinfo.mi_cid,
	       uinfo.mi_cx,
	       uinfo.mi_cy,
	       uinfo.mi_cz);

	err = cpu_copy_to_uspace(pinfo, &uinfo, sizeof(*pinfo));

	if(err) 
	{ 
		printk(INFO, "%s: error copying result to userland, err %d\n", 
		       __FUNCTION__,
		       err);

		err = EBADE; 
		goto SYS_MCNTL_ERR;
	}

	return 0;

SYS_MCNTL_ERR:
	printk(DEBUG, "DEBUG: %s: cpu %d, tid %x, vaddr %x, err %d\n", 
	       __FUNCTION__,
	       cpu_get_id(),
	       current_thread,
	       vaddr,
	       err);

	current_thread->info.errno = err;
	return err; 
}
