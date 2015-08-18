/*
 * mmu-info.c - TSAR MMU exceptions related informations
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

#define _USE_MMU_INFO_H_
#include <mmu-info.h>
#include <bits.h>

#define MMU_UNKNOWN_ERR  0x0000

static mmu_except_info_t mmu_except_db[] = 
{{0x0001, "MMU_WRITE_PT1_UNMAPPED", "Write access : Page fault on Table 1 (invalid PTE)", "Non Fatal Error"},
 {0x0002, "MMU_WRITE_PT2_UNMAPPED", "Write access : Page fault on Table 2 (invalid PTE)", "Non Fatal Error"},
 {0x0004, "MMU_WRITE_PRIVILEGE_VIOLATION", "Write access : Protected access in user mode", "User Error"},
 {0x0008, "MMU_WRITE_ACCES_VIOLATION","Write access : Write to a non writable page", "User error"},
 {0x0020, "MMU_WRITE_UNDEFINED_XTN","Write access : Undefined external access address","User error"},
 {0x0040, "MMU_WRITE_PT1_ILLEGAL_ACCESS","Write access : Bus Error in Table1 access","Kernel error"},
 {0x0080, "MMU_WRITE_PT2_ILLEGAL_ACCESS","Write access : Bus Error in Table2 access","Kernel error"},
 {0x0100, "MMU_WRITE_DATA_ILLEGAL_ACCESS","Write access : Bus Error during the cache access","Kernel error"},
 {0x1001, "MMU_READ_PT1_UNMAPPED","Read access : Page fault on Table1 (invalid PTE)","non fatal error"},
 {0x1002, "MMU_READ_PT2_UNMAPPED","Read access : Page fault on Table 2 (invalid PTE)","non fatal error"},
 {0x1004, "MMU_READ_PRIVILEGE_VIOLATION","Read access : Protected access in user mode","User error"},
 {0x1010, "MMU_READ_EXEC_VIOLATION","Read access : Exec access to a non exec page","User error"},
 {0x1020, "MMU_READ_UNDEFINED_XTN","Read access : Undefined external access address","User error"},
 {0x1040, "MMU_READ_PT1_ILLEGAL_ACCESS","Read access : Bus Error in Table1 access","Kernel error"},
 {0x1080, "MMU_READ_PT2_ILLEGAL_ACCESS","Read access : Bus Error in Table2 access","Kernel error"},
 {0x1100, "MMU_READ_DATA_ILLEGAL_ACCESS","Read access : Bus Error during the cache access","Kernel error"},
 {MMU_UNKNOWN_ERR, "UNKNOWN", "Exception Detected but no MMU Error code has been found", "Disfunction of MMU's Error Report"}}; 



inline mmu_except_info_t* mmu_except_get_entry(uint_t mmu_err_val)
{
	uint_t entry = 0;

	while(mmu_except_db[entry].err != MMU_UNKNOWN_ERR)
	{
		if(mmu_except_db[entry].err == mmu_err_val)
			break;

		entry ++;
	}
  
	return &mmu_except_db[entry];
}
