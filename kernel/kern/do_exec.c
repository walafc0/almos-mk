/*
 * kern/do_exec.c - excecutes a new user process, load init.
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

#include <stdarg.h>
#include <config.h>
#include <errno.h>
#include <types.h>
#include <libk.h>
#include <bits.h>
#include <kmem.h>
#include <task.h>
#include <cpu.h>
#include <pmm.h>
#include <page.h>
#include <task.h>
#include <thread.h>
#include <pid.h>
#include <list.h>
#include <vfs.h>
#include <sys-vfs.h>
#include <scheduler.h>
#include <spinlock.h>
#include <cluster.h>
#include <ku_transfert.h>
#include <dqdt.h>

#define USR_LIMIT  (CONFIG_USR_LIMIT)

#define DEV_STDIN   CONFIG_DEV_STDIN
#define DEV_STDOUT  CONFIG_DEV_STDOUT
#define DEV_STDERR  CONFIG_DEV_STDERR

#define TASK_DEFAULT_HEAP_SIZE  CONFIG_TASK_HEAP_MIN_SIZE

#define INIT_PATH   "/bin/init"

/* FIXME : ecopy et estrlen attendent une adresse de vecteur dans l'espace utilisateur. Or pour
 * l'instant l'environnement des processus est "forgé" dans do_exec(), c'est donc des buffers
 * noyaux. Solution temporaire : on utilise kexec_copy et kexec_strlen.
 */
error_t args_len(char **vect, uint_t pages_max, 
		uint_t *pages_nr, uint_t *entries_nr,
		exec_copy_t ecopy, exec_strlen_t estrlen)
{
	register uint_t cntr;
	register uint_t pgnr;
	error_t err;
	uint_t count;
	uint_t len;
        char* ptr;
  
	cntr  = 0;
	count = 0;
	pgnr  = 0;

	while(1)
	{
		if((err = ecopy(&ptr, &vect[cntr], sizeof(ptr))))
		{
			printk(INFO, "INFO: %s:%s: EFAULT Has been Catched on vect, cntr %d, strlen &vect[%x] = %x\n",  \
                                        __FUNCTION__, __LINE__, cntr, &vect[cntr], vect[cntr]);
			return err;
		}

		if(ptr == NULL)
                        break;

		if((err=estrlen(ptr, &len)))
		{
                        printk(INFO, "INFO: %s:%s: EFAULT Has been Catched, cntr %d, strlen &vect[%x] = %x\n",          \
                                        __FUNCTION__, __LINE__, cntr, &vect[cntr], vect[cntr]);
			return err;
		}
		cntr  ++;
		len   ++;
		count += (ARROUND_UP(len, 8));
		pgnr   = ARROUND_UP(count, PMM_PAGE_SIZE);
    
		if((pgnr >> PMM_PAGE_SHIFT) >= pages_max)
			return E2BIG;
	}

	count       = (cntr + 1) * sizeof(char*);
	*pages_nr   = (ARROUND_UP(count, PMM_PAGE_SIZE)) / PMM_PAGE_SIZE;
	*entries_nr = cntr;
	return 0;
}

