//////////////////////////////////////////
//       ROM mapped segments
//////////////////////////////////////////
//
//!!!!!!!!!!!!!!! TO ENABLE BOOT ON AN OTHER CPU THAN PROC 0, PRELOADER MUST GUARANTEE :
// - change of bootstrap_cpu value in arch_info (or load an other arch_info)
// - add to the the following addresses an offset of x_boot|y_boot<<32 before loading the code/data from the disk
// - In order to enable the condition 2 and to enable physical addressing in boot-loader : mtc2 EXT_BOOT_CLUSTER_ADDR before jumping to boot_entry


/* FG : imposed by hard design : we assumed in boot_loader code that 
* preloader was put from the first address in a ram of a cluster */
#define PRELOADER_RESERVED_START 0x00000000
/* FG : Field to be set at 0 if preloader is not in the boot_cluster ram */
#define PRELOADER_RESERVED_END   0x00100000

/* BOOT_BASE Must be >= PRELOADER_RESERVED_END */
#define BOOT_BASE                (PRELOADER_RESERVED_END) //TODO Modify info2bib too

//////////////////////////////////////////
