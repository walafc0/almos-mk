/*
 * kern/sys_utls.c - User Thread Local Storage
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
#include <thread.h>
#include <kmem.h>
#include <kmagics.h>
#include <semaphore.h>

#define UTLS_SET       1
#define UTLS_GET       2
#define UTLS_GET_ERRNO 3

int sys_utls(uint_t operation, uint_t value)
{
	struct thread_s *this = current_thread;
  
	switch(operation)
	{
	case UTLS_SET:
		this->info.usr_tls = value;
		return 0;

	case UTLS_GET:
		return this->info.usr_tls;

	case UTLS_GET_ERRNO:
		return this->info.errno;

	default:
		this->info.errno = EINVAL;
		return -1;
	}
}