//for now the pointers can be either be from uspace
//or kernel space. Solutions :
//first keep this binaroty and add a flag to
//choose how to copy the arguments
//second make them all user space ptr
//third make them kernel space ptr
//the first solution (although ugly), is the most
//perfroming(compared to u->k) and simple 
//(compared to k->u)
error_t compute_args(struct task_s *task, 
		     char **vect,
		     struct page_s **pgtbl,
		     uint_t pages_max, 
		     uint_t start,
		     uint_t *current, 
		     uint_t *pgnr,
		     uint_t *pgindex,
		     exec_copy_t ecopy, 
		     exec_strlen_t estrlen)
{
	kmem_req_t req;
	uint_t count;
	uint_t cntr;
	uint_t pages_nr;
	char **args;
	char *ptr;
	uint_t i;
	uint_t len;
	uint_t index;
	uint_t paddr;
	error_t err;

	req.type  = KMEM_PAGE;
	req.size  = 0;
	req.flags = AF_USER | AF_ZERO | AF_REMOTE;
	req.ptr   = task->cluster;

	pages_nr  = 0;
	count     = 0;
	cntr      = 0;
	index     = *pgindex;

	err = args_len(vect, pages_max, &pages_nr, &cntr, ecopy, estrlen);
  
	if(err) return err;

	*pgnr += pages_nr;

	for(i = index; i < (pages_nr + index); i++)
	{
		pgtbl[i] = kmem_alloc(&req);
    
		if(pgtbl[i] == NULL) return ENOMEM;
	}
  
	*current = start - (pages_nr * PMM_PAGE_SIZE);
 
	paddr = (uint_t) ppm_page2addr(pgtbl[index]);
	count = (cntr + 1) * sizeof(char*);
	args = (char **) paddr;
	pages_nr = 0;

	while(count > PMM_PAGE_SIZE)
	{
		err = ecopy((void*)paddr, vect + (pages_nr << PMM_PAGE_SHIFT), PMM_PAGE_SIZE);

		if(err) return err;
   
		index ++;
		paddr  = (uint_t) ppm_page2addr(pgtbl[index]);
		count -= PMM_PAGE_SIZE;
		pages_nr ++;
	}

	if(count > 0)
	{
		err = ecopy((void*)paddr, vect + (pages_nr << PMM_PAGE_SHIFT), count);
		if(err) return err;
	}

	paddr += count;
 
	for(i=0; i < cntr; i++)
	{
		if((err = ecopy(&ptr, &vect[i], sizeof(ptr))))
		{
			printk(INFO, "INFO: %s: EFAULT Has been Catched on vect\n", __FUNCTION__);
			return err;
		}

		err = estrlen(ptr, &len);
		if(err) return err;
		len++;


		args[i] = (char*)(*current + (pages_nr << PMM_PAGE_SHIFT) + count);
    
		while(len)
		{
			if((PMM_PAGE_SIZE - count) > (ARROUND_UP(len, 8)))
			{
				err = ecopy((void*)paddr, ptr, len);
				if(err) return err;
				paddr += (ARROUND_UP(len, 8));
				count += (ARROUND_UP(len, 8));
				len = 0;
			}
			else
			{
				err = ecopy((void*)paddr, ptr, PMM_PAGE_SIZE - count);
				if(err) return err;
				ptr += (PMM_PAGE_SIZE - count);
				len -= (PMM_PAGE_SIZE - count);
				index ++;
				paddr = (uint_t) ppm_page2addr(pgtbl[index]);
				pages_nr ++;
				count = 0;
			}
		}
	}

	*pgindex = index + 1;  
	return 0;
}


error_t map_args(struct task_s *task,
		 struct page_s **pgtbl, 
		 uint_t start_index, 
		 uint_t end_index,
		 uint_t start_addr)
{
	struct pmm_s *pmm;
	pmm_page_info_t info;
	uint_t current_vma;
	error_t err;
	uint_t i;

	pmm = &task->vmm.pmm;
	info.attr = PMM_PRESENT | PMM_READ | PMM_WRITE | PMM_CACHED | PMM_USER; 
	info.cluster = task->cluster;
	current_vma = start_addr;

	for(i = start_index; i < end_index; i++)
	{    
		info.ppn = ppm_page2ppn(pgtbl[i]);
    
		if((err = pmm_set_page(pmm, current_vma, &info)))
			return err;

		pgtbl[i] = NULL;
		current_vma += PMM_PAGE_SIZE;
	}
  
	return 0;
}

