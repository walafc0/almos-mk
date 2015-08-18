/*
 * kern/syscall.h - kernel service numbers asked by userland
 * 
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Ghassan Almaless
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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

enum
{
	SYS_EXIT,                      /* SYS_EXIT SERIVCE NUMBER IS FROZEN */
	SYS_MMAP,
	SYS_CREATE,
	SYS_JOIN,
	SYS_DETACH,
	SYS_YIELD,
	SYS_SEMAPHORE,
	SYS_COND_VAR,
	SYS_BARRIER,                  
	SYS_RWLOCK,
	SYS_SLEEP,                    /* Service NR 10 */
	SYS_WAKEUP,
	SYS_OPEN,                    
	SYS_CREAT,
	SYS_READ,
	SYS_WRITE,
	SYS_LSEEK,
	SYS_CLOSE,
	SYS_UNLINK,                   
	SYS_PIPE,
	SYS_CHDIR,                    /* Service NR 20 */
	SYS_MKDIR,
	SYS_MKFIFO,                   
	SYS_OPENDIR,
	SYS_READDIR,
	SYS_CLOSEDIR,
	SYS_GETCWD,
	SYS_CLOCK,
	SYS_ALARM,                    
	SYS_DMA_MEMCPY,
	SYS_UTLS,                     /* Service NR 30 */  
	SYS_SIGRETURN,
	SYS_SIGNAL,                
	SYS_SET_SIGRETURN,
	SYS_KILL,
	SYS_GETPID,
	SYS_FORK,
	SYS_EXEC,
	SYS_GETATTR,                 
	SYS_PS,
	SYS_MADVISE,			/* Service NR 40 */   
	SYS_PAGEINFO,
	SYS_STAT,
	SYS_MIGRATE,
	SYS_SBRK,
	SYS_RMDIR,
	SYS_FTIME,
	SYS_CHMOD,
	SYS_FSYNC,
	__SYS_CALL_SERVICES_NUM,
};

#endif
