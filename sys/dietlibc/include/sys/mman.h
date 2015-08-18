#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H

#include <sys/types.h>

#define MREMAP_MAYMOVE	1UL
#define MREMAP_FIXED	2UL

#define PROT_READ	0x1		/* page can be read */
#define PROT_WRITE	0x2		/* page can be written */
#define PROT_EXEC	0x4		/* page can be executed */
#define PROT_NONE	0x0		/* page can not be accessed */

#define MAP_SHARED	0x0001		/* Share changes */
#define MAP_PRIVATE	0x0002		/* Changes are private */
#define MAP_ANONYMOUS	0x0004		/* don't use a file */
#define MAP_GROWSDOWN	0x0008		/* stack-like segment */
#define MAP_STACK       0x0008
#define MAP_LOCKED	0x0010		/* pages are locked */
#define MAP_HUGETLB     0x0020		/* Allocate the mapping using "huge pages." */
#define MAP_FIXED	0x0040		/* Interpret addr exactly */
#define MAP_DENYWRITE	0x200000	/* ETXTBSY */
#define MAP_EXECUTABLE	0x400000	/* mark it as an executable */
#define MAP_32BIT       0x800000	/* Put the mapping into the first 2 Gigabytes of the process address space. */

#define MAP_POPULATE	0x010000
#define MAP_NORESERVE	0x020000	/* don't check for reservations */


#define MS_ASYNC	0x0001		/* sync memory asynchronously */
#define MS_INVALIDATE	0x0002		/* invalidate mappings & caches */
#define MS_SYNC		0x0004		/* synchronous memory sync */
#define MCL_CURRENT	1		/* lock all current mappings */
#define MCL_FUTURE	2		/* lock all future mappings */
#define MADV_NORMAL	0x0		/* default page-in behavior */
#define MADV_RANDOM	0x1		/* page-in minimum required */
#define MADV_SEQUENTIAL	0x2		/* read-ahead aggressively */
#define MADV_WILLNEED	0x3		/* pre-fault pages */
#define MADV_DONTNEED	0x4		/* discard these pages */
#define MADV_MIGRATE    0x5		/* migrate page on next-touch */

/* compatibility flags */
#define MAP_ANON	MAP_ANONYMOUS
#define MAP_FILE	0

#define MAP_FAILED      ((void *) -1)

void *mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset);

int munmap (void *__addr, size_t __len);
extern int mprotect (void *__addr, size_t __len, int __prot);
extern int msync (void *__addr, size_t __len, int __flags);
extern void *mremap (void *__addr, size_t __old_len, size_t __new_len,
		     unsigned long __may_move);
extern int mincore (void *__start, size_t __len, unsigned char *__vec);

/* Public structure used by mcntl */
typedef struct
{
  uint_t mi_cid;		/* cluster id */
  uint_t mi_cx;			/* cluster x coordinate */
  uint_t mi_cy;			/* cluster y coordinate */
  uint_t mi_cz;			/* cluster z coordinate */
} minfo_t;

/* Commands used by mcntl */
#define MCNTL_READ           0x0
#define MCNTL_L1_iFLUSH      0x1
#define MCNTL_MOVE           0x2 /* this operation is not implemented in this version */

/**
 * Control of process virtual address & core caches
 *
 * @op       : (in) one of MCNTL_XXX operations
 * @addr     : (in) virtual address (will be arround to page base address)
 * @len      : (in) number of pages 
 * @info     : (in/out) addr related info
 * 
 * @return   : 0 if OK
 *
 * Rationale: 
 *   MCNTL_READ    : fills info structre and doesn't need len parameter.
 *   MCNTL_MOVE    : uses info structre as input parameter and must be initialized correctly.
 **/
extern int mcntl(int op, void *vaddr, size_t len, minfo_t *info);

int mlockall(int flags) ;
int mlock(const void *addr, size_t len);
int munlock(const void *addr, size_t len);
int munlockall(void);

int madvise(void *start, size_t length, int advice);

#define _POSIX_MAPPED_FILES

#endif
