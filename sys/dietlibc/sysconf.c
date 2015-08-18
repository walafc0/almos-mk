#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

#if CONFIG_LIBC_SYSCONF_DEBUG
#include <pthread.h>
#include <stdio.h>
#endif

static long sysconf_get(char *token) {
  int fd=open("/sys/sysconf",O_RDONLY, 0);
  char buf[512];
  size_t l;
  if (fd==-1) { errno=ENOSYS; return -1; }
  l=read(fd,buf,sizeof(buf));

#if CONFIG_LIBC_SYSCONF_DEBUG
  int cpu_id;
  pthread_attr_getcpuid_np(&cpu_id);
  
  if(cpu_id == 15)
    printf("%s: cpu %d: got %d [%s]\n", __FUNCTION__, cpu_id, l, &buf[0]);
#endif

  if (l!=(size_t)-1) {
    char* c;
    buf[l]=0;
    c=strstr(buf,token);
    if (c) {
      c+=strlen(token); while (*c==' ' || *c=='\t') ++c;
      l=0;
      while (*c>='0' && *c<='9') {
	l=l*10+*c-'0';
	++c;
      }
    }
  }
  close(fd);
  return l;
}

long sysconf(int name)
{
  switch(name)
  {
  case _SC_OPEN_MAX:
    return OPEN_MAX;

  case _SC_CLK_TCK:
#ifdef __alpha__
    return 1024;
#else
    return 100;
#endif

  case _SC_PAGESIZE:
    return sysconf_get("PAGE_SIZE");

  case _SC_ARG_MAX:
    return ARG_MAX;

  case _SC_NGROUPS_MAX:
    return NGROUPS_MAX;

  case _SC_NPROCESSORS_ONLN:
    return sysconf_get("CPU_ONLN_NR");

  case _SC_PAGE_SIZE:
    return sysconf_get("PAGE_SIZE");

  case _SC_CACHE_LINE_SIZE:
    return sysconf_get("CPU_CACHE_LINE_SIZE");

  case _SC_HEAP_MAX_SIZE:
    return sysconf_get("HEAP_MAX_SIZE");

  case _SC_NCLUSTERS_ONLN:
    return sysconf_get("CLUSTER_ONLN_NR");
  }
  errno=ENOSYS;
  return -1;
}
