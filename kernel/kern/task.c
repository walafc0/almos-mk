/*
 * kern/task.c - task related management
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

#include <types.h>
#include <errno.h>
#include <bits.h>
#include <kmem.h>
#include <page.h>
#include <vmm.h>
#include <kdmsg.h>
#include <vfs.h>
#include <cpu.h>
#include <thread.h>
#include <list.h>
#include <scheduler.h>
#include <spinlock.h>
#include <dqdt.h>
#include <cluster.h>
#include <pmm.h>
#include <boot-info.h>
#include <pid.h>
#include <htable.h>
#include <utils.h>
#include <task.h>

error_t task_fd_init(struct task_s *task)
{
        uint_t i;

        spinlock_init(&task->fd_info.lock, "Task FDs");
        task->fd_info.extended  = false;
        task->fd_info.max       = CONFIG_TASK_FILE_MAX_NR;
        task->fd_info.fd_entry  = task->fd_info.fd_entry_base;

        for ( i = 0; i < task->fd_info.max; i++ )
        {
                task->fd_info.fd_entry[i].busy   = 0;
        }

	return 0;
}

/* Close all opened files and reset all */
error_t task_fd_destroy(struct task_s *task)
{
        struct vfs_file_s *file;
        error_t err;
        uint_t i;

        err = 0;

        for(i = 0; i < task->fd_info.max; i++)
        {
                if ( (err = task_fd_lookup(task, i, &file)) )
                        return err;

                if ( file == NULL )
                        continue;

                if ( task->fd_info.fd_entry[i].busy )
                        vfs_close(file, NULL);
        }

        /* Free memory */
        /* TODO : use memory manager to free the extended table */
        assert(!task->fd_info.extended);

        /* Delete reference to the file descriptors table */
	task->fd_info.fd_entry = NULL;

        return 0;
}

error_t task_fd_set(struct task_s *task, uint_t fd, struct vfs_file_s *file)
{
        task->fd_info.fd_entry[fd].busy  = 1;
        task->fd_info.fd_entry[fd].file  = *file;
	return 0;
}

/* Get the next file descriptor number and reserve a field in the table */
error_t task_fd_get(struct task_s *task, uint_t *ret_fd, struct vfs_file_s **file)
{
        uint_t  fd;
        bool_t  found;
        error_t err;

        err     = 0;
        found   = false;

	spinlock_lock(&task->fd_info.lock);

        /* Looking for an empty field */
        for ( fd = 0; fd < task->fd_info.max; fd++ )
        {
                /* If found and not busy */
                if ( task->fd_info.fd_entry[fd].busy == 0 )
                {
                        found = true;
                        task->fd_info.fd_entry[fd].busy  = 1;
			*file = &task->fd_info.fd_entry[fd].file;
			*ret_fd = fd;
                        break;
                }
        }

	spinlock_unlock(&task->fd_info.lock);

        if ( !found )
                err = EMFILE;   /* Too many opened file */

        return err;

}

error_t task_fd_put(struct task_s *task, uint_t fd)
{
        if ( (fd < 0) || (fd>task->fd_info.max) )
                return EBADF;

        spinlock_lock(&task->fd_info.lock);
        task->fd_info.fd_entry[fd].busy        = 0;
        spinlock_unlock(&task->fd_info.lock);

        return 0;
}


error_t task_fd_lookup(struct task_s *task, uint_t fd, struct vfs_file_s **file)
{
        if ( (fd < 0) || (fd >= task->fd_info.max) )
                return EBADFD;

        spinlock_lock(&task->fd_info.lock);
	*file = &task->fd_info.fd_entry[fd].file;
        spinlock_unlock(&task->fd_info.lock);

        return 0;
}

error_t task_fd_fork(struct task_s *dst, struct task_s *src)
{
        error_t err;

        err = 0;

        assert( !src->fd_info.extended );/* Not implemented for now. Coming soon. */

	spinlock_lock(&src->fd_info.lock);
        err = __task_fd_fork(&dst->fd_info, &src->fd_info);
	spinlock_unlock(&src->fd_info.lock);

        return err;
}

