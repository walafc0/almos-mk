/*
 * soclib_memc.c - soclib/tsar L2 Confugration Controller
 *
 * Copyright (c) 2008,2009,2010,2011,2012,2013 Ghassan Almaless
 * Copyright (c) 2011,2012,2013 UPMC Sorbonne Universites
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

#include <bits.h>
#include <device.h>
#include <driver.h>
#include <drvdb.h>
#include <errno.h>
#include <vfs.h>
#include <string.h>
#include <soclib_memc.h>
#include <vmm.h>

#define MEMC_CNF_FUNC         0x0
#define MEMC_PRF_FUNC         0x1
#define MEMC_ERR_FUNC         0x2

#define MEMC_RD_MUPDT_SEL     0x0
#define MEMC_WR_MINVL_SEL     0x1
#define MEMC_LL_CLNUP_SEL     0x2
#define MEMC_SC_MUPDTT_SEL    0x3
#define MEMC_CS_MINVLT_SEL    0x4
#define MEMC_XX_BINVLT_SEL    0x5

#define MEMC_LOC_SEL          0x0
#define MEMC_REM_SEL          0x1
#define MEMC_OTH_SEL          0x2

#define MEMC_DIRCT_SEL        0x0
#define MEMC_COHER_SEL        0x1

#define MEMC_LO_SEL           0x0
#define MEMC_HI_SEL           0x1

#define MEMC_REG_IDX(z,y,x,w)  (((z) << 6) | ((y) << 4) | ((x) << 1) | (w))
#define MEMC_REG(func,idx)     (((func) << 9) | ((idx) << 2))

/* ---------------------------------------- */
/* PERFORMANCE REGISTERS FOR DIRECT TRAFFIC */
/* ---------------------------------------- */

#define MEMC_RD_LIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_LOC_SEL, MEMC_RD_MUPDT_SEL, MEMC_LO_SEL))
#define MEMC_RD_RIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_REM_SEL, MEMC_RD_MUPDT_SEL, MEMC_LO_SEL))
#define MEMC_RD_CIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_OTH_SEL, MEMC_RD_MUPDT_SEL, MEMC_LO_SEL))

#define MEMC_WR_LIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_LOC_SEL, MEMC_WR_MINVL_SEL, MEMC_LO_SEL))
#define MEMC_WR_RIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_REM_SEL, MEMC_WR_MINVL_SEL, MEMC_LO_SEL))
#define MEMC_WR_CIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_OTH_SEL, MEMC_WR_MINVL_SEL, MEMC_LO_SEL))

#define MEMC_LL_LIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_LOC_SEL, MEMC_LL_CLNUP_SEL, MEMC_LO_SEL))
#define MEMC_LL_RIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_REM_SEL, MEMC_LL_CLNUP_SEL, MEMC_LO_SEL))
#define MEMC_LL_CIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_OTH_SEL, MEMC_LL_CLNUP_SEL, MEMC_LO_SEL))

#define MEMC_SC_LIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_LOC_SEL, MEMC_SC_MUPDTT_SEL, MEMC_LO_SEL))
#define MEMC_SC_RIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_REM_SEL, MEMC_SC_MUPDTT_SEL, MEMC_LO_SEL))
#define MEMC_SC_CIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_OTH_SEL, MEMC_SC_MUPDTT_SEL, MEMC_LO_SEL))

#define MEMC_CS_LIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_LOC_SEL, MEMC_CS_MINVLT_SEL, MEMC_LO_SEL))
#define MEMC_CS_RIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_REM_SEL, MEMC_CS_MINVLT_SEL, MEMC_LO_SEL))
#define MEMC_CS_CIDX      (MEMC_REG_IDX(MEMC_DIRCT_SEL, MEMC_OTH_SEL, MEMC_CS_MINVLT_SEL, MEMC_LO_SEL))

