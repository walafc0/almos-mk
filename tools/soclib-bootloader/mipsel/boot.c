/*
   This file is part of AlmOS.
  
   AlmOS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   AlmOS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with AlmOS; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

/*
 * Modifications :
 *    - il n'est plus nécessaire de mettre en sommeil les différents processeurs, cela est réalisé par le preloader
 *    En effet, le processeur bootstrap exécute ce code avant les autres processeurs, pas d'exécution simultanée.
 *    - On a renommé la fonction pour éviter les conflits avec le preloader
 *    - On a supprimé les actions déjà réalisée par le preloader (initialisation status register ...)
 *    - On pourrait probablement faire l'économie du calcul du numero de processeur dans le cluster et du num de cluster, déjà fait dans reset.S(TODO)
 *    -le démasquage des irq ne sera plus à faire une fois passé à 40 bits (fait dans le preloader)
 *    -Rq : ce fichier constitue le point d'entrée du boot_loader
 *    -Rq : le champ CONFIG_BOOT_SIGNAL_SIGNATURE est a priori inutile désormais (config.h, )
 */


///////////////////////////////////////////
//            SYSTEM STARTUP             //
///////////////////////////////////////////

#include <config.h>
#include <mips32_registers.h>
#include <segmentation.h>

#define TO_STR(s) str(s)
#define str(s) #s
#define BSZ CONFIG_BOOT_STACK_SIZE

void __attribute__ ((section(".boot"))) boot_cluster_reset(void)
{
  __asm__ volatile (
	".set noreorder                    \n"
	"mtc0   $0,    $12                 \n"                // init status register : IRQ disabled
	//load global varibales
	"or     $20,   $0,    %0           \n"
	//compute cpu_lid
	"mfc0   $8,    $15,   1		   \n"                // $8 <-> arch_cpu_id
	"andi   $8,    $8,    0xFFF        \n"                // mask all but arch_cpu_id 
	"lw     $9,    8($20)              \n"                // $9 <-> per_cluster_cpu_nr
	"divu   $8,    $9                  \n"                // arch_cpu_id/per_cluster_cpu_nr
	"mflo   $10                        \n"                // $10 <-> arch_cid
	"mfhi   $11                        \n"                // $11 <-> local proc id (cpu_lid)
	//get rsrvd_limit and set stack
	"lw     $12,   4($20)              \n"                // $12  <-> shared stack top (rsrvd_limit)
	"li     $13,   ("TO_STR(BSZ)")     \n"                // stack_size == PMM_PAGE_SIZE
	"multu  $11,   $13                 \n"                // local_proc_id * stack_size
	"mflo   $14                        \n"                // $14 <-> local_proc_id * stack_size
	"subu   $29,   $12,     $14        \n"                // $29 <-> shared_stack_top - (local_proc_id * stack_size)
	//jump to boot_entry
	"or     $4,    $0,    $8           \n"                // $4 <-> arch_cpu_id (argument)
	"la     $31,   boot_entry          \n"	              // $31 <-> boot_entry
	//"1: blez   $0,    1b               \n"	              // infinite loop
	"jr     $31                        \n"                // BSP: goto boot_entry function args : cpu_arch_id
	"subu   $29,   $29,    4           \n"
	".set reorder                      \n" 
	::"r" (arch_bib_base) //base address of arch_bib
	); 
}



/* assume the same number of proc per cluster and the same amount of mem */
void __attribute__ ((section(".boot"))) other_proc_reset(void)
{
  __asm__ volatile (
	".set noreorder                    \n"
	"mtc0   $0,    $12                 \n"                // init status register : IRQ disabled
	//load global varibales
	"or     $20,   $0,    %0           \n"
	"or     $21,   $0,    %1           \n"
	//compute arch_cid & cpu_lid
	"mfc0   $8,    $15,   1		   \n"                // $8 <-> arch_cpu_id
	"andi   $8,    $8,    0xFFF        \n"                // mask all but arch_cpu_id 
	"lw     $9,    8($20)              \n"                // $9 <-> per_cluster_cpu_nr
	"divu   $8,    $9                  \n"                // arch_cpu_id/per_cluster_cpu_nr
	"mflo   $10                        \n"                // $10 <-> arch_cid
	"mfhi   $11                        \n"                // $11 <-> cpu_lid (local proc id)
	//get rsrvd_limit and set stack
	"lw     $12,   4($20)              \n"                // $12  <-> shared stack top (rsrvd_limit)
	//set stack
	"li     $13,   ("TO_STR(BSZ)")     \n"                // stack_size == PMM_PAGE_SIZE
	"multu  $11,   $13                 \n"                // local_proc_id * stack_size
	"mflo   $14                        \n"                // $14 <-> local_proc_id * stack_size
	"subu   $29,   $12,     $14        \n"                // $29 <-> shared_stack_top - (local_proc_id * stack_size)
	//set func and args 
	"or     $4,    $0,    $21           \n"                // $4 <-> boot_arch_cid 
	"or     $5,    $0,    $10           \n"                // $5 <-> arch_cid 
	"or     $6,    $0,    $11           \n"                // $6 <-> cpu_lid 
	"la     $31,  other_cluster_boot_entry \n"	               // $31 <->  other_cluster_boot_entry
	//all global varibales have been read,
	//we can switch to local dspace 
	"mtc2   $10,   $24                   \n"
	//jump to function
	"jr     $31                         \n"                
	"subu   $29,   $29,     12          \n"
	".set reorder                       \n" 
	::"r" (arch_bib_base), //base address of arch_bib
	  "r" (boot_arch_cid)
	); 
}