/* Do the fork without lock */
error_t __task_fd_fork(struct fd_info_s *dst, struct fd_info_s *src)
{
        uint_t fd;
        struct fd_entry_s *entry;
        struct vfs_file_s *file;

        for(fd = 0; fd < src->max; fd++)
	{
		entry = &src->fd_entry[fd];

		if( entry->busy )
		{
			file = &entry->file;
                        if ( file == NULL )
                                return 1;
                        vfs_file_up(file);
			/* copy entry struct in dest */
			dst->fd_entry[fd] = *entry;
		}
	}

        return 0;
}

/* Used only when restoring task from ashes in case of remote exec */
static error_t task_dup_clone(struct task_s *dst, struct vfs_file_s *vfs_root,          \
                        struct vfs_file_s *vfs_cwd, struct vfs_file_s *bin)
{
	rwlock_wrlock(&dst->cwd_lock);
        dst->vfs_root = *vfs_root;
        dst->vfs_cwd  = *vfs_cwd;
        dst->bin      = *bin;
        vfs_file_up(&dst->vfs_root);
        vfs_file_up(&dst->vfs_cwd);
        vfs_file_up(&dst->bin);
	rwlock_unlock(&dst->cwd_lock);

        return 0;
}

static struct task_s task0;

error_t task_restore(struct task_s *task, struct sys_exec_remote_s *exec_remote)
{
        uint_t err;

        /* Restore file descriptors */
        /* We need to update the fd_entry pointer */
        exec_remote->fd_info.fd_entry = exec_remote->fd_info.fd_entry_base;
        err = __task_fd_fork(&task->fd_info, &exec_remote->fd_info);
        if ( err )
                return err;

        /* Restore root, cwd and binary */
        err = task_dup_clone(task, &exec_remote->vfs_root, &exec_remote->vfs_cwd,       \
                        &exec_remote->bin);
        if ( err )
                return err;

        /* Init virtual memory */
        err = vmm_init(&task->vmm);
        task->vmm.limit_addr = CONFIG_USR_LIMIT; /* This value has been lost during the migration. */

        /* Get the physical page manager from ktask */
        err = pmm_init(&task->vmm.pmm, current_cluster);
        err = pmm_dup(&task->vmm.pmm, &task0.vmm.pmm);

        return err;
}

/* TODO: use atomic counter instead of spinlock */
struct tasks_manager_s
{
	atomic_t tm_next_clstr;
	atomic_t tm_next_cpu;
	spinlock_t tm_lock;
	pid_t tm_last_pid;

        /* Location management */
	struct task_locator_s tm_tbl[PID_MAX_LOCAL];
        struct hheader_s *tm_htable;
};

static struct tasks_manager_s tasks_mgr = 
{
	.tm_next_clstr = ATOMIC_INITIALIZER,
	.tm_next_cpu = ATOMIC_INITIALIZER,
        .tm_tbl[0].cid  = CID_NULL,
	.tm_tbl[0].task = &task0
};

void task_manager_init(void)
{
	spinlock_init(&tasks_mgr.tm_lock, "Tasks Mgr");

	memset(&tasks_mgr.tm_tbl[1], 
	       CID_NULL,
	       sizeof(struct task_locator_s) * (PID_MAX_LOCAL - 1));
}

void task_manager_init_finalize(void)
{
        error_t err;

        /* PID_MIN_GLOBAL is always used by task0 */
        tasks_mgr.tm_last_pid = PID_MIN_GLOBAL + 1;

        /* 0 means no flags */
        err = hhalloc(tasks_mgr.tm_htable, 0);
        if (err)
                PANIC("Failed to allocate tasks' manager hash table !\n");

        err = hhinit(tasks_mgr.tm_htable, htable_int_default,                   \
                        task_htable_pid_compare);

        /* hhinit() always returns zero, this is just in case of change */
        if (err)
                PANIC("Failed to initialize tasks' manager hash table !\n");

        return;
}