#define MEMC_RD_LOCAL      MEMC_REG(MEMC_PRF_FUNC, MEMC_RD_LIDX)
#define MEMC_RD_REMOTE     MEMC_REG(MEMC_PRF_FUNC, MEMC_RD_RIDX)
#define MEMC_RD_COST       MEMC_REG(MEMC_PRF_FUNC, MEMC_RD_CIDX)

#define MEMC_WR_LOCAL      MEMC_REG(MEMC_PRF_FUNC, MEMC_WR_LIDX)
#define MEMC_WR_REMOTE     MEMC_REG(MEMC_PRF_FUNC, MEMC_WR_RIDX)
#define MEMC_WR_COST       MEMC_REG(MEMC_PRF_FUNC, MEMC_WR_CIDX)

#define MEMC_LL_LOCAL      MEMC_REG(MEMC_PRF_FUNC, MEMC_LL_LIDX)
#define MEMC_LL_REMOTE     MEMC_REG(MEMC_PRF_FUNC, MEMC_LL_RIDX)
#define MEMC_LL_COST       MEMC_REG(MEMC_PRF_FUNC, MEMC_LL_CIDX)

#define MEMC_SC_LOCAL      MEMC_REG(MEMC_PRF_FUNC, MEMC_SC_LIDX)
#define MEMC_SC_REMOTE     MEMC_REG(MEMC_PRF_FUNC, MEMC_SC_RIDX)
#define MEMC_SC_COST       MEMC_REG(MEMC_PRF_FUNC, MEMC_SC_CIDX)

#define MEMC_CS_LOCAL      MEMC_REG(MEMC_PRF_FUNC, MEMC_CS_LIDX)
#define MEMC_CS_REMOTE     MEMC_REG(MEMC_PRF_FUNC, MEMC_CS_RIDX)
#define MEMC_CS_COST       MEMC_REG(MEMC_PRF_FUNC, MEMC_CS_CIDX)

/* ------------------------------------------- */
/* PERFORMANCE REGISTERS FOR COHERENCE TRAFFIC */
/* ------------------------------------------- */

#define MEMC_MUPDT_LIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_LOC_SEL, MEMC_RD_MUPDT_SEL, MEMC_LO_SEL))
#define MEMC_MUPDT_RIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_REM_SEL, MEMC_RD_MUPDT_SEL, MEMC_LO_SEL))
#define MEMC_MUPDT_CIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_OTH_SEL, MEMC_RD_MUPDT_SEL, MEMC_LO_SEL))

#define MEMC_MINVL_LIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_LOC_SEL, MEMC_WR_MINVL_SEL, MEMC_LO_SEL))
#define MEMC_MINVL_RIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_REM_SEL, MEMC_WR_MINVL_SEL, MEMC_LO_SEL))
#define MEMC_MINVL_CIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_OTH_SEL, MEMC_WR_MINVL_SEL, MEMC_LO_SEL))

#define MEMC_CLNUP_LIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_LOC_SEL, MEMC_LL_CLNUP_SEL, MEMC_LO_SEL))
#define MEMC_CLNUP_RIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_REM_SEL, MEMC_LL_CLNUP_SEL, MEMC_LO_SEL))
#define MEMC_CLNUP_CIDX      (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_OTH_SEL, MEMC_LL_CLNUP_SEL, MEMC_LO_SEL))

#define MEMC_MUPDTT_TIDX     (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_OTH_SEL, MEMC_SC_MUPDTT_SEL, MEMC_LO_SEL))
#define MEMC_MINVLT_TIDX     (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_OTH_SEL, MEMC_CS_MINVLT_SEL, MEMC_LO_SEL))
#define MEMC_BINVLT_TIDX     (MEMC_REG_IDX(MEMC_COHER_SEL, MEMC_OTH_SEL, MEMC_XX_BINVLT_SEL, MEMC_LO_SEL))

#define MEMC_MUPDT_LOCAL     MEMC_REG(MEMC_PRF_FUNC, MEMC_MUPDT_LIDX)
#define MEMC_MUPDT_REMOTE    MEMC_REG(MEMC_PRF_FUNC, MEMC_MUPDT_RIDX)
#define MEMC_MUPDT_COST      MEMC_REG(MEMC_PRF_FUNC, MEMC_MUPDT_CIDX)

