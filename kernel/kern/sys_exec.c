/*
 * kern/sys_exec.c - executes a new program (main work is done by do_exec)
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
#include <cpu.h>
#include <vfs.h>
#include <cluster.h>
#include <task.h>
#include <pid.h>
#include <thread.h>
#include <vmm.h>
#include <pmm.h>
#include <ppm.h>
#include <kdmsg.h>
#include <task.h>
#include <utils.h>
#include <string.h>

/* This function is executed on the recipient cluster */
/* FIXME #1 : don't use generic environment */
RPC_DECLARE(__sys_exec,                                                                 \
                RPC_RET( RPC_RET_PTR(int,    err) ),                                    \
                RPC_ARG( RPC_ARG_PTR(struct sys_exec_remote_s, exec_remote))            \
        )
{
        uint_t          i;
        uint_t          len;
	uint_t          isFatal;
        kmem_req_t      req;
        struct task_s*  task;
        struct thread_s *main_thread;
        char *envp[] = {"ALMOS_VERSION="CONFIG_ALMOS_VERSION, "PWD=/", NULL};   /* FIXME #1 */
        char *argv[3];
        char *filename;

        /* Prepare buffer for argv */
        req.type  = KMEM_GENERIC;
        req.flags = AF_KERNEL | AF_ZERO;
        req.size  = CONFIG_ENV_MAX_SIZE*sizeof(char);

        for ( i = 0; i < 3; i++ )
        {
                req.ptr = argv[i];
                argv[i] = kmem_alloc(&req);
                if ( argv[i] == NULL )
                {
                        isFatal = 1;
                        goto fail_args;
                }
        }

        filename = strrchr(exec_remote->path, '/');
        if ( filename == NULL )
                goto fail_args;
        else
                filename++;

        /* Fill argv */
        strcpy(argv[0], exec_remote->path);
        strcpy(argv[1], filename);
        argv[2] = NULL;

	len       = 0;
	isFatal   = 1;

        /* In case of real RPC between two clusters, we need to allocate a new
         * struct task_s on the recipient cluster
         */
        if ( RPC_SENDER_CID != current_cid )
        {
                exec_dmsg(1, "%s: real RPC. Creating clone on cluster %u\n",            \
                                __FUNCTION__, current_cid);

                /* Clone exec' task and restore pid/ppid. It also updates the tasks manager
                 * information for this task */
                *err = task_create_clone(&task, CPU_USR_MODE, exec_remote->pid,         \
                                exec_remote->ppid);

                if ( *err )
                        goto fail_clone;

                exec_dmsg(1, "%s: new task is %x. Restoring minimal informations\n",    \
                                __FUNCTION__, task);

                /* Restore file descriptors, root, cwd and binary */
                *err = task_restore(task, exec_remote);
                if ( *err )
                        goto fail_restore;
        }
        else /* Find the current task */
        {
                exec_dmsg(1, "%s: local exec. Retrieving task %u on cluster %u\n",      \
                                __FUNCTION__, exec_remote->pid, current_cid);
                /* Retrieve exec' task */
                task = task_lookup(exec_remote->pid)->task;

                if ( task == NULL )
                        goto fail_task;
        }

	signal_manager_destroy(task);
	signal_manager_init(task);

        exec_dmsg(1, "%s: task %u: do_exec(@t:0x%x, %s, @argv:0x%x, @envp:0x%x)\n",     \
                        __FUNCTION__, exec_remote->pid, task, exec_remote->path,        \
                        &argv, &envp);

        /* Once the task is restored, do the exec() */
	*err = do_exec(task, exec_remote->path, &argv[0], &envp[0], &isFatal,           \
                        &main_thread, kexec_copy, kexec_strlen);

        /* No error */
        if(*err == 0)
	{
                exec_dmsg(1, "%s: task %u: do_exec(@t:0x%x, %s, @argv:0x%x, @envp:0x%x):done\n",\
                        __FUNCTION__, exec_remote->pid, task, exec_remote->path, &argv,         \
                        &envp);

		assert(main_thread != NULL && (main_thread->signature == THREAD_ID));

                /* Register new thread on scheduler */
		*err = sched_register(main_thread);
		assert(*err == 0);
		task->state = TASK_READY;

#if CONFIG_ENABLE_TASK_TRACE
		main_thread->info.isTraced = true;
#endif
                /* Add new thread on scheduler. The initial thread which initiates this RPC will be
                 * destroy in the sys_exec() function. Once it's done, and in case of "fake" RPC,
                 * main_thread starts its execution. In case of real RPC, main_thread doesn't need
                 * to wait the initial thread to be destroyed to starts its execution.
                 */
                sched_add_created(main_thread);

                exec_dmsg(1, "%s: main thread (%p) added to scheduler on cpu %u, cluster %u\n", \
                                __FUNCTION__, main_thread, main_thread->info.attr.cpu_lid,      \
                                main_thread->info.attr.cid);

                return;
	}

fail_args:
fail_task:
fail_clone:
fail_restore:
	if(isFatal)
                *err = isFatal;
	return;
}

