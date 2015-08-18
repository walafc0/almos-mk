
#ifndef	 _CPU_REMOTE_H_ 
#define	 _CPU_REMOTE_H_ 

#include <system.h>
//FIXME use cpu.h

/* Remote acess operations */
//static inline sint_t cpu_remote_atomic_add(void *ptr, uint_t cid, 
//						sint_t add_val);
static inline void cpu_remote_memcpy(void* dest, uint_t dcid, 
			void* src, uint_t scid, size_t size);
static inline void cpu_remote_sw(void* addr, uint_t cid, uint_t val);
static inline void cpu_remote_sb(void* addr, uint_t cid, char val);
static inline uint_t cpu_remote_lw(void* addr, uint_t cid);
static inline bool_t cpu_remote_atomic_cas(void *ptr, uint_t cid, 
					sint_t old, sint_t new);

/* Remote acess operations */
#define mips_remote_store(addr,ext,val,op)				\
	do{								\
		__asm__ volatile(					\
		"mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT */	\
		"mtc2   %1,     $24                \n"     /* PADDR_EXT <= ext */	\
		#op" 	%0,	0(%2)		   \n"					\
		"mtc2   $8,     $24                \n"     /* PADDR_EXT <= $8 */	\
		::"r" (val), "r"(ext), "r" (addr): "$8");		\
	}while(0)

static inline void cpu_remote_sw(void* addr, uint_t cid, uint_t val)
{
	uint_t acid;
	acid = arch_clst_arch_cid(cid);

	mips_remote_store(addr, acid, val, sw);
}

static inline void cpu_remote_sb(void* addr, uint_t cid, char val)
{
	uint_t acid;
	acid = arch_clst_arch_cid(cid);

	mips_remote_store(addr, acid, val, sb);
}

static inline uint_t cpu_remote_lw(void* addr, uint_t cid)
{
	uint_t val;
	uint_t acid;
	acid = arch_clst_arch_cid(cid);

	__asm__ volatile(					
	"mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT */	
	"mtc2   %1,     $24                \n"     /* PADDR_EXT <= ext */	
	"lw 	%0,	0(%2)		   \n"					
	"mtc2   $8,     $24                \n"     /* PADDR_EXT <= $8 */	
	:"=r" (val): "r"(acid), "r" (addr): "$8");		

	return val;
}

static inline bool_t cpu_remote_atomic_cas(void *ptr, uint_t cid, 
					sint_t old, sint_t new)
{
	uint_t acid;
	bool_t isAtomic;

	acid = arch_clst_arch_cid(cid);
  
	__asm__ volatile
		(
		"mfc2   $9,	$24                \n"     /* $9 <= PADDR_EXT */   
		"mtc2   %4,     $24                \n"     /* PADDR_EXT <= acid */   

		/* CAS */
		".set noreorder	                    \n"
		"sync                               \n"
		"or      $8,      $0,       %3      \n"
		"ll      $3,      (%1)              \n"
		"bne     $3,      %2,       1f      \n"
		"li      $7,      0                 \n"
		"sc      $8,      (%1)              \n"
		"or      $7,      $8,       $0      \n"
		"sync                               \n"
		".set reorder                       \n"
		"1:                                 \n"
		"or      %0,      $7,       $0      \n"
		/*END CAS*/

		"mtc2   $9,     $24                 \n"    /* restore PADDR_EXT */

		: "=&r" (isAtomic)
		: "r" (ptr), "r" (old) , "r" (new), "r" (acid) 
		: "$3", "$7", "$8", "$9");

	return isAtomic;
}



static inline void cpu_remote_memcpy(void* dest, uint_t dcid, 
				void* src, uint_t scid, size_t size)
{
	int i;
	size_t wsize;
	uint_t sacid;
	uint_t dacid;

	sacid = arch_clst_arch_cid(scid);
	dacid = arch_clst_arch_cid(dcid);



	if(((uintptr_t)dest & 0x3) || ((uintptr_t)src & 0x3))
	{
		wsize = 0;//do it all in bytes
	}else
		wsize = size >> 2;

	//optimize: do as in memcpy
	for(i=0; i< wsize; i++)
	{
		__asm__ volatile(					
		"mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT */	
		"mtc2   %0,     $24                \n"     /* PADDR_EXT <= sacid */	
		"lw 	$9,	0(%1)		   \n"					
		"mtc2   %2,     $24                \n"     /* PADDR_EXT <= dacid */	
		"sw 	$9,	0(%3)		   \n"					
		"mtc2   $8,     $24                \n"     /* PADDR_EXT <= $8 */	
		:: 
		"r"(sacid), "r" (src+(i<<2)), 
		"r"(dacid), "r" (dest+(i<<2))
		: 
		"$8", "$9");		
	}

	for(i=wsize << 2; i< size; i++)
	{
		//FIXME:wrap theses assemblies in a macro ?
		__asm__ volatile(					
		"mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT */	
		"mtc2   %0,     $24                \n"     /* PADDR_EXT <= sacid */	
		"lb 	$9,	0(%1)		   \n"					
		"mtc2   %2,     $24                \n"     /* PADDR_EXT <= dacid */	
		"sb 	$9,	0(%3)		   \n"					
		"mtc2   $8,     $24                \n"     /* PADDR_EXT <= $8 */	
		:: 
		"r"(sacid), "r" (src+i), 
		"r"(dacid), "r" (dest+i)
		: 
		"$8", "$9");		
	}
}

#endif	/* _CPU_ASM_H_ */
