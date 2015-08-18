#include <cpu-regs.h>
#define _ALMOS_ASM_
#include <boot-info.h>

#FIXME: can't we make it in C
	.section  .kheader, "ax",@progbits
	
	.extern __ktext_start
	.extern __entry_end
	.extern __ktext_end
	.extern __kdata_start
	.extern __kdata_end
	.extern __virtual_end
	.extern kern_init
	.ent kheader
	.global kheader

#if 0
typedef struct kernel_info_s
{
	uint_t signature;/*1*/
	uint_t text_start;/*2*/
	uint_t text_end;/*3*/
	uint_t data_start;/*4*/
	uint_t data_end;/*5*/
	uint_t virtual_end;/*6*/
	uint_t kern_init;/*7*/
}kernel_info_t;
#endif

kheader:	
	header:	.word KERNEL_SIGNATURE, __ktext_start, __ktext_end, __kdata_start, __kdata_end, __virtual_end, kern_init
	
	.end kheader
