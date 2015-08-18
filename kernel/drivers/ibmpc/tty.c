/*
 * tty.c - a basic tty driver
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

#include <config.h>
#include <system.h>
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <cpu-io.h>
#include <kmem.h>
#include <libk.h>
#include <tty.h>
#include <rwlock.h>
#include <cpu-trace.h>
#include <thread.h>
#include <mwmr.h>

struct device_s ttys_tbl[TTY_DEV_NR];

static void tty_clear(struct device_s *tty)
{
	register uint_t i = 0;
	struct tty_context_s *ctx;
	uint8_t *vga_ram;

	ctx = (struct tty_context_s*)tty->data;
	vga_ram = (uint8_t*)tty->base;

	for(i = 0; i < TTY_SIZE; i+=2)
	{
		vga_ram[i] = 0;
		vga_ram[i+1] = ctx->attr;
	}
}


static sint_t tty_read(struct device_s *tty, dev_request_t *rq)
{
	struct tty_context_s *ctx; 
	register size_t size;
	uint_t irq_state;
	register char *dst;
	register size_t count;
	register uint32_t eol = 0;
   
	cpu_trace_write(current_thread()->local_cpu, tty_read);

	ctx = (struct tty_context_s*)tty->data;
	size = 0;
	count = rq->count;
	dst = rq->dst;
   
	if(rq->flags & DEV_RQ_NOBLOCK)
		rwlock_rdlock(&ctx->rwlock);
	else
		rwlock_wrlock(&ctx->rwlock);

	spinlock_lock_noirq(&tty->lock, &irq_state);

	size = mwmr_read(ctx->in_channel, dst, count);
	eol = ctx->eol;
   
	if((eol) || (size == count) || (rq->flags & DEV_RQ_NOBLOCK))
	{
		ctx->eol = 0;
		spinlock_unlock_noirq(&tty->lock, irq_state);
		rwlock_unlock(&ctx->rwlock);
		return size;
	}

	rq->count -= size;
	rq->dst = dst + size;
	ctx->pending_rq = rq;

	wait_on(&ctx->wait_queue, WAIT_LAST);
	spinlock_unlock_noirq(&tty->lock, irq_state);
	sched_sleep(current_thread());
	rwlock_unlock(&ctx->rwlock);
	return count - rq->count;
}

void tty_scrollup(struct device_s *tty, int n)
{
	struct tty_context_s *ctx;
	uint8_t *vga_ram;
	uint8_t *ptr;
	uint8_t *limit = (uint8_t*)((uint_t)TTY_SIZE);

	ctx = (struct tty_context_s*)tty->data;
	vga_ram = (uint8_t*)tty->base;

	for(; vga_ram < limit; vga_ram+=2)
	{
		ptr=&vga_ram[n*2*TTY_XSIZE];
    
		if(ptr < limit) 
		{
			*vga_ram = *ptr;
			*(vga_ram + 1) = *(ptr+1);
		}
		else
		{
			*vga_ram = 0;
			*(vga_ram+1) = 0x07;
		}
	}
  
	ctx->pos_Y -= n;
	ctx->pos_Y = (ctx->pos_Y < 0) ? 0 : ctx->pos_Y;
}

static void tty_cleanup(struct device_s *tty)
{
	struct tty_context_s *ctx;

	ctx = (struct tty_context_s*)tty->data;
	tty_clear(tty);
	ctx->pos_Y = 0;
	ctx->pos_X = 0;
}

#define TTY_CURSOR_CMD        0x3d4
#define TTY_CURSOR_DATA       0x3d5

void tty_move_cursor(uint_t pos_X, uint_t pos_Y)
{
	uint_t pos;

	pos = pos_Y * TTY_XSIZE + pos_X;
	cpu_io_out8(TTY_CURSOR_CMD, 0x0f);
	cpu_io_out8(TTY_CURSOR_DATA, pos);
	cpu_io_out8(TTY_CURSOR_CMD, 0x0e);
	cpu_io_out8(TTY_CURSOR_DATA, pos >> 8);
}

static void tty_putc(struct device_s *tty, uint8_t ch)
{
	struct tty_context_s *ctx;
	uint8_t *vga_ram;
	int offset;

	ctx = (struct tty_context_s*)tty->data;
	vga_ram = (uint8_t*)tty->base;

	switch (ch)
	{
	case '\b':
		ctx->pos_X --;
		break;
	case '\t':
		ctx->pos_X += 8;
		break;

	case '\r':
		ctx->pos_X = 0;
		break;

	case '\n':	
		ctx->pos_X = 0;
		ctx->pos_Y ++;
		break;
  
	default:
		offset = 2*TTY_XSIZE*ctx->pos_Y + 2*ctx->pos_X;
		ctx->pos_X++;
		vga_ram[offset] = ch;
		vga_ram[offset + 1] = (uint8_t)(ctx->attr & 0xFF);
	}

	if(ctx->pos_X >= TTY_XSIZE)
	{
		ctx->pos_X = 0;
		ctx->pos_Y ++;
	}
  
#if 1
	if(ctx->pos_Y >= TTY_YSIZE)
	{
		tty_cleanup(tty);
		//ctx->pos_Y = 0;
	}
#endif
}


static sint_t tty_write(struct device_s *tty, dev_request_t *rq)
{
	struct tty_context_s *ctx; 
	register unsigned int i;
	register size_t size = rq->count;
  
	cpu_trace_write(current_thread()->local_cpu, tty_write);
  
	ctx = (struct tty_context_s*)tty->data;
  
	if(!(rq->flags & DEV_RQ_NOBLOCK))
		rwlock_wrlock(&ctx->rwlock);

	for (i = 0; i < size; i++)
		tty_putc(tty, *((uint8_t*)rq->src + i));

	if(!(rq->flags & DEV_RQ_NOBLOCK))
		rwlock_unlock(&ctx->rwlock);

	return size;
}


static sint_t tty_get_params(struct device_s *tty, dev_params_t *params)
{
	params->chr.xSize = TTY_XSIZE;
	params->chr.ySize = TTY_YSIZE;
	params->size = 0;
	return 0;
}

#define KEYBOARD_READ_REG     0x60
#define KEYBOARD_WRITE_REG    0x60
#define KEYBOARD_STATUS_REG   0x64
#define KEYBORAD_CMD_REG      0x64

#define KEY_IS_HOLD           0x80
#define KEY_IS_LEFT_SHIFT     0x29
#define KEY_IS_RIGHT_SHIFT    0x35
#define KEY_IS_ALT            0x37
#define KEY_IS_CTRL           0x1C
#define KEY_IS_CAPSLOCK       0x39

static uint_t isLeftShift;
static uint_t isRightShift;
static uint_t isAlt;
static uint_t isCtrl;
static uint_t isCAPSLOCK;

static error_t tty_getc(struct device_s *tty)
{
	register uint8_t isUpper;
	register uint8_t scode;
	register error_t ch;
	ch = -1;
	scode = 0;

	/* Waite KEYBOARD OUTPUT BUFFER */
	do
	{
		scode = cpu_io_in8(KEYBOARD_STATUS_REG);
	}while((scode & 0x1) == 0);
    
	scode = cpu_io_in8(KEYBOARD_READ_REG);
	scode --;
  
	//  isr_dmsg(DEBUG, "scode %x\n", scode);
  
	if(scode < KEY_IS_HOLD)
	{
		switch (scode)
		{
		case KEY_IS_RIGHT_SHIFT:
			isRightShift = 1;
			break;
		case KEY_IS_CTRL:
			isCtrl = 1;
			break;
		case KEY_IS_ALT:
			isAlt = 1;
			break;
		case KEY_IS_CAPSLOCK:
			break;
		case KEY_IS_LEFT_SHIFT:
			isLeftShift = 1;
			break;
		default:
			isUpper = (isRightShift || isLeftShift || isCAPSLOCK) ? 1 : 0;
			ch = keyboard_map[(scode << 2) + isUpper + isAlt];
		}
	}
	else
	{
		scode -= 0x80;
		switch (scode)
		{
		case KEY_IS_RIGHT_SHIFT:
			isRightShift = 0;
			break;
		case KEY_IS_CTRL:
			isCtrl = 0;
			break;
		case KEY_IS_ALT:
			isAlt = 0;
			break;
		case KEY_IS_CAPSLOCK:
			isCAPSLOCK = (isCAPSLOCK) ? 0 : 1;
			break;
		case KEY_IS_LEFT_SHIFT:
			isLeftShift = 0;
			break;
		default:
			/* No action for other scan code */
			break;
		}
	}
  
	struct tty_context_s *ctx = tty->data;
	tty_move_cursor(ctx->pos_X, ctx->pos_Y);
	return ch;
}

