/*
   This file is part of MutekP.
   COpied from GLibC
  
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
  
   UPMC / LIP6 / SOC (c) 2015
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

/* Obsolete function to get current working directory.
   Copyright (C) 1991-2013 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>


char *
getwd (buf)
     char *buf;
{
  char tmpbuf[PATH_MAX];

  if (buf == NULL)
    {
      __set_errno (EINVAL);
      return NULL;
    }

  if (getcwd (tmpbuf, PATH_MAX) == NULL)
    {
      return NULL;
    }

  /* This is completely unsafe.  Nobody can say how big the user
     provided buffer is.  Perhaps the application and the libc
     disagree about the value of PATH_MAX.  */
  return strcpy (buf, tmpbuf);
}

/* link_warning (getwd,
	      "the `getwd' function is dangerous and should not be used.")*/
