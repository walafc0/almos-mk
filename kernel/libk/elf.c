/*
 * elf.c - elf parser, find and map process sections
 *
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
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
#include <errno.h>
#include <thread.h>
#include <task.h>
#include <ppm.h>
#include <pmm.h>
#include <thread.h>
#include <cpu.h>
#include <kmem.h>
#include <vfs.h>
#include <vfs-params.h>
#include <ku_transfert.h>
#include <vmm.h>
#include <elf.h>
#include <libk.h>

#define BUFFER_SIZE     4096


static error_t elf_isElfHeader(Elf32_Ehdr *e_header)
{
	if((e_header->e_ident[EI_MAG0] == ELFMAG0) 
	   && (e_header->e_ident[EI_MAG1] == ELFMAG1) 
	   && (e_header->e_ident[EI_MAG2] == ELFMAG2)
	   && (e_header->e_ident[EI_MAG3] == ELFMAG3))
		return 1;
	else
		return 0;
}

static error_t elf_isValidHeader(Elf32_Ehdr *e_header)
{
	if((e_header->e_ident[EI_CLASS] == ELFCLASS32) 
	   && (e_header->e_ident[EI_DATA] == ELFDATA2LSB) 
	   && (e_header->e_ident[EI_VERSION] == EV_CURRENT)
	   && (e_header->e_ident[EI_OSABI] == ELFOSABI_NONE)
	   && ((e_header->e_machine == EM_MIPS) || 
	       (e_header->e_machine == EM_MIPS_RS3_LE) ||
	       (e_header->e_machine == EM_386))
	   && (e_header->e_type == ET_EXEC))
		return 1;
   
	assert((e_header->e_ident[EI_CLASS] == ELFCLASS32) && "Elf is not 32-Binary");
	assert((e_header->e_ident[EI_DATA] == ELFDATA2LSB) && "Elf is not 2's complement, little endian");
	assert((e_header->e_ident[EI_VERSION] == EV_CURRENT) && "Elf is not in Current Version");
	assert((e_header->e_ident[EI_OSABI] == ELFOSABI_NONE) && "Unexpected Elf ABI, I need UNIX System V ABI");
	assert(((e_header->e_machine == EM_MIPS) || 
		(e_header->e_machine == EM_MIPS_RS3_LE) || (e_header->e_machine == EM_386)) 
	       && "Unexpected machine type, I accept MIPS/80386 machines in this version of AlmOS");
	assert((e_header->e_type == ET_EXEC) && 
	       "Elf is not executable binaray, I accept just executable files in this AlmOS version");

	return 0;
}


static error_t elf_header_read(char* pathname, struct vfs_file_s *file, Elf32_Ehdr **header)
{ 
	kmem_req_t req;
	Elf32_Ehdr *e_header;
	uint_t refcount;
	register error_t err;
	register size_t eh_size;
	register ssize_t count;
	struct ku_obj ku_path;

	KK_BUFF(ku_path, pathname);
	if((err = vfs_open(&current_task->vfs_cwd, &ku_path, VFS_O_RDONLY, 0, file)))
	{
		printk(ERROR, "\nERROR: elf_header_read: faild to open executable file, error %d\n", err);
		return err;
	}
  
	eh_size = sizeof(*e_header);

	req.type  = KMEM_GENERIC;
	req.size  = eh_size;
	req.flags = AF_USER;

	if((e_header = kmem_alloc(&req)) == NULL)
	{
		vfs_close(file, &refcount);
		return -ENOMEM;
	}

	KK_SZ_BUFF(ku_path, e_header, eh_size);
	if((count= vfs_read(file, &ku_path)) != eh_size)
	{
		printk(ERROR, "\nERROR: elf_header_read: faild to read ELF header, got %d bytes, expected %d bytes\n", count, eh_size);
		err = (error_t)count;
		goto ELF_HEADER_READ_ERR;
	}

	if(!(err= elf_isElfHeader(e_header)))
	{
		printk(ERROR, "\nERROR: elf_header_read: executable is not in ELF format\n");
		goto ELF_HEADER_READ_ERR;
	}

	if(!(err= elf_isValidHeader(e_header)))
	{
		printk(ERROR, "\nERROR: elf_header_read: not supported Elf\n");
		goto ELF_HEADER_READ_ERR;
	}
 
	*header = e_header;
	return 0;
 
ELF_HEADER_READ_ERR:
	vfs_close(file, &refcount);
	req.ptr = e_header;
	kmem_free(&req);
	return -1;
}

static error_t elf_program_entries_read(struct vfs_file_s *file, Elf32_Ehdr *e_header, Elf32_Phdr **p_entries)
{
	register error_t err;
	register size_t size;
	register ssize_t count;
	struct ku_obj ku_buff;
	uint8_t *buff;
	kmem_req_t req;
  
	size = e_header->e_phoff * e_header->e_phnum;
  
	if(size == 0)
	{
		printk(ERROR, "\nERROR: elf_program_entries_read: no program entries found\n");
		return EINVAL;
	}

	req.type  = KMEM_GENERIC;
	req.size  = size;
	req.flags = AF_USER;

	if((buff = kmem_alloc(&req)) == NULL)
		return ENOMEM;

	if((err=vfs_lseek(file, e_header->e_phoff, VFS_SEEK_SET, NULL)))
	{
		printk(ERROR, "\nERROR: elf_program_entries_read: faild to localise entries\n");
		goto ELF_PROG_ENTRIES_READ_ERR;
	}

	KK_SZ_BUFF(ku_buff, buff, size);
	if((count=vfs_read(file, &ku_buff)) != size)
	{
		printk(ERROR, "\nERROR: elf_program_entries_read: faild to read program entries, got %d bytes, expected %d bytes\n", 
		       count, size);
		err = (error_t) count;
		goto ELF_PROG_ENTRIES_READ_ERR;
	}
  
	*p_entries = (Elf32_Phdr*) buff;
	return 0;

ELF_PROG_ENTRIES_READ_ERR:
	req.ptr = buff;
	kmem_free(&req);
	return err;
}


//FIXME: can we optimize by coalescing remote access
static error_t elf_segments_load(struct vfs_file_s *file,
				 Elf32_Ehdr *e_header, 
				 Elf32_Phdr *p_entry,
				 struct task_s *task)
				
{
	register error_t err;
	register uint_t index;
	register size_t size;
	register uint_t start;
	register uint_t limit;
	uint_t proto;
	uint_t flags;

	for(index = 0; index < e_header->e_phnum; index++, p_entry++) 
	{   
		if(p_entry->p_type != PT_LOAD)
			continue;
    
#if 1
		if(NOT_IN_USPACE(p_entry->p_vaddr))
		{
			err = EPERM;
			printk(ERROR, "\nERROR: %s: p_vaddr %x, index %d [ EPERM ]\n",
			       __FUNCTION__, 
			       p_entry->p_vaddr, 
			       index);

			return err;
		}
#endif
    
		if((err=vfs_lseek(file, p_entry->p_offset, VFS_SEEK_SET, NULL)))
		{
			printk(ERROR, "\nERROR: %s: faild to localise segment @index %d\n", 
			       __FUNCTION__, 
			       index);

			return err;
		}

		size  = 0;
		start = p_entry->p_vaddr;
		limit = p_entry->p_vaddr + p_entry->p_memsz;
    
		if((start & PMM_PAGE_MASK) || (limit & PMM_PAGE_MASK))
		{
			printk(WARNING,"WARNING: %s: segment not aligned <0x%x - 0x%x>\n",
			       __FUNCTION__, 
			       start, 
			       limit);

			return EACCES;
		}

		if(task->vmm.text_start == 0)
		{
			proto                = VM_REG_RD | VM_REG_EX;
			flags                = VM_REG_SHARED | VM_REG_FIXED | VM_REG_INST;
			task->vmm.text_start = start;
			task->vmm.text_end   = limit;

			elf_dmsg(1, "%s: Text <0x%x - 0x%x>\n",
			       __FUNCTION__, 
			       start, 
			       limit);
		}
		else
		{
			if(task->vmm.data_start != 0)
				continue;
      
			proto                  = VM_REG_RD | VM_REG_WR;
			flags                  = VM_REG_PRIVATE | VM_REG_FIXED;
			task->vmm.data_start   = start;
			task->vmm.data_end     = limit;
			task->vmm.heap_start   = limit;
			task->vmm.heap_current = limit;

			elf_dmsg(1, "%s: Data <0x%x - 0x%x>\n",
			       __FUNCTION__, 
			       start, 
			       limit);
		}

		err = (error_t) vmm_mmap(task, 
					 file, 
					 (void*)start, 
					 limit - start, 
					 proto, 
					 flags,
					 p_entry->p_offset);

		if(err == (error_t)VM_FAILED)
		{
			printk(WARNING,"WARNING: %s: Faild to map segment <0x%x - 0x%x>, proto %x, file inode %x\n",
			       __FUNCTION__, 
			       start, 
			       limit, 
			       proto, 
			       file->f_inode.ptr);

			return current_thread->info.errno;
		}

		vfs_file_up(file);
	}

	return 0;
}

error_t elf_load_task(char *pathname, struct task_s *task)
{
	Elf32_Ehdr *e_header;
	Elf32_Phdr *p_entry;
	struct vfs_file_s file;
	register error_t err;
	kmem_req_t req;
	uint_t count;

	req.type = KMEM_GENERIC;
	e_header = NULL;
	p_entry  = NULL;
	file = (const struct vfs_file_s ){0};

	task->vmm.text_start = 0;
	task->vmm.data_start = 0;

	elf_dmsg(1, "%s: going to read elf header [%s]\n", 
		 __FUNCTION__, 
		 pathname);

	if((err=elf_header_read(pathname, &file, &e_header)))
		return err;

	elf_dmsg(1, "%s: going to read elf header entries [%s]\n", 
		 __FUNCTION__, 
		 pathname);

	if((err=elf_program_entries_read(&file, e_header, &p_entry)))
		goto ELF_LOAD_TASK_ERR1;

	elf_dmsg(1, "%s: going to read elf segments [%s]\n", 
		 __FUNCTION__, 
		 pathname);

	if((err=elf_segments_load(&file, e_header, p_entry, task)))
		goto ELF_LOAD_TASK_ERR2;

	elf_dmsg(1, "%s: elf entry point @%x [%s]\n", 
		 __FUNCTION__, 
		 (uint_t) e_header->e_entry, 
		 pathname);

	task->vmm.entry_point = (uint_t)e_header->e_entry;
	err = 0;
  
ELF_LOAD_TASK_ERR2:
	req.ptr = p_entry;
	kmem_free(&req);

ELF_LOAD_TASK_ERR1:

	if(err)
		vfs_close(&file, &count);
	else
	{
		task->bin = file;
		elf_dmsg(1, "%s: task 0x%x, bin %x\n", 
			 __FUNCTION__, 
			 task, 
			 task->bin);
	}

	req.ptr = e_header;
	kmem_free(&req);

	elf_dmsg(1, "INFO: %s: done, err %d\n", 
		 __FUNCTION__, 
		 err);

	return err;
}
