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
#include <cpu.h>
#include <device.h>
#include <soclib_tty.h>
#include <arch-config.h>
#include <spinlock.h>
#include <distlock.h>
#include <mcs_sync.h>
#include <libk.h>
#include <kdmsg.h>

#define MSB(paddr) ((uint_t)((paddr)>>32))
#define LSB(paddr) ((uint_t)(paddr))

//spinlock_t printk_lock;
DISTLOCK_DECLARE(printk_lock);
//spinlock_t isr_lock;
DISTLOCK_DECLARE(isr_lock);

DISTLOCK_DECLARE(boot_lock);

kdmsg_channel_t klog_tty = {{.id = 1}};
kdmsg_channel_t kisr_tty = {{.id = 1}};
kdmsg_channel_t kexcept_tty = {{.id = 1}};

void kdmsg_init()
{
	uint_t tty_count;
  
	tty_count = 0;
	while((tty_count < TTY_DEV_NR) && (ttys_tbl[tty_count++] != NULL));
  
	if(tty_count >= 3)
	{
		kisr_tty.id = 3;
		kexcept_tty.id = 3;
	}
	
}

uint_t boot_cluster;
uint_t kboot_tty_cid;
uint_t kboot_tty_base;

void kboot_tty_init(boot_info_t *info)
{
	kboot_tty_base  = info->tty_base;

	kboot_tty_cid   = info->tty_cid;

	boot_cluster	= info->boot_cluster_id;
}

//TODO ?
int early_boot_dmsg (const char *fmt, ...)
{
	va_list ap;
	int count;

	//if(current_cluster->cid != boot_cluster) 
	//	return 0;

	va_start (ap, fmt);

	count = iprintk ((char*)kboot_tty_base, kboot_tty_cid, 0, (char *) fmt, ap);

	va_end (ap);
	
	return count;
}

int __arch_boot_dmsg (const char *fmt, ...)
{
	va_list ap;
	int count;


	va_start (ap, fmt);
		
	distlock_lock(&boot_lock);

	count = iprintk ((char*)kboot_tty_base, kboot_tty_cid, 0, (char *) fmt, ap);

	distlock_unlock(&boot_lock);

	va_end (ap);

	return count;
}

int __fprintk (int tty, ktty_lock_t ktty, const char *fmt, ...)
{
	va_list ap;
	int count;
	//uint_t irq_state;
	distlock_t *lock;

	va_start (ap, fmt);

	switch(ktty)
	{
		case PRINTK_KTTY_LOCK:
			lock = &printk_lock;
			break;
		case ISR_KTTY_LOCK:
			lock = &isr_lock;
			break;
		default:
		case NO_KTTY_LOCK:
			lock = NULL;
			break;
	}

	if(lock)
	{
		distlock_lock(lock);
	}
  
	count = iprintk ((char*)(ttys_tbl[tty]->base + TTY_WRITE_REG), 
			ttys_tbl[tty]->cid, 0, (char *) fmt, ap);
   
	if(lock)
	{
		distlock_unlock(lock);
	}

	va_end (ap);

	return count;
}

#include <thread.h>

int __perror (int fatal, const char *fmt, ...)
{
	va_list ap;
	int count;

	va_start (ap, fmt);
	count = iprintk ((char*)(ttys_tbl[kexcept_tty.id]->base + TTY_WRITE_REG), 
				ttys_tbl[kexcept_tty.id]->cid, 
				0, (char *) fmt, ap);

	va_end (ap);
   
	if(fatal) while(1);
	return count;
}

#if 0
//FIXME add 40 bits support
void bdump(uint8_t *buff, size_t count)
{
	uint8_t b1, b2;
	char *tab = "0123456789ABCDEF";

	spinlock_lock(&ttys_tbl[klog_tty.id]->lock);

	while(count--) 
	{
		b1 = tab[(*buff & 0xF0)>>4];
		b2 = tab[*buff & 0x0F];
		buff++;
		*(char*)(ttys_tbl[klog_tty.id]->base + TTY_WRITE_REG) = b1;
		*(char*)(ttys_tbl[klog_tty.id]->base + TTY_WRITE_REG) = b2;
	}
	*(char*)(ttys_tbl[klog_tty.id]->base + TTY_WRITE_REG) = '\n';
	spinlock_unlock (&ttys_tbl[klog_tty.id]->lock);
}
#endif