static void tty_irq_handler(struct irq_action_s *action)
{
	sint_t ch;
	struct device_s *tty;
	struct tty_context_s *ctx; 
	register char *dst;
	volatile uint32_t *base;
   
	cpu_trace_write(current_thread()->local_cpu, tty_irq_handler);

	tty = action->dev;
	base = tty->base;
	ctx = (struct tty_context_s*)tty->data;

	if((ch = tty_getc(tty)) & 0x80)
		return;

	cpu_spinlock_lock(&tty->lock.val);

	if(ctx->pending_rq != NULL)
	{
		dst = ctx->pending_rq->dst;
		*(dst ++) = ch;
		ctx->pending_rq->dst = dst;
		ctx->pending_rq->count --;

		if((ch == '\n') || (ctx->pending_rq->count == 0))
		{
			wakeup_one(&ctx->wait_queue, WAIT_FIRST);
			ctx->pending_rq = NULL;
		}
	}
	else
	{
		mwmr_write(ctx->in_channel, &ch, 1);
		if(ch == '\n') ctx->eol = 1;
	}

#if CONFIG_TTY_ECHO_MODE
	switch(ch)
	{
	case '\b':
	case 0x7F:
		tty_putc(tty, '\b');
	tty_putc(tty, ' ');
	tty_putc(tty, '\b');
	break;
	default:
		tty_putc(tty, ch);
	}
#endif

	cpu_spinlock_unlock(&tty->lock.val);
}

