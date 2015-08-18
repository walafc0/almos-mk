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
  
   UPMC / LIP6 / SOC (c) 2007
*/

#include <types.h>
#include <thread.h>
//#include <cpu.h>
#include <atomic.h>

/* PRNG parameters. Best results if:
 * PRNG_C and PRNG_M are relatively prime
 * PRNG_A-1 is divisible by all prime factors of PRNG_M
 * PRNG_C-1 is a muliple of 4 if PRNG_M is a multiple of 4
 *
 * PRNG_M is 2^32 and is not declared since we use an uint32_t,
 * so the modulus is implicit.
 */

#if 0
/* PRNG_A is a prime number */
static uint32_t PRNG_A = 65519;

/* PRNG_A is a multiple of 4 because
 * PRNG_M a multiple of 4. 64037
 * is some random prime number. */
static uint32_t PRNG_C = 64037 & 0xFFFFFFFB;

static atomic_t last_num;
#endif

/**
 * Pseudo random number generator: initialization
 */
void srand(unsigned int seed)
{
	current_cpu->last_num = seed;
	//atomic_init(&last_num,seed);
}

uint_t rand(void)
{  
#if 0
	uint_t old, new;
	bool_t isAtomic = false;
	uint_t count    = 1000;
#endif
	struct cpu_s *cpu;

	cpu = current_cpu;
	cpu->last_num = ((cpu->last_num * cpu->prng_A) + cpu->prng_C) ^ (cpu_time_stamp() & 0xFFF);
#if 0
	while((count > 0) && (isAtomic == false))
	{
		old = atomic_get(&last_num);
		new = ((old * PRNG_A) + PRNG_C);

		isAtomic = atomic_cas(&last_num, old, new);
		count --;
	}
#endif

	return cpu->last_num;
}

