/*
   This file is part of ALMOS-SYS.
  
   ALMOS-SYS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   ALMOS-SYS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with ALMOS-SYS; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <stdlib.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <pthread.h>
#include <gomp/omp-pv.h>

char **environ;

extern int main();
extern void __heap_manager_init(void);

extern unsigned int __bss_start;
extern unsigned int __bss_end;

static void __sigreturn(void)
{
  cpu_syscall(NULL,NULL,NULL,NULL,SYS_SIGRETURN);
}

void __attribute__ ((section (".init"))) _start(char **argv, char **envp)
{
  int retval;
  int argc;
  __pthread_tls_t tls;
  struct __shared_s __default;
  unsigned int *bss_end = &__bss_end;
  unsigned int *bss_start = &__bss_start;
 
  CPU_SET_GP(bss_start);

  environ = envp;

  /* First of all, init tls */
  __pthread_tls_init(&tls);
  __pthread_tls_set(&tls,__PT_TLS_SHARED,&__default);
  __default.mailbox = 0;
  __default.tid = tls.attr.key;
  /* ---------------------- */
  __heap_manager_init();
  __pthread_init();

#ifdef HAVE_GOMP
  initialize_env();
  initialize_team();
  initialize_critical();
#endif
  
  for(argc=0; argv[argc] != NULL; argc++)
  {
#if 0
    printf("LibC: argc %d, argv[%d]=%s\n", argc, argc, argv[argc]);
    fflush(stdout);
#endif
  }

  cpu_syscall(&__sigreturn,NULL,NULL,NULL, SYS_SET_SIGRETURN);
  //pthread_exit((void *)main());
  retval = main(argc, argv, envp);

  fprintf(stderr, "LibC: main ended\n");

#ifdef HAVE_GOMP
  fprintf(stderr, "LibC: calling gomp team_destructor\n");
  team_destructor();
#endif

  exit(retval);
}