void ibmpc_tty_init(struct device_s *tty, void *base, uint_t irq, uint_t tty_id)
{
	struct tty_context_s *ctx;
  
	isRightShift = 0;
	isLeftShift = 0;
	isCtrl = 0;
	isAlt = 0;
	isCAPSLOCK = 0;

	spinlock_init(&tty->lock);
	tty->base = base;
	tty->irq = irq;
	tty->type = DEV_CHR;

	tty->action.dev = tty;
	tty->action.irq_handler = &tty_irq_handler;
	tty->action.data = NULL;
 
	tty->op.dev.open = NULL;
	tty->op.dev.read = &tty_read;
	tty->op.dev.write = &tty_write;
	tty->op.dev.close = NULL;
	tty->op.dev.lseek = NULL;
	tty->op.dev.set_params = NULL;
	tty->op.dev.get_params = &tty_get_params;

	ctx = kbootMem_calloc(sizeof(*ctx));
	ctx->id = tty_id;
	ctx->in_channel = mwmr_init(1,TTY_BUFFER_DEPTH, 0);
	rwlock_init(&ctx->rwlock);
	ctx->eol = 0;
	ctx->attr = 0x07;
	tty->data = (void*) ctx;
	sprintk(tty->name, "TTY%d", tty_id);
	metafs_init(&tty->node, tty->name);
	wait_queue_init(&ctx->wait_queue,tty->name);
	tty_clear(tty);
}


uint8_t keyboard_map[] = { 0x1B, 0x1B, 0x1B, 0x1B,
			   '&', '1', '1', '1',
			   '~', '2', '2', '2',
			   '"', '3', '3', '3',
			   '\'','4', '{', '{',
			   '(', '5', '5', '5',
			   '-', '6', '|', '|',
			   '`', '7', '7', '7',
			   '_', '8', '\\', '\\',
			   '^', '9', '9', '9',
			   '@', '0', '0', '0',
			   ')', ']', '-', '-',
			   '=', '+', '}','}',
			   '\b','\b', 0x7F,'\b',
			   '\t','\t','\t','\t',
			   'a', 'A', 'a', 'a',
			   'z', 'Z', 'z', 'z',
			   'e', 'E', 'e', 'e',
			   'r', 'R', 'r', 'r',
			   't', 'T', 't', 't',
			   'y', 'Y', 'y', 'y',
			   'u', 'U', 'u', 'u',
			   'i', 'I', 'i', 'i',
			   'o', 'O', 'o', 'o',
			   'p', 'P', 'p', 'p',
			   '[', '{', '[', '[',
			   '$', '$', '$', ']',
			   '\n','\n','\n','\n',
			   0xFF, 0xFF, 0xFF, 0xFF,
			   'q', 'A', 'a', 'a',
			   's', 'S', 's', 's',
			   'd', 'D', 'd', 'd',
			   'f', 'F', 'f', 'f',
			   'g', 'G', 'g', 'g',
			   'h', 'H', 'h', 'h',
			   'j', 'J', 'j', 'j',
			   'k', 'K', 'k', 'k',
			   'l', 'L', 'l', 'l',
			   'm', 'M', ';', ';',
			   '%', '%', '%', '%',
			   0x28, 0x28, 0x28, 0x28,
			   0x29, 0x29, 0x29, 0x29, /* Left Shift (41) */
			   '*', '*', '*', '*',
			   'w', 'W', 'w', 'w',
			   'x', 'X', 'x', 'x',
			   'c', 'C', 'c', 'c',
			   'v', 'V', 'v', 'v',
			   'b', 'B', 'b', 'b',
			   'n', 'N', 'n', 'n',
			   ',', '?', ',', ',',
			   ';', '.', ';', ';',
			   ':', '/', ':', ':',
			   '!', '!', '!', '!',
			   0x35, 0x35, 0x35, 0x35,	/*      Rshift  (0x35)  */
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   ' ', ' ', ' ', ' ',	        /*      space   */
			   0x39, 0x39, 0x39, 0x39,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF, /* 0x40 */	
			   0xFF, 0xFF, 0xFF, 0xFF, 
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF, /* 0x50 */
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   '<', '>', '<', '<',
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF,	
			   0xFF, 0xFF, 0xFF, 0xFF, /* 0x60 */
			   0xFF, 0xFF, 0xFF, 0xFF};