inline error_t tasks_manager_htable_hinsert(struct task_s *task)
{
        return hinsert(tasks_mgr.tm_htable, &task->t_hnode, &task->pid);

}

inline error_t tasks_manager_htable_hremove(struct task_s *task)
{
        return hremove(tasks_mgr.tm_htable, &task->pid);
}

RPC_DECLARE(__task_whereis,
                RPC_RET( RPC_RET_PTR(cid_t, location) ),
                RPC_ARG( RPC_ARG_VAL(pid_t, pid) )
           )
{
        tasks_manager_lock();
        *location = task_lookup(pid)->cid;
        tasks_manager_unlock();
        return;
}

cid_t task_whereis(pid_t pid)
{
        cid_t location;

        location = CID_NULL;
        RCPC(PID_GET_CLUSTER(pid), RPC_PRIO_TSK_LOOKUP, __task_whereis, \
                        RPC_RECV( RPC_RECV_OBJ(location) ),             \
                        RPC_SEND( RPC_SEND_OBJ(pid) ) );

        return location;

}

/* Assuming that task0 is correctly allocated */
inline struct task_s* task_lookup_zero()
{
        return tasks_mgr.tm_tbl[0].task;
}

struct task_locator_s* task_lookup(pid_t pid)
{
        struct task_locator_s *task_locator;

        /* Check if am I the owner of this task : this should always be true */
        assert(PID_GET_CLUSTER(pid) == current_cid);

        /* Avoid access to pid 0 */
        assert(PID_GET_LOCAL(pid) != PID_MIN_LOCAL);

        /* Check if this is a valid PID */
        if( PID_GET_LOCAL(pid) >= PID_MAX_LOCAL )
        {
                printk(WARNING, "WARNING: %s: Invalid pid (%u)", __FUNCTION__, pid);
                return NULL;
        }

        task_locator = &tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)];

        return task_locator;
}

/* Warning : new_task is an address only true in RPC_SEND_CID !*/
RPC_DECLARE(__task_pid_alloc,
                RPC_RET( RPC_RET_PTR(error_t, err), RPC_RET_PTR(pid_t, pid) ),
                RPC_ARG( RPC_ARG_PTR(struct task_s, new_task) )
           )
{

        pid_t   new_pid;
        error_t __err;
        uint_t  overlap;

        pid_dmsg(1, "%s: cluster %u asks for a pid on cluster %u\n",            \
                        __FUNCTION__, RPC_SENDER_CID, current_cid);

        __err   = EAGAIN;
        overlap = false;

        tasks_manager_lock();
        new_pid = tasks_mgr.tm_last_pid;

        while(tasks_mgr.tm_tbl[PID_GET_LOCAL(new_pid)].cid != CID_NULL)
        {
                if((++(new_pid)) > PID_MAX_GLOBAL)
                {
                        new_pid = PID_MIN_GLOBAL+1;
                        if(overlap == true)
                                break;
                        overlap = true;
                        pid_dmsg(1, "%s: overlap detected, get back to %u\n",   \
                                        __FUNCTION__, PID_MIN_GLOBAL);
                }
        }
        if(tasks_mgr.tm_tbl[PID_GET_LOCAL(new_pid)].cid == CID_NULL)
        {
                __err = 0;
                tasks_mgr.tm_tbl[PID_GET_LOCAL(new_pid)].cid  = RPC_SENDER_CID;
                tasks_mgr.tm_tbl[PID_GET_LOCAL(new_pid)].task = new_task;

                pid_dmsg(1, "%s: next pid will be %u\n", __FUNCTION__, new_pid);
        }

        tasks_mgr.tm_last_pid = new_pid;
        *pid = new_pid;
        *err = __err;
        tasks_manager_unlock();
}


