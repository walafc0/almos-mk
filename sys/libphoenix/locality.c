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

#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include "locality.h"
#include "stddefines.h"
#include "processor.h"

static int num_cluster = 0;

/* Retrieve the number of processors that belong to the locality
   group of the calling LWP. */
int loc_get_lgrp_size ()
{
    /* XXX smarter implementation? have all cpus local to this thread */
    return proc_get_num_cpus () / loc_get_num_lgrps();
}

/* Retrieve the number of total locality groups on system. */
int loc_get_num_lgrps ()
{
  if(num_cluster == 0)
  {
    num_cluster = sysconf(_SC_NCLUSTERS_ONLN);
   
    if(num_cluster < 1)
      num_cluster = 1;
  }

  return num_cluster;
}

/* Retrieve the locality group of the calling LWP. */
int loc_get_lgrp ()
{
    int cpuid = proc_get_cpuid();

    assert(cpuid != -1);

    if (cpuid == -1)
       return 0;
    else
       return cpuid % loc_get_num_lgrps();
}

/* Retrieve the locality group of the physical memory that backs
   the virtual address ADDR. */
int loc_mem_to_lgrp (void *addr)
{
  minfo_t info;
  int err;

  err = mcntl(MCNTL_READ, addr, 1, &info);

  assert(err == 0);

  if(err != 0) return 0;

  return info.mi_cid;
}