/* Get path to new binary in a kernel buffer. We must do this step on the source cluster because
 * recipient cluster can't receive an user buffer. A RPC must send kernel buffer.
 */
static error_t get_upath(char* ufilename, char* kfilename)
{
        error_t err;
        uint_t len;

	if(( err = cpu_uspace_strlen(ufilename, &len) ))
                return err;

	if(++len >= PMM_PAGE_SIZE)
		return E2BIG;

	if(( err  = cpu_copy_from_uspace(kfilename, ufilename, len) ))
                return err;
        else
                return 0;
}

int sys_exec(char *filename, char **argv, char **envp)
{
	error_t err;
	struct task_s *task;

	task    = current_thread->task;
        err     = 0;

        exec_dmsg(1, "%s: cpu %d thread %p, started [%d]\n",
		  __FUNCTION__,
		  cpu_get_id(),
                  current_thread,
		  cpu_time_stamp());

        /* Basic checks : args, threads number and binary */
	if((filename == NULL) || (argv == NULL) || (envp == NULL))
	{
		err = EINVAL;
		goto fail_args;
	}

	if(task->threads_nr != 1)
	{
		printk(INFO, "INFO: %s: current task (pid %d) has more than 1 thread !\n", 
		       __FUNCTION__, 
		       task->pid);

		err = EACCES;
		goto fail_access;
	}

        /* Change task's state to prevent any concurent access */
        tasks_manager_lock();
        task->state = TASK_MIGRATE;
        tasks_manager_unlock();

        /* Prepare all informations for recipient */
        /* This structure is very heavy : more than 1024 bytes ! */
        struct sys_exec_remote_s exec_remote =  { .pid       = task->pid,
                                                  .ppid      = task->parent,
                                                  .fd_info   = task->fd_info,
                                                  .vfs_root  = task->vfs_root,
                                                  .vfs_cwd   = task->vfs_cwd,
                                                  .bin       = task->bin
                                                };

        if ( (err = get_upath(filename, exec_remote.path) ) )
                        goto fail_path;

        /* From this point, assuming that current task exists in two clusters.
         * If the RPC returns an error, then the remote exec() failed and we
         * can restore this task. If there is no error, destroy this task.
         */
        RCPC( PID_GET_CLUSTER(task->pid), RPC_PRIO_EXEC, __sys_exec,                    \
                        RPC_RECV( RPC_RECV_OBJ(err) ),                                  \
                        RPC_SEND( RPC_SEND_OBJ(exec_remote))                            \
            );

        /* In case of error, current task is not modified and all operations
         * on recipient have been canceled
         */
        if (err)
        {
                exec_dmsg(1, "%s: error %u during exec RPC\n",                          \
                                __FUNCTION__, err);
                goto fail_rpc;
        }

        exec_dmsg(1, "%s: task has been exec'd on cluster %u\n",                        \
                        __FUNCTION__, PID_GET_CLUSTER(task->pid));

        /* If "fake" RPC, destroy old thread and let main_thread created in do_exec() do the right
         * stuff. Otherwise, delete the task on this cluster.
         */
        if ( PID_GET_CLUSTER(task->pid) == current_cid )
        {
                exec_dmsg(1, "%s: thread %p will exit\n",                               \
                                __FUNCTION__, current_thread);

                thread_set_no_vmregion(current_thread);
                sched_exit(current_thread);
        }
        else
        {
                exec_dmsg(1, "%s: destroying task %p on cluster %u\n",                  \
                                __FUNCTION__, task, current_cid);

                /* This operation calls task_destroy() and delete the task in the hash table if necessary */
                sched_remove(current_thread);
        }

        return 0;

fail_rpc:
fail_path:
fail_access:
fail_args:
	current_thread->info.errno = err;
	task->state = TASK_READY;
        return -1;
}