/* Send a message to a cluster which will initialize a struct task_locator_s
 * in its tasks manager and deliver a pid. rcid is the cluster which deliver
 * it.
 */
error_t
task_pid_alloc(pid_t *new_pid, cid_t rcid, struct task_s *new_task)
{
        error_t err;

        /* Only send the new task address and not the content */
        RCPC( rcid, RPC_PRIO_PID_ALLOC, __task_pid_alloc,
              RPC_RECV( RPC_RECV_OBJ(err), RPC_RECV_OBJ(*new_pid) ),
              RPC_SEND( RPC_SEND_OBJ(new_task) )
            );

        return err;
}

inline ppn_t task_vaddr2ppn(struct task_s* task, void *vma)
{
	error_t err;
	pmm_page_info_t info;

	err = pmm_get_page(&task->vmm.pmm, (vma_t)vma, &info);
  
	if((err) || ((info.attr & PMM_PRESENT) == 0))
	{
		printk(WARNING, "WARNING: %s: core %d, pid %d, tid %d, vma 0x%x, err %d, attr 0x%x, ppn 0x%x\n",
		       __FUNCTION__,
		       cpu_get_id(),
		       task->pid,
		       current_thread->info.order,
		       vma,
		       err,
		       info.attr,
		       info.ppn);

		return 0;
	}

	return info.ppn;
}

inline void* task_vaddr2paddr(struct task_s* task, void *vma)
{
	uint_t paddr;
	ppn_t ppn;

	ppn = task_vaddr2ppn(task, vma);
	if(!ppn) return NULL;

	paddr = (ppn << PMM_PAGE_SHIFT) | ((vma_t)vma & PMM_PAGE_MASK);

	return (void*) paddr;
}

static void task_ctor(struct kcm_s *kcm, void *ptr);

error_t task_bootstrap_init(struct boot_info_s *info)
{
	task_ctor(NULL, &task0);
	memset(&task0.vmm, 0, sizeof(task0.vmm));
	task0.vmm.text_start  = CONFIG_KERNEL_OFFSET;
	task0.vmm.limit_addr  = CONFIG_KERNEL_OFFSET;
	//FIXME: the following should removed
	task0.vmm.devreg_addr = 0XFFFFFFFF;//CONFIG_DEVREGION_OFFSET; 
	pmm_bootstrap_init(&task0.vmm.pmm, info->boot_pgdir);
	/* Nota: vmm is not intialized */
	task0.vfs_root = (const struct vfs_file_s){ 0 };
        task0.vfs_cwd  = (const struct vfs_file_s){ 0 };
	task0.bin  = (const struct vfs_file_s){ 0 };
	task0.threads_count   = 0;
	task0.threads_nr      = 0;
	task0.threads_limit   = CONFIG_PTHREAD_THREADS_MAX;
	task0.pid             = PID_MIN_GLOBAL;
	atomic_init(&task0.childs_nr, 0);
	task0.childs_limit    = CONFIG_TASK_CHILDS_MAX_NR;
	return 0;
}

error_t task_bootstrap_finalize(struct boot_info_s *info)
{
	register error_t err;
  
	err = vmm_init(&task0.vmm);
  
	if(err) return err;

	err =  pmm_init(&task0.vmm.pmm, current_cluster);

	if(err)
	{
		printk(ERROR,"ERROR: %s: Failed, err %d\n", __FUNCTION__, err);
		while(1);
	}

	return 0;
}

