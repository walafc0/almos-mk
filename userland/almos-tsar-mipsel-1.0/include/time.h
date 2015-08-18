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
  
   UPMC / LIP6 / SOC (c) 2008, 2015
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/


#ifndef _TIME_H_
#define _TIME_H_

#include <sys/types.h>

#define CLOCKS_PER_SEC 200000

struct timespec {
  time_t tv_sec;    /* secondes */
  long   tv_nsec;   /* nanosecondes */
};

struct timeval {
  clock_t tv_sec;    /* secondes */
  clock_t tv_usec;   /* microsecondes */
};

struct timezone {
  int tz_minuteswest;     /* minutes west of Greenwich */
  int tz_dsttime;         /* type of DST correction */
};

struct timeb {
	time_t         time;
	unsigned short millitm;
	short          timezone;
	short          dstflag;
};

clock_t clock(void);
int gettimeofday(struct timeval *tv, struct timezone *tz);
time_t time(time_t *t);
int ftime(struct timeb *tp);

#endif	/* _TIME_H_ */
