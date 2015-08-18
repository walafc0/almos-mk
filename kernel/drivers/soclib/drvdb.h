/*
 * drvdb.h - a dictionary of supported devices and their drivers
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

#ifndef _DRVDB_H_
#define _DRVDB_H_

#include <driver.h>

/* Registered Drivers */
typedef enum
{
	SOCLIB_RAM_ID = 0,
	SOCLIB_XICU_ID,
	SOCLIB_TTY_ID,
	SOCLIB_DMA_ID,
	SOCLIB_BLKDEV_ID,
	SOCLIB_FB_ID,
	SOCLIB_ICU_ID,
	SOCLIB_TIMER_ID,
	SOCLIB_IOPIC_ID,
	SOCLIB_MEMC_ID,
	SOCLIB_MNIC_ID,
	SOCLIB_CHDMA_ID,
	SOCLIB_RESERVED_ID,
	SOCLIB_DRVID_NR
}soclib_drvid_t;

/* Driver DataBase Entry */
struct drvdb_entry_s;

/* Entry Setters */
#define drvdb_entry_set_id(entry,drvid)
#define drvdb_entry_set_name(entry,name)
#define drvdb_entry_set_info(entry,info)
#define drvdb_entry_set_driver(entry,driver)

/* Entry Getters */
#define drvdb_entry_get_id(entry)
#define drvdb_entry_get_name(entry)
#define drvdb_entry_get_info(entry)
#define drvdb_entry_get_driver(entry)

/* Drivers DataBase Operations */
struct drvdb_entry_s *drvdb_locate_byId(uint_t drvid);
struct drvdb_entry_s *drvdb_locate_byName(char *name);
error_t   drvdb_set_driver(uint_t drvid, driver_t *driver);
driver_t* drvdb_get_driver(uint_t drvid);


/////////////////////////////////////////
//           Private Section           //
/////////////////////////////////////////

/* Driver DataBase Entry */
struct drvdb_entry_s
{
	uint_t id;
	char *name;
	char *info;
	driver_t *driver;
};

#undef drvdb_entry_get_id
#undef drvdb_entry_get_name
#undef drvdb_entry_get_info
#undef drvdb_entry_get_driver
#undef drvdb_entry_set_id
#undef drvdb_entry_set_name
#undef drvdb_entry_set_info
#undef drvdb_entry_set_driver

#define drvdb_entry_get_id(_entry)       ((_entry)->id)
#define drvdb_entry_get_name(_entry)     ((_entry)->name)
#define drvdb_entry_get_info(_entry)     ((_entry)->info)
#define drvdb_entry_get_driver(_entry)   ((_entry)->driver)

#define drvdb_entry_set_id(_entry,_drvid)      do {(_entry)->id = (_drvid);}      while(0)
#define drvdb_entry_set_name(_entry,_name)     do {(_entry)->name = (_name);}     while(0)
#define drvdb_entry_set_info(_entry,_info)     do {(_entry)->info = (_info);}     while(0)
#define drvdb_entry_set_driver(_entry,_driver) do {(_entry)->driver = (_driver);} while(0)

#endif	/* _DRVDB_H_ */
