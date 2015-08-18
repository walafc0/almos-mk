/*
   This file is part of MutekP.
  
   MutekP is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   MutekP is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with MutekP; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2008
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <sys/types.h>
#include <getopt.h>

#define O_DIRECTORY  0x00040000
#define O_APPEND     0x00080000
#define O_RDONLY     0x00100000
#define O_WRONLY     0x00200000
#define O_RDWR       0x00300000
#define O_CREAT      0x00400000
#define O_EXCL       0x00800000
#define O_TRUNC      0x01000000
#define O_SYNC       0x08000000
#define O_DSYNC      O_SYNC
#define O_RSYNC      O_SYNC

#define SEEK_SET   0
#define SEEK_CUR   1
#define SEEK_END   2

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define _SC_OPEN_MAX         0
#define _SC_CLK_TCK          1
#define _SC_PAGESIZE         2
#define _SC_ARG_MAX          3
#define _SC_NGROUPS_MAX      4
#define _SC_NPROCESSORS_ONLN 5
#define _SC_PAGE_SIZE        6
#define _SC_CACHE_LINE_SIZE  7
#define _SC_HEAP_MAX_SIZE    8
#define _SC_NCLUSTERS_ONLN   9

//#define LIBC_DEBUG
long sysconf(int name);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *pathname);
int close(int fd);
int fsync(int fd);
int chdir(const char *path);
int rmdir(const char *pathname);
char *getcwd(char *buf, size_t size);
char *getwd(char *buf);
int pipe(int pipefd[2]);

unsigned int sleep (unsigned int nb_sec);
int isatty(int fd);

char *crypt(const char *key, const char *salt);
void encrypt(char block[64], int edflag);
void setkey(const char *key);

cid_t getcid(void);
pid_t getpid(void);
pid_t fork(void);

int execve(const char *filename, char *const argv[], char *const envp[]);

int execl(const char *path, const char *arg, ...);
int execlp(const char* file, const char *arg, ...);
//int execle(const char *path, ...);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);


#ifdef LIBC_DEBUG
#include <string.h>
static char __buff_[128];
#define log_msg(msg, ...)					\
  ({								\
    memset(__buff_, 0, 128);					\
    write(2,__buff_, sprintf(__buff_, msg, __VA_ARGS__));		\
  })
#else
#define log_msg(msg, ...)
#endif


#endif	/* _UNISTD_H_ */
