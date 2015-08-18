/*
 * drvdb.c - a dictionary of supported devices and their drivers
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
#include <libk.h>
#include <event.h>
#include <device.h>
#include <driver.h>
#include <drvdb.h>
#include <soclib_xicu.h>
#include <soclib_icu.h>
#include <soclib_tty.h>
#include <soclib_timer.h>
#include <soclib_dma.h>
#include <soclib_block.h>
#include <soclib_fb.h>
#include <soclib_memc.h>
#include <soclib_iopic.h>


static struct drvdb_entry_s drivers_db[SOCLIB_DRVID_NR] = 
{
	{SOCLIB_RAM_ID,"RAM","Memory Module", NULL},
	{SOCLIB_XICU_ID,"XICU","Interrupt Control Unite With Integrated RT-Timers", &soclib_xicu_driver},
	{SOCLIB_TTY_ID,"TTY","Emulated Terminal Controler", &soclib_tty_driver},
	{SOCLIB_DMA_ID,"DMA","DMA Channel", &soclib_dma_driver},
	{SOCLIB_BLKDEV_ID,"BLKDEV","H.D.D Controler", &soclib_blkdev_driver},
	{SOCLIB_FB_ID,"FB","Frame Buffer Controler", &soclib_fb_driver},
	{SOCLIB_ICU_ID,"ICU","Interrupt Control Unite", &soclib_icu_driver},
	{SOCLIB_TIMER_ID,"TIMER","Real-Time Timer", &soclib_timer_driver},
	{SOCLIB_IOPIC_ID,"IOPIC","Hardware Interrupt To Software Interrupt Translator", &soclib_iopic_driver},
	{SOCLIB_MEMC_ID,"MEMC","L2 Configuration Interface", &soclib_memc_driver},
	{SOCLIB_MNIC_ID,"MNIC","Multi-Channels GMII-Compliant NIC Controller", NULL},
	{SOCLIB_CHDMA_ID,"CHDMA","Multi-Channels DMA Controller Supporting Chained Buffers", NULL},
	{SOCLIB_RESERVED_ID, NULL, NULL, NULL}
};


struct drvdb_entry_s *drvdb_locate_byId(uint_t drvid)
{
	if(drvid >= SOCLIB_RESERVED_ID)
		return NULL;

	return &drivers_db[drvid];
}

struct drvdb_entry_s *drvdb_locate_byName(char *name)
{
	register uint_t i;

	for(i=0; drivers_db[i].id != SOCLIB_RESERVED_ID; i++)
		if(!strcmp(drivers_db[i].name, name))
			return &drivers_db[i];

	return NULL;
}

driver_t* drvdb_get_driver(uint_t drvid)
{
	if(drvid >= SOCLIB_RESERVED_ID)
		return NULL;

	return drivers_db[drvid].driver;
}

error_t drvdb_set_driver(uint_t drvid, driver_t *driver)
{
	if(drvid >= SOCLIB_RESERVED_ID)
		return ERANGE;

	drivers_db[drvid].driver = driver;
	return 0;
}
