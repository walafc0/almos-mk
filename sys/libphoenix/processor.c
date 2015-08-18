/* Copyright (c) 2007-2009, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the names of its 
*       contributors may be used to endorse or promote products derived from 
*       this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* OS specific headers and defines. */
#include <sched.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>

#include "processor.h"
#include "memory.h"
#include <pthread.h>

static int num_cpus = 0;

/* Query the number of CPUs online. */
int proc_get_num_cpus (void)
{
    char *num_proc_str;
    
    if(num_cpus == 0)
    {
      num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

      /* Check if the user specified a different number of processors. */
      if ((num_proc_str = getenv("MAPRED_NPROCESSORS")))
      {
        int temp = atoi(num_proc_str);
        if (temp < 1 || temp > num_cpus)
	  num_cpus = 0;
        else
	  num_cpus = temp;
      }
    }

    return num_cpus;
}

/* Bind the calling thread to run on CPU_ID. 
   Returns 0 if successful, -1 if failed. */
int proc_bind_thread (int cpu_id)
{
    return 0;
}

int proc_unbind_thread ()
{
   return 0;
}

/* Test whether processor CPU_ID is available. */
bool proc_is_available (int cpu_id)
{
  return true;
}

int proc_get_cpuid (void)
{
  int cpu_id = -1;
  int ret;
  
  ret = pthread_attr_getcpuid_np(&cpu_id);
  
  return (ret == 0) ? cpu_id : -1;
}
