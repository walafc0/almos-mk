/*
    This file is part of AlmOS.

    AlmOS is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    AlmOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AlmOS; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

    UPMC / LIP6 / SOC (c) 2008
    Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/


#ifndef _DEVDB_H_
#define _DEVDB_H_

#include <stdint.h>

/* Registered Drivers */
typedef enum
{
  SOCLIB_RAM_ID  = 0,
  SOCLIB_XICU_ID,
  SOCLIB_TTY_ID,
  SOCLIB_DMA_ID,
  SOCLIB_BLKDEV_ID,
  SOCLIB_FB_ID,
  SOCLIB_ICU_ID,
  SOCLIB_TIMER_ID,
  SOCLIB_IOPIC_ID,
  SOCLIB_RESERVED_ID,
  SOCLIB_DRVID_NR
}soclib_drvid_t;

/* Locate Device Id Given It's Name */
int dev_locate_by_name(char *devid, uint8_t *id);

/* Locate Device Name Given It's Id */
char* dev_locate_by_id(uint8_t id);

#endif	/* _DEVDB_H_ */
