/*
 * kern/kernel-config.h - global kernel configurations
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

#ifndef _KERNEL_CONFIG_H_
#define _KERNEL_CONFIG_H_

#ifndef _CONFIG_H_
#error This config-file is not to be included directely, use config.h instead
#endif


////////////////////////////////////////////////////
//             KERNEL REVISION INFO               //
////////////////////////////////////////////////////
#define CONFIG_ALMOS_VERSION  \
"Almos almos-mk-v0.1 2015 "CONFIG_ARCH_NAME" "CONFIG_CPU_NAME" "CONFIG_CPU_ABI_NAME

////////////////////////////////////////////////////

////////////////////////////////////////////////////
//        KERNEL SUBSYSTEMS CONFIGURATIONS        //
////////////////////////////////////////////////////
#define CONFIG_FIFO_SUBSYSTEM            no
#define CONFIG_ROOTFS_IS_EXT2            no
#define CONFIG_ROOTFS_IS_VFAT            yes

////////////////////////////////////////////////////

////////////////////////////////////////////////////
//         TASK MANAGEMENT CONFIGURATIONS         //
////////////////////////////////////////////////////
#define CONFIG_TASK_MAX_NR               1024
#define CONFIG_TASK_MAX_NR_POW           10
#define CONFIG_TASK_FILE_MAX_NR          8
#define CONFIG_TASK_CHILDS_MAX_NR        512
#define CONFIG_TASK_ARGS_PAGES_MAX_NR    32
#define CONFIG_TASK_HEAP_MIN_SIZE        0x00010000
#define CONFIG_TASK_HEAP_MAX_SIZE        0x30000000

////////////////////////////////////////////////////
//          KERNEL GENERAL CONFIGURATIONS         //
////////////////////////////////////////////////////
#define CONFIG_DEV_VERSION               yes
#define CONFIG_KERNEL_REPLICATE          yes
#define CONFIG_USE_COA                   yes
#define CONFIG_MAPPER_AUTO_MGRT          yes
#define CONFIG_EXEC_LOCAL                no
#define CONFIG_USE_SCHED_LOCKS           yes
#define CONFIG_THREAD_LOCAL_ALLOC        yes
#define CONFIG_REMOTE_THREAD_CREATE      no
#define CONFIG_USE_KEYSDB                yes
#define CONFIG_SHOW_BOOT_BANNER          yes
#define CONFIG_MAX_CLUSTER_NR            256
#define CONFIG_MAX_DEVICE_NR             128
#define CONFIG_MAX_CLUSTER_ROOT          16
#define CONFIG_MAX_CPU_PER_CLUSTER_NR    4
#define CONFIG_GLOBAL_CLUSTERS_ORDER     0
#define CONFIG_GLOBAL_CORES_ORDER        0
#define CONFIG_MAX_DQDT_DEPTH            4
#define CONFIG_USE_DQDT                  no
#define CONFIG_DQDT_LEVELS_NR            5
#define CONFIG_DQDT_MGR_PERIOD           1
#define CONFIG_DQDT_ROOTMGR_PERIOD       4
#define CONFIG_DQDT_WAIT_FOR_UPDATE      no
#define CONFIG_CPU_BALANCING_PERIOD      4
#define CONFIG_CPU_LOAD_PERIOD           4
#define CONFIG_CLUSTER_KEYS_NR           8
#define CONFIG_REL_KFIFO_SIZE            32
#define CONFIG_VFS_NODES_PER_CLUSTER     128
#define CONFIG_SCHED_THREADS_NR          32
#define CONFIG_BARRIER_WQDB_NR           4
#define CONFIG_BARRIER_ACTIVE_WAIT       no
#define CONFIG_BARRIER_BORADCAST_UREAD   no
#define CONFIG_CPU_LOAD_BALANCING        no //yes, FIXME(40): manipulate dqdt
#define CONFIG_PTHREAD_THREADS_MAX       2048
#define CONFIG_PTHREAD_STACK_SIZE        512*1024
#define CONFIG_PTHREAD_STACK_MIN         4096
#define CONFIG_RPC_FIFO_SLOT_NR		 128
#define CONFIG_ENV_MAX_SIZE              128

////////////////////////////////////////////////////
//          KERNEL DEBUG CONFIGURATIONS           //
////////////////////////////////////////////////////
#define CONFIG_DMSG_LEVEL                DMSG_DEBUG
#define CONFIG_XICU_USR_ACCESS           no
#define CONFIG_SPINLOCK_CHECK            no
#define CONFIG_SPINLOCK_TIMEOUT          100
#define CONFIG_ENABLE_TASK_TRACE         no
#define CONFIG_ENABLE_THREAD_TRACE       no
#define CONFIG_SHOW_PAGEFAULT            no
#define CONFIG_SHOW_CPU_USAGE            no
#define CONFIG_SHOW_CPU_IPI_MSG          no
#define CONFIG_SHOW_FPU_MSG              no
#define CONFIG_SHOW_THREAD_MSG           no
#define CONFIG_SHOW_PPM_PGALLOC_MSG      no
#define CONFIG_SHOW_VMM_LOOKUP_TM        no
#define CONFIG_SHOW_VMM_ERROR_MSG        no
#define CONFIG_SHOW_SPURIOUS_PGFAULT     no
#define CONFIG_SHOW_MIGRATE_MSG          no
#define CONFIG_SHOW_VMMMGRT_MSG          no
#define CONFIG_SHOW_SYSMGRT_MSG          no
#define CONFIG_SHOW_REMOTE_PGALLOC       no
#define CONFIG_SHOW_LOCAL_EVENTS         no
#define CONFIG_SHOW_SIG_MSG              no
#define CONFIG_SCHED_SHOW_NOTIFICATIONS  no
#define CONFIG_SHOW_EPC_CPU0             no
#define CONFIG_CPU_TRACE                 no
#define CONFIG_DQDT_DEBUG                no
#define CONFIG_KFIFO_DEBUG               no
#define CONFIG_KHM_DEBUG                 no
#define CONFIG_KCM_DEBUG                 no
#define CONFIG_KMEM_DEBUG                no
#define CONFIG_MAPPER_DEBUG              no
#define CONFIG_SHOW_KMEM_INIT            no
#define CONFIG_MEM_CHECK                 no
#define CONFIG_THREAD_TIME_STAT          no
#define CONFIG_SCHED_RR_CHECK            no
#define CONFIG_FORK_DEBUG                no
#define CONFIG_VMM_DEBUG                 no
#define CONFIG_VMM_REGION_DEBUG          no
#define CONFIG_ELF_DEBUG                 no
#define CONFIG_VFAT_PGWRITE_ENABLE       no
#define CONFIG_VFAT_DEBUG                no
#define CONFIG_VFAT_INSTRUMENT           no
#define CONFIG_EXT2_DEBUG                no
#define CONFIG_METAFS_DEBUG              no
#define CONFIG_VFS_DEBUG                 no
#define CONFIG_DEVFS_DEBUG               no
#define CONFIG_SYSFS_DEBUG               no
#define CONFIG_BLKIO_DEBUG               no
#define CONFIG_BC_DEBUG                  no
#define CONFIG_BC_INSTRUMENT             no
#define CONFIG_LOCKS_DEBUG               no
#define CONFIG_SCHED_DEBUG               no
#define CONFIG_VERBOSE_LOCK              no
#define CONFIG_PID_DEBUG                 yes
#define CONFIG_HTBL_DEBUG                no
#define CONFIG_SHOW_RPC_MSG              no
#define CONFIG_EXEC_DEBUG                yes
#define CONFIG_USE_RPC_TEST              no     /* Don't test RPC latency at boot */
#define CONFIG_SHOW_ALL_BOOT_MSG         no
//////////////////////////////////////////////

//////////////////////////////////////////////
//      USER APPLICATION
//////////////////////////////////////////////
#define CONFIG_DEV_STDIN           "/dev/tty2"
#define CONFIG_DEV_STDOUT          "/dev/tty2"
#define CONFIG_DEV_STDERR          "/dev/tty3"
//////////////////////////////////////////////

#endif	/* _KERNEL_CONFIG_H_ */