error_t do_exec(struct task_s *task, 
		char *path_name, 
		char **argv, 
		char **envp,
		uint_t *isFatal,
		struct thread_s **new,
		exec_copy_t ecopy, 
		exec_strlen_t estrlen)
{
	error_t err;
	pthread_attr_t attr;
	uint_t usr_stack_top;
	uint_t pages_nr;
	uint_t pages_max;
	uint_t index;
	uint_t reg1_index;
	sint_t order;
	struct thread_s *main_thread;
	struct page_s *pgtbl[CONFIG_TASK_ARGS_PAGES_MAX_NR];

	usr_stack_top = USR_LIMIT;
	pages_nr = 0;
	pages_max = CONFIG_TASK_ARGS_PAGES_MAX_NR;
	index = 0;
	reg1_index = 0;
	*isFatal = 1;
	memset(&pgtbl[0], 0, sizeof(pgtbl));

	err = compute_args(task, 
			   envp, 
			   &pgtbl[0], 
			   pages_max, 
			   USR_LIMIT, 
			   &usr_stack_top, 
			   &pages_nr, 
			   &index,
			   ecopy,
			   estrlen);

	reg1_index = index;

	if(err)
                goto DO_EXEC_ENOTFATAL;

	attr.arg2 = (void*)usr_stack_top;	/* environ */

	err = compute_args(task, 
			   argv, 
			   &pgtbl[0], 
			   pages_max - pages_nr, 
			   usr_stack_top, 
			   &usr_stack_top, 
			   &pages_nr, 
			   &index,
			   ecopy,
			   estrlen);

	attr.arg1 = (void*)usr_stack_top;	/* argv */

	if(err)
                goto DO_EXEC_ENOTFATAL;

        /* FIXME: zero should not be hard-coded. Use something like MAIN_KERNEL which represents the
         * kernel with the init and sh processes (assuming that both of them are on the same
         * kernel).
         */
	if( ((task->pid != PID_MIN_GLOBAL+1) && (current_cid == 0))     ||              \
                        ( (task->pid != PID_MIN_GLOBAL) && (current_cid != 0) ) )
	{
		vmm_destroy(&task->vmm);
		pmm_release(&task->vmm.pmm);    
		err = vmm_init(&task->vmm);
		if(err)
                        goto DO_EXEC_ERR;
	}
 
	attr.stack_addr = (void*)(usr_stack_top - CONFIG_PTHREAD_STACK_SIZE);
	attr.stack_size = CONFIG_PTHREAD_STACK_SIZE;

	err = (error_t) vmm_mmap(task, NULL, 
				 attr.stack_addr, 
				 USR_LIMIT - (uint_t)attr.stack_addr, 
				 VM_REG_RD | VM_REG_WR,
				 VM_REG_PRIVATE | VM_REG_ANON | VM_REG_STACK | VM_REG_FIXED, 0);
  
	if(err == -1)
	{
		err = current_thread->info.errno;
		printk(INFO, "INFO: %s: failed to mmap main's stack, [pid %d, err %d]\n",       \
                                __FUNCTION__, task->pid, err);
		goto DO_EXEC_ERR;
	}

	err = map_args(task, pgtbl, 0, reg1_index, (uint_t)attr.arg2);

	if(err)
                goto DO_EXEC_ERR;

	err = map_args(task, pgtbl, reg1_index, index, (uint_t)attr.arg1);

	if(err)
                goto DO_EXEC_ERR;

	if(( err = elf_load_task(path_name, task) ))
	{
		printk(INFO, "INFO: %s: kernel has encountered an error while loading task [pid %d, err %d]\n",      \
                                __FUNCTION__, task->pid, err);

		goto DO_EXEC_ERR;
	}

	err = (error_t) vmm_mmap(task, NULL, 
				 (void*)task->vmm.heap_start, 
				 TASK_DEFAULT_HEAP_SIZE, VM_REG_RD | VM_REG_WR,
				 VM_REG_PRIVATE | VM_REG_ANON | VM_REG_HEAP | VM_REG_FIXED, 0);

	if(err == -1)
	{
		err = current_thread->info.errno;
		printk(INFO, "INFO: %s: failed to mmap main's heap, [pid %d, err %d]\n", 
		       __FUNCTION__, task->pid, err);
		goto DO_EXEC_ERR;
	}

	task->vmm.heap_current += TASK_DEFAULT_HEAP_SIZE;

	attr.flags = (current_thread->info.attr.flags | PT_ATTR_DETACH);
	attr.sched_policy = SCHED_RR;
	attr.cid = task->cluster->id;
	attr.cpu_lid = task->cpu->lid;
	attr.cpu_gid = task->cpu->gid;
	attr.entry_func = (void*)task->vmm.entry_point;
	attr.exit_func = NULL;
	attr.stack_size = attr.stack_size - SIG_DEFAULT_STACK_SIZE;
	attr.sigstack_addr = (void*)((uint_t)attr.stack_addr + attr.stack_size);
	attr.sigstack_size = SIG_DEFAULT_STACK_SIZE;
	attr.sigreturn_func = 0;

	if((err = thread_create(task, &attr, &main_thread)))
	{
		printk(INFO,
		       "INFO: %s: kernel has encountered an error while "
		       "creating main thread of task [pid %d, err %d]\n",
		       __FUNCTION__,
		       task->pid,
		       err);

		goto DO_EXEC_ERR;
	}
  
	// Add the new thread to the set of created threads
	// Order here is significan for profiling purposes
	list_add_last(&task->th_root, &main_thread->rope);
	task->threads_count = 1;
	task->threads_nr ++;
	order = bitmap_ffs2(task->bitmap, 0, sizeof(task->bitmap));

	if(order == -1) goto DO_EXEC_ERR;

	bitmap_clear(task->bitmap, order);
	main_thread->info.attr.key = order;
        main_thread->info.order = order;
	task->next_order = order + 1;
	task->max_order = order;
	task->th_tbl[order] = main_thread;
	*new = main_thread;
	return 0;

DO_EXEC_ENOTFATAL:
	*isFatal = 0;

DO_EXEC_ERR:
	for(index = 0; index < pages_max; index ++)
	{
		if(pgtbl[index] != NULL)
			ppm_free_pages(pgtbl[index]);
	}

	return err;
}

