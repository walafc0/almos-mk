/*
 * vfs/vfs-errno.h - vfs to errno error-codes conversion
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

#ifndef _VFS_ERRNO_H_
#define _VFS_ERRNO_H_

#include <errno.h>

/* TODO: remove the usage of this file, use errno instead ! */

#define VFS_FOUND            0
#define VFS_NOT_FOUND        ENOENT
#define VFS_ENOMEM           ENOMEM
#define VFS_ENOSPC           ENOSPC
#define VFS_IO_ERR           EIO
#define VFS_EUNKNOWN         EUNKNOWN
#define VFS_EINVAL           EINVAL
#define VFS_EBADBLK          EBADBLK
#define VFS_EEXIST           EEXIST
#define VFS_EISDIR           EISDIR
#define VFS_EBADF            EBADF
#define VFS_EISPIPE          EISPIPE
#define VFS_EOVERFLOW        EOVERFLOW
#define VFS_ENOTDIR          ENOTDIR
#define VFS_EODIR            EEODIR
#define VFS_ENOTUSED         ENOTSUPPORTED
#define VFS_EPIPE            EPIPE
#define VFS_ENOTEMPTY        ENOTEMPTY

#endif