/* Same thing that task_create but whitout pid allocation and hash table checking */
error_t task_create_clone(struct task_s **new_task, uint_t mode, pid_t pid, pid_t ppid)
{
	struct task_s *task;
	kmem_req_t req;
	uint_t err;

	assert(mode != CPU_SYS_MODE);

        req.type  = KMEM_TASK;
	req.size  = sizeof(struct task_s);
	req.flags = AF_KERNEL;
	req.ptr   = current_cluster;

	if((task = kmem_alloc(&req)) == NULL)
	{
		err = ENOMEM;
		goto fail_task_desc;
	}

	if((err = signal_manager_init(task)) != 0)
		goto fail_signal_mgr;

	req.type = KMEM_PAGE;
	req.size = (CONFIG_PTHREAD_THREADS_MAX * sizeof(struct thread_s*)) / PMM_PAGE_SIZE;
	req.size = (req.size == 0) ? 0 : bits_log2(req.size);

	task->th_tbl_pg = kmem_alloc(&req);

	if(task->th_tbl_pg == NULL)
	{
		err = ENOMEM;
		goto fail_th_tbl;
	}

	task->th_tbl = ppm_page2addr(task->th_tbl_pg);

	err = task_fd_init(task);

	if(err)
		goto fail_fd_info;

	memset(&task->vmm, 0, sizeof(task->vmm));

        tasks_manager_lock();

	task->cluster         = current_cluster;
	task->cpu             = current_cpu;
	task->vfs_root        = (const struct vfs_file_s){ 0 };
	task->vfs_cwd         = (const struct vfs_file_s){ 0 };
	task->bin             = (const struct vfs_file_s){ 0 };
	task->threads_count   = 0;
	task->threads_nr      = 0;
	task->threads_limit   = CONFIG_PTHREAD_THREADS_MAX;
	bitmap_set_range(task->bitmap, 0, CONFIG_PTHREAD_THREADS_MAX);
	task->pid             = pid;
        task->gid             = task->cpu->gid;
	task->state           = TASK_MIGRATE;
	atomic_init(&task->childs_nr, 0);
	task->childs_limit    = CONFIG_TASK_CHILDS_MAX_NR;
	*new_task             = task;
	tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].cid  = current_cid;
	tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].task = task;

        tasks_manager_unlock();
	cpu_wbflush();
	return 0;

fail_fd_info:
	req.type = KMEM_PAGE;
	req.ptr  = task->th_tbl_pg;
	kmem_free(&req);

fail_signal_mgr:
fail_th_tbl:
	req.type = KMEM_TASK;
	req.ptr  = task;
	kmem_free(&req);

fail_task_desc:
	tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].cid  = CID_NULL;
	tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].task = NULL;
	*new_task = NULL;
	return err;

}

error_t task_create(struct task_s **new_task, struct dqdt_attr_s *attr, uint_t mode)
{
	struct task_s *task;
	kmem_req_t req;
	pid_t pid;
	uint_t err;
  
	assert(mode != CPU_SYS_MODE);

        req.type  = KMEM_TASK;
	req.size  = sizeof(struct task_s);
	req.flags = AF_KERNEL | AF_REMOTE;
	req.ptr   = current_cluster;

	if((task = kmem_alloc(&req)) == NULL)
	{
		err = ENOMEM;
		goto fail_task_desc;
	}

        pid = 0;
        /* This function takes a lock on the tasks manager */
        if ((err = task_pid_alloc(&pid, attr->cid_exec, task)))
        {
                printk(WARNING, "WARNING: %s: cluster %u is out of PIDs\n", \
                                __FUNCTION__, attr->cid);
                goto fail_task_pid;
        }

	if((err = signal_manager_init(task)) != 0)
		goto fail_signal_mgr;

	req.type = KMEM_PAGE;
	req.size = (CONFIG_PTHREAD_THREADS_MAX * sizeof(struct thread_s*)) / PMM_PAGE_SIZE;
	req.size = (req.size == 0) ? 0 : bits_log2(req.size);
  
	task->th_tbl_pg = kmem_alloc(&req);

	if(task->th_tbl_pg == NULL)
	{
		err = ENOMEM;
		goto fail_th_tbl;
	}

	task->th_tbl = ppm_page2addr(task->th_tbl_pg);
  
	err = task_fd_init(task);

	if(err)
		goto fail_fd_info;

	memset(&task->vmm, 0, sizeof(task->vmm));

        tasks_manager_lock();
	task->cluster         = current_cluster;//attr->cluster;
	task->cpu             = cpu_lid2ptr(attr->cpu_id);//attr->cpu;
	task->vfs_root        = (const struct vfs_file_s){ 0 };
	task->vfs_cwd         = (const struct vfs_file_s){ 0 };
	task->bin             = (const struct vfs_file_s){ 0 };
	task->threads_count   = 0;
	task->threads_nr      = 0;
	task->threads_limit   = CONFIG_PTHREAD_THREADS_MAX;
	bitmap_set_range(task->bitmap, 0, CONFIG_PTHREAD_THREADS_MAX);
	task->pid             = pid;
        task->gid             = task->cpu->gid;
	task->state           = TASK_CREATE;
	atomic_init(&task->childs_nr, 0);
	task->childs_limit    = CONFIG_TASK_CHILDS_MAX_NR;
	*new_task             = task;

        /* Current cluster is not the task's anchor, only the creator */
        if ( PID_GET_CLUSTER(task->pid) != current_cid )
        {
                if((err = tasks_manager_htable_hinsert(task)))
                        goto fail_htbl;
        }
        else
        {
                tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].cid  = current_cid;
                tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].task = task;
        }

        tasks_manager_unlock();
	cpu_wbflush();
	return 0;