#define MEMC_MINVL_LOCAL     MEMC_REG(MEMC_PRF_FUNC, MEMC_MINVL_LIDX)
#define MEMC_MINVL_REMOTE    MEMC_REG(MEMC_PRF_FUNC, MEMC_MINVL_RIDX)
#define MEMC_MINVL_COST      MEMC_REG(MEMC_PRF_FUNC, MEMC_MINVL_CIDX)

#define MEMC_CLNUP_LOCAL     MEMC_REG(MEMC_PRF_FUNC, MEMC_CLNUP_LIDX)
#define MEMC_CLNUP_REMOTE    MEMC_REG(MEMC_PRF_FUNC, MEMC_CLNUP_RIDX)
#define MEMC_CLNUP_COST      MEMC_REG(MEMC_PRF_FUNC, MEMC_CLNUP_CIDX)

#define MEMC_MUPDT_TOTAL     MEMC_REG(MEMC_PRF_FUNC, MEMC_MUPDTT_TIDX)
#define MEMC_MINVL_TOTAL     MEMC_REG(MEMC_PRF_FUNC, MEMC_MINVLT_TIDX)
#define MEMC_BINVL_TOTAL     MEMC_REG(MEMC_PRF_FUNC, MEMC_BINVLT_TIDX)

#define MEMC_REG_NR 27

static const uint16_t reg_tbl[MEMC_REG_NR] = {
		MEMC_RD_LOCAL,
		MEMC_RD_REMOTE,
		MEMC_RD_COST,
		MEMC_WR_LOCAL,
		MEMC_WR_REMOTE,
		MEMC_WR_COST,
		MEMC_LL_LOCAL,
		MEMC_LL_REMOTE,
		MEMC_LL_COST,
		MEMC_SC_LOCAL,
		MEMC_SC_REMOTE,
		MEMC_SC_COST,
		MEMC_CS_LOCAL,
		MEMC_CS_REMOTE,
		MEMC_CS_COST,
		MEMC_MUPDT_LOCAL,
		MEMC_MUPDT_REMOTE,
		MEMC_MUPDT_COST,
		MEMC_MINVL_LOCAL,
		MEMC_MINVL_REMOTE,
		MEMC_MINVL_COST,
		MEMC_CLNUP_LOCAL,
		MEMC_CLNUP_REMOTE,
		MEMC_CLNUP_COST,
		MEMC_MUPDT_TOTAL,
		MEMC_MINVL_TOTAL,
		MEMC_BINVL_TOTAL};

static sint_t memc_read(struct device_s *memc, dev_request_t *rq)
{	
	register size_t count;
	register uint_t i;
	uint_t *dst;
		      
	assert(rq->file != NULL);
  
	count = rq->count >> 2;	/* 32 bits registers */
	dst   = rq->dst;

	count = (count > MEMC_REG_NR) ? MEMC_REG_NR : count;

	for(i = 0; i < count; i++)
	{
		memcpy((void*)&dst[i], (void *)((uint_t)memc->base + reg_tbl[i]), 4);
	}
	return (count << 2);
}

static sint_t memc_write(struct device_s *memc, dev_request_t *rq)
{
	return -ENOTSUPPORTED;
}

static error_t memc_lseek(struct device_s *memc, dev_request_t *rq)
{
	return ENOTSUPPORTED;
}

static sint_t memc_get_params(struct device_s *memc, dev_params_t *params)
{
	params->size = MEMC_REG_NR * 4; /* 32 bits registers */
	return 0;
}

static sint_t memc_open(struct device_s *memc, dev_request_t *rq)
{
	return 0;
}

static VM_REGION_PAGE_FAULT(memc_pagefault)
{
	return EFAULT;
}

