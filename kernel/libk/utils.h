/*
 * libk/utils.h - useful functions for the kernel
 * 
 * Copyright (c) 2015 UPMC Sorbonne Universites
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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <task.h>
#include <thread.h>
#include <cluster.h>

struct kernel_iter_s {
        cid_t cid;
};

struct kernel_iter_s kernel_list;

/* This structure contains minimal informations to re-build a task from ashes in case of remote
 * exec. It's used in kern/sys_exec.c and kern/task.c in the task_restore() function */
#define MAX_NAME_SIZE   CONFIG_ENV_MAX_SIZE
struct sys_exec_remote_s
{
        pid_t                   pid;
        pid_t                   ppid;
        char                    path[MAX_NAME_SIZE];
        struct fd_info_s        fd_info;
        struct vfs_file_s       vfs_root;
        struct vfs_file_s       vfs_cwd;
        struct vfs_file_s       bin;
};

/* Those functions are used on init loading and in case of remote exec */
error_t kexec_copy(void *dst, void *src, uint_t count);
error_t kexec_strlen(char *dst, uint_t *count);

/* The number of kernel running on the platform (some clusters does not have
 * a kernel, like the I/O cluster.
 */
#define  KERNEL_TOTAL_NR         current_cluster->clstr_wram_nr

/* From Linux */
#define container_of(ptr, type, member) ({                              \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);            \
                (type *)( (char *)__mptr - offsetof(type,member) );})

/* Assuming that all running kernels are contiguously in cluster 0,1,2 etc...,
 * and empty cluster and I/O cluster are the two last. This is a very
 * dangerous assumption, but for now we can't do otherwise. It should be
 * marked as FIXME. This comment is also true for kernel_foreach_backward.
 */
#define kernel_foreach(iter, next)                              \
        iter = &kernel_list;                                    \
        next = 0;                                               \
        for ( iter->cid = 0; iter->cid < KERNEL_TOTAL_NR;       \
                        iter->cid++, next = iter->cid )

/* cid_t is an unsigned int16_t and CID_NULL == (cid_t)-1 == 65535. When the
 * loop is doing 0-1, it's equal to 65535, and so equal to CID_NULL, that's
 * why this is the end loop condition.
 * 
 * You are not expected to understand this.
 *
 */
#define kernel_foreach_backward_init(iter,next)                 \
        iter = &kernel_list;                                    \
        next = KERNEL_TOTAL_NR-1;

#define kernel_foreach_backward(iter, next)                     \
        kernel_foreach_backward_init(iter,next)                 \
        for ( iter->cid = KERNEL_TOTAL_NR-1;                    \
                        iter->cid != CID_NULL;                  \
                        iter->cid--, next = iter->cid )

#endif /* _UTILS_H_ */