fail_htbl:
fail_fd_info:
	req.type = KMEM_PAGE;
	req.ptr  = task->th_tbl_pg;
	kmem_free(&req);

fail_task_pid:
        /* Don't allow PID 0 */
        if ( (tasks_mgr.tm_last_pid > PID_MIN_GLOBAL+1) && (attr->cid_exec == current_cid) )
                tasks_mgr.tm_last_pid--;
fail_signal_mgr:
fail_th_tbl:
        if ( attr->cid == current_cid )
        {
                tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].cid  = CID_NULL;
                tasks_mgr.tm_tbl[PID_GET_LOCAL(pid)].task = NULL;
        }
	req.type = KMEM_TASK;
	req.ptr  = task;
	kmem_free(&req);

fail_task_desc:
	*new_task = NULL;
	return err;
}

error_t task_dup(struct task_s *dst, struct task_s *src)
{
	rwlock_wrlock(&src->cwd_lock);

	vfs_file_up(&src->vfs_root);
	vfs_file_up(&src->vfs_cwd);

	dst->vfs_root = src->vfs_root;
	dst->vfs_cwd  = src->vfs_cwd;

	rwlock_unlock(&src->cwd_lock);
  
	task_fd_fork(dst, src);

	//assert(src->bin != NULL);
	vfs_file_up(&src->bin);
        dst->bin = src->bin;
	return 0;
}

