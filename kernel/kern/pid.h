/*
 * kern/pid.h - pid related operations
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

#ifndef _PID_H_
#define _PID_H_

#include <config.h>

/* A kernel can allocate a pid between PID_MIN_GLOBAL and PID_MAX_GLOBAL (both included) */
#define PID_MIN_LOCAL           0
#define PID_MAX_LOCAL           CONFIG_TASK_MAX_NR
#define PID_MIN_GLOBAL          (current_cid*PID_MAX_LOCAL)
#define PID_MAX_GLOBAL          (((current_cid+1)*PID_MAX_LOCAL)-1)
#define PID_MASK                (0xFFFFFFFF-(PID_MAX_LOCAL-1))
#define PID_GET_LOCAL(x)        (x & (PID_MAX_LOCAL-1))
#define PID_GET_CLUSTER(x)      ((x & PID_MASK) >> CONFIG_TASK_MAX_NR_POW)

#endif /* _PID_H_ */
