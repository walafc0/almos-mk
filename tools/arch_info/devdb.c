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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "devdb.h"

typedef struct record_device_s
{
  char *name;
  char *info;
  uint8_t id;
}record_dev_t;

static record_dev_t dev_db[] = {
  {"RAM", "Memory Module", SOCLIB_RAM_ID},
  {"XICU", "Interrupt Control Unite With Integrated Real-Time Timers", SOCLIB_XICU_ID},
  {"TTY", "Emulated Terminal Controler", SOCLIB_TTY_ID},
  {"DMA", "DMA Channel", SOCLIB_DMA_ID},
  {"BLKDEV", "H.D.D Controler", SOCLIB_BLKDEV_ID},
  {"FB", "Frame Buffer Controler", SOCLIB_FB_ID},
  {"ICU","Interrupt Control Unite", SOCLIB_ICU_ID},
  {"IOPIC","Harware Interrupt to WTI Interrupt Translator", SOCLIB_IOPIC_ID},
  {"TIMER", "Real-Time Timer", SOCLIB_TIMER_ID},
  {NULL, NULL, SOCLIB_RESERVED_ID}};

int dev_locate_by_name(char *devid, uint8_t *id)
{
  int i;
  for(i=0; dev_db[i].name != NULL; i++)
  {
    if(!strcmp(devid, dev_db[i].name))
    {
      log_msg("Found Device %s: %s\n", devid, dev_db[i].info);
      *id = dev_db[i].id;
      return 0;
    }
  }
  return EDEVID;
}

char* dev_locate_by_id(uint8_t id)
{
  int i;
  for(i=0; dev_db[i].name != NULL; i++)
  {
    if(dev_db[i].id == id)
      return dev_db[i].name;
  }
  return NULL;
}