void task_destroy(struct task_s *task)
{
	kmem_req_t req;
	register pid_t pid;
	uint_t pgfault_nr;
	uint_t spurious_pgfault_nr;
	uint_t remote_pages_nr;
	uint_t u_err_nr;
	uint_t m_err_nr;
        error_t err;

	assert(task->threads_nr == 0 && 
	       "Unexpected task destruction, One or more Threads still active\n");

	pid = task->pid;
	signal_manager_destroy(task);

        tasks_manager_lock();

	tasks_mgr.tm_tbl[PID_GET_LOCAL(task->pid)].cid  = CID_NULL;
	tasks_mgr.tm_tbl[PID_GET_LOCAL(task->pid)].task = NULL;

        /* Current cluster is not the task's anchor, only the creator */
        if (PID_GET_CLUSTER(task->pid) != current_cid)
        {
                if ((err = tasks_manager_htable_hremove(task)))
                        sig_dmsg(1, "%s: Cannot remove task %u from tasks manager hash table on cluster %u\n",
                                __FUNCTION__, task->pid, current_cid);
        }
	cpu_wbflush();

        tasks_manager_unlock();
        /* From this point, the task is unreachable. There is no signal
         * manager, and the struct task_s is not in the tasks manager and the
         * chained hash table.
         */

        /* Delete all file descriptors (and also closed opened files */
	err = task_fd_destroy(task);
        if ( err )
                printk(WARNING, "%s: task %u has not closed all opened files\n",
                                __FUNCTION__, pid);

        /* Close binary */
	//if(task->bin != NULL) TODO!
		vfs_close(&task->bin, NULL);

	vfs_file_down(&task->vfs_root);
	vfs_file_down(&task->vfs_cwd);

	pgfault_nr          = task->vmm.pgfault_nr;
	spurious_pgfault_nr = task->vmm.spurious_pgfault_nr;
	remote_pages_nr     = task->vmm.remote_pages_nr;
	u_err_nr            = task->vmm.u_err_nr;
	m_err_nr            = task->vmm.m_err_nr;

        /* Destroy virtual and physical memory managers */
	vmm_destroy(&task->vmm);
	pmm_release(&task->vmm.pmm);
	pmm_destroy(&task->vmm.pmm);

	req.type = KMEM_PAGE;
	req.ptr  = task->th_tbl_pg;
	kmem_free(&req);

	req.type = KMEM_TASK;
	req.ptr  = task;

        /* Free memory */
	kmem_free(&req);

	printk(INFO, "INFO: %s: pid %d [ %d, %d, %d, %d, %d ]\n",
	       __FUNCTION__, 
	       pid, 
	       pgfault_nr,
	       spurious_pgfault_nr,
	       remote_pages_nr,
	       u_err_nr,
	       m_err_nr);

        return;
}

bool_t task_htable_pid_compare(struct hnode_s *hn, void* key)
{
       struct task_s    *task;
       pid_t            *pid;

       pid = key;
       task = container_of(hn, struct task_s, t_hnode);

       if (task->pid == *pid)
               return true;
       else
               return false;
}

inline struct hheader_s* tasks_manager_get_htable()
{
        return tasks_mgr.tm_htable;
}

inline void tasks_manager_lock()
{
        spinlock_lock(&tasks_mgr.tm_lock);
        pid_dmsg(DEBUG, "DEBUG: %s: Task manager locked by task %x\n",    \
                        __FUNCTION__, current_task);
}

inline void tasks_manager_unlock()
{
        spinlock_unlock(&tasks_mgr.tm_lock);
        pid_dmsg(DEBUG, "DEBUG: %s: Task manager unlocked by task %x\n",    \
                        __FUNCTION__, current_task);
}

static void task_ctor(struct kcm_s *kcm, void *ptr)
{
        struct task_s *task = ptr;

	mcs_lock_init(&task->block, "Task");
	spinlock_init(&task->lock, "Task");
	rwlock_init(&task->cwd_lock);
	spinlock_init(&task->th_lock, "Task threads");
	spinlock_init(&task->tm_lock, "Task Time");
	list_root_init(&task->children);
	list_root_init(&task->th_root);
}

KMEM_OBJATTR_INIT(task_kmem_init)
{
	attr->type   = KMEM_TASK;
	attr->name   = "KCM Task";
	attr->size   = sizeof(struct task_s);
	attr->aligne = 0;
	attr->min    = CONFIG_TASK_KCM_MIN;
	attr->max    = CONFIG_TASK_KCM_MAX;
	attr->ctor   = task_ctor;
	attr->dtor   = NULL;
	return 0;
}

KMEM_OBJATTR_INIT(task_fdinfo_kmem_init)
{
	attr->type   = KMEM_FDINFO;
	attr->name   = "KCM FDINFO";
	attr->size   = sizeof(struct fd_info_s);
	attr->aligne = 0;
	attr->min    = CONFIG_FDINFO_KCM_MIN;
	attr->max    = CONFIG_FDINFO_KCM_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}