static struct vm_region_op_s memc_vm_region_op = 
{
	.page_in     = NULL,
	.page_out    = NULL,
	.page_lookup = NULL,
	.page_fault  = memc_pagefault
};

static error_t memc_mmap(struct device_s *memc, dev_request_t *rq)
{
	struct vfs_file_s *file;
	struct vm_region_s *region;
	struct pmm_s *pmm;
	uint_t size;
	pmm_page_info_t info;
	vma_t current_vma;
	error_t err;

	file   = rq->file;
	region = rq->region;
	pmm    = &region->vmm->pmm;
	size   = (uint_t) memc->data;
  
	if(size < (region->vm_limit - region->vm_start))
	{
		printk(WARNING, "WARNING: %s: asked size (%d) exceed real one (%d)\n", 
		       __FUNCTION__, region->vm_limit - region->vm_start, size);
		return ERANGE;
	}

	printk(INFO, "INFO: %s: started, file inode %p, region <0x%x - 0x%x>\n",
	       __FUNCTION__, 
	       file->f_inode, 
	       region->vm_start, 
	       region->vm_limit);

	if((err = pmm_get_page(pmm, (vma_t)memc->base, &info)))
		return err;

	region->vm_flags |= VM_REG_DEV;
	current_vma       = region->vm_start;
	info.attr         = region->vm_pgprot & ~(PMM_CACHED);
	info.cluster      = NULL;
	size              = region->vm_limit - region->vm_start; 
	size              = ARROUND_UP(size, PMM_PAGE_SIZE);

	while(size)
	{
		if((err = pmm_set_page(pmm, current_vma, &info)))
			return err;

		info.ppn ++;
		current_vma += PMM_PAGE_SIZE;
		size -= PMM_PAGE_SIZE;
	}
  
	region->vm_file = *file;
	//region->vm_mapper = NULL;

	region->vm_op = &memc_vm_region_op;
	return 0;
}

static error_t memc_munmap(struct device_s *memc, dev_request_t *rq)
{
	struct vfs_file_s *file;
	struct vm_region_s *region;
	struct pmm_s *pmm;
	uint_t size;
	pmm_page_info_t info;
	vma_t current_vma;
	error_t err;

	file   = rq->file;
	region = rq->region;
	pmm    = &region->vmm->pmm;
	size   = (uint_t) memc->data;

	if(size < (region->vm_limit - region->vm_start))
	{
		printk(ERROR, "ERROR: %s: asked size (%d) exceed real one (%d)\n", 
		       __FUNCTION__, region->vm_limit - region->vm_start, size);
		return ERANGE;
	}

	current_vma = region->vm_start;
	info.attr   = 0;
	info.ppn    = 0;
	size        = region->vm_limit - region->vm_start;

	while(size)
	{    
		if((err = pmm_set_page(pmm, current_vma, &info)))
			return err;

		current_vma += PMM_PAGE_SIZE;
		size -= PMM_PAGE_SIZE;
	}
  
	return 0;
}

static uint_t memc_count = 0;

error_t soclib_memc_init(struct device_s *memc)
{  
	memc->type = DEV_CHR;

	memc->op.dev.open       = &memc_open;
	memc->op.dev.read       = &memc_read;
	memc->op.dev.write      = &memc_write;
	memc->op.dev.close      = NULL;
	memc->op.dev.lseek      = &memc_lseek;
	memc->op.dev.mmap       = &memc_mmap;
	memc->op.dev.munmap     = &memc_munmap;
	memc->op.dev.set_params = NULL;
	memc->op.dev.get_params = &memc_get_params;
	memc->op.drvid          = SOCLIB_MEMC_ID;

	memc->data = (void*)memc->size;

	sprintk(memc->name, 
#if CONFIG_ROOTFS_IS_VFAT
		"MEMC%d"
#else
		"memc%d"
#endif
		, memc_count++);

	metafs_init(&memc->node, memc->name);
	return 0;
}


driver_t soclib_memc_driver = { .init = &soclib_memc_init };