error_t kexec_copy(void *dst, void *src, uint_t count)
{
	/* printk(INFO, "%s: copying from %p to %p with size %x\n", __FUNCTION__, dst, src, count); */
	memcpy(dst, src, count);
	return 0;
}

error_t kexec_strlen(char *dst, uint_t *count)
{
	*count = strlen(dst);
	/* printk(INFO, "%s: size of %p is %x\n", __FUNCTION__, dst, *count); */
	return 0;
}

error_t task_load_init(struct task_s *task)
{
	struct task_s *init;
	struct dqdt_attr_s attr;
	struct thread_s *main_thread;
	struct vfs_file_s stdin;
	struct vfs_file_s stdout;
	struct vfs_file_s stderr;
	struct ku_obj ku_path;
	error_t err;
	error_t err1;
	error_t err2;
	uint_t isFatal;
	char *environ[] = {"ALMOS_VERSION="CONFIG_ALMOS_VERSION, NULL};
	char *argv[]    = {INIT_PATH, "init", NULL};

	printk(INFO, "INFO: Loading Init Process [ %s ]\n", INIT_PATH);

	err = dqdt_task_placement(dqdt_root, &attr);
        /* Force init to be on current cluster */
        attr.cid_exec = current_cid;

	assert(err == 0);

	if((err = task_create(&init, &attr, CPU_USR_MODE)))
		return err;

	err = vmm_init(&init->vmm);

	if(err) goto INIT_ERR;

	err = pmm_init(&init->vmm.pmm, current_cluster);

	if(err) goto INIT_ERR;
  
	err = pmm_dup(&init->vmm.pmm, &task->vmm.pmm);
  
	if(err) goto INIT_ERR;
  
	vfs_file_up(&task->vfs_root);
	init->vfs_root = task->vfs_root;

	vfs_file_up(&task->vfs_root);
	init->vfs_cwd  = task->vfs_root;

	init->vmm.limit_addr = CONFIG_USR_LIMIT;
	init->uid = 0;
	init->parent = task->pid;
	atomic_init(&init->childs_nr, 0);
	atomic_init(&task->childs_nr, 1);
  
	list_add_last(&task->children, &init->list);

	KK_BUFF(ku_path, ((void*) DEV_STDIN));
	err  = vfs_open(&init->vfs_cwd, &ku_path,  VFS_O_RDONLY, 0, &stdin);
	KK_BUFF(ku_path, ((void*) DEV_STDOUT));
	err1 = vfs_open(&init->vfs_cwd, &ku_path, VFS_O_WRONLY, 0, &stdout);
	KK_BUFF(ku_path, ((void*) DEV_STDERR));
	err2 = vfs_open(&init->vfs_cwd, &ku_path, VFS_O_WRONLY, 0, &stderr);

	if(err || err1 || err2)
	{
		if(!err)  vfs_close(&stdin,  NULL);
		if(!err1) vfs_close(&stdout, NULL);
		if(!err2) vfs_close(&stderr, NULL);
    
		printk(ERROR,"ERROR: do_exec: cannot open fds [%d, %d, %d]\n", err, err1, err2);
		err = ENOMEM;
    
		goto INIT_ERR;
	}

	task_fd_set(init, 0, &stdin);
	task_fd_set(init, 1, &stdout);
	task_fd_set(init, 2, &stderr);

	current_thread->info.attr.flags = PT_ATTR_AUTO_NXTT | PT_ATTR_MEM_PRIO | PT_ATTR_AUTO_MGRT;

	err = do_exec(init, INIT_PATH, &argv[0], &environ[0], &isFatal, 
				&main_thread, kexec_copy, kexec_strlen);
  
	if(err == 0)
	{
		assert(main_thread != NULL && (main_thread->signature == THREAD_ID));
		init->state = TASK_READY;
		err = sched_register(main_thread);
		assert(err == 0); 		/* FIXME: ask DQDT for another core */

#if CONFIG_ENABLE_TASK_TRACE
		main_thread->info.isTraced = true;
#endif
		sched_add_created(main_thread);

                printk(INFO, "INFO: Init Process Loaded [ %s ]\n", INIT_PATH);

		return 0;
	}

INIT_ERR:
	task_destroy(init);
	return err;
}
