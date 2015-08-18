/*
 * kdmsg.c - output kernel debug/trace/information to an available terminal
 * (see kern/kdmsg.h)
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
#include <hardware.h>
#include <cpu.h>
#include <device.h>
#include <tty.h>
#include <libk.h>
#include <kdmsg.h>
#include <spinlock.h>

spinlock_t exception_lock;
spinlock_t printk_lock;
spinlock_t isr_lock;

void kdmsg_init()
{
	spinlock_init(&exception_lock);
	spinlock_init(&printk_lock);
	spinlock_init(&isr_lock);
}

int __fprintk (int tty, spinlock_t *lock, const char *fmt, ...)
{
	va_list ap;
	int count;
	char buff[256];
	dev_request_t rq;

	va_start (ap, fmt);

	assert(tty == 0 && "Unexpected tty device id");
	count = iprintk (buff, 1, (char *) fmt, ap);
	assert(count < 256 && "Buffer overflow");

	va_end (ap);
	rq.src   = buff;
	rq.count = count;
	rq.flags = DEV_RQ_NOBLOCK;

	if(lock)
	{
		if(lock == &printk_lock)
			spinlock_lock(&ttys_tbl[tty].lock);
		else
			cpu_spinlock_lock(&ttys_tbl[tty].lock.val);
	}

	ttys_tbl[tty].op.dev.write(&ttys_tbl[tty], &rq);
   
	if(lock)
	{
		if(lock == &printk_lock)
			spinlock_unlock(&ttys_tbl[tty].lock);
		else
			cpu_spinlock_unlock(&ttys_tbl[tty].lock.val);
	}

	return count;
}

int __perror (int fatal, const char *fmt, ...)
{
	va_list ap;
	int count;
	char buff[256];
	dev_request_t rq;

	va_start (ap, fmt);
	count = iprintk (buff, 1, (char *) fmt, ap);
	va_end (ap);
   
	rq.src = buff;
	rq.count = count;
	rq.flags = DEV_RQ_NOBLOCK;

	ttys_tbl[0].op.dev.write(&ttys_tbl[0], &rq);

	if(fatal)
		while(1);

	return count;
}

void bdump(uint_t fd, char* buff, size_t count)
{
	register uint_t i,j,size;
	char *tab = "0123456789ABCDEF";
	char tmp[256];
	dev_request_t rq;
     
	assert((fd != 0) && "file descriptor is not a tty");
	i=0;

	while(count)
	{
		size = (count < 256) ? count : 256;
    
		for(j=0; j < size; j++, i++)
			tmp[j] = tab[ (buff[i] & 0xF)];
		
		rq.src   = tmp;
		rq.count = 256;
		rq.flags = 0;
    
		ttys_tbl[0].op.dev.write(&ttys_tbl[0], &rq);

		count -= size;
	}
}
