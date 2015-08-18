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
   Copyright Ghassan Almaless <ghassan.almaless@lip6.fr>
*/

#ifndef _SGI_IMG_H_
#define _SGI_IMG_H_

#include <stdint.h>

#define SGI_ALIGN4(val)				\
  (((val) & 0x000000FF) << 24  |		\
   ((val) & 0x0000FF00) << 8   |		\
   ((val) & 0x00FF0000) >> 8   |		\
   (val) >> 24)

#define SGI_ALIGN2(val)				\
  (((val) & 0x00FF)     << 8   |		\
   (val) >> 8)

#define SGI_TO_LE2(val)				\
  (((val) & 0xFF00)     >> 8   |		\
   (val) << 8)

struct sgi_info_s
{
  uint16_t   magic;
  uint8_t    storage;
  uint8_t    bpc;
  uint16_t   dimension;
  uint16_t   x_size;
  uint16_t   y_size;
  uint16_t   z_size;
  uint32_t   pix_min;
  uint32_t   pix_max;
  uint32_t   dummy1;
  uint8_t    img_name[80];
  uint32_t   color_map;
  uint8_t    dummy2[404];
} __attribute__ ((packed));


#endif
