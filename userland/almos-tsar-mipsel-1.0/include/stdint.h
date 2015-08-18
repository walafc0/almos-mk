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
  
   UPMC / LIP6 / SOC (c) 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#ifndef _STDINT_H_
#define _STDINT_H_

/* Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int int16_t;
typedef unsigned short int uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef signed long long int int64_t;
typedef unsigned long long int uint64_t;

/* Integer type capable of holding object pointers */
typedef uint32_t uintptr_t;
typedef int32_t  intptr_t;

/* Fastest integer types for 32-bits architecture*/
typedef int32_t int_fast8_t;
typedef uint32_t uint_fast8_t;

typedef int32_t int_fast16_t;
typedef uint32_t uint_fast16_t;

typedef int32_t int_fast32_t;
typedef uint32_t uint_fast32_t;

typedef int64_t int_fast64_t;
typedef uint64_t uint_fast64_t ;

/* Integer type capable of holding object sizes */
typedef uint32_t size_t;
typedef int32_t ssize_t;

/* Minimum of signed integer types */
#define INT8_MIN       (-128)
#define INT16_MIN      (-32767-1)
#define INT32_MIN      (-2147483647-1)

/* Maximum of signed integral types.  */
#define INT8_MAX       (127)
#define INT16_MAX      (32767)
#define INT32_MAX      (2147483647)

/* Maximum of unsigned integral types.  */
#define UINT8_MAX      (255)
#define UINT16_MAX     (65535)
#define UINT32_MAX     (4294967295U)

#endif
