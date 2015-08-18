#include <pthread.h>
#include <cpu-syscall.h>

int *__errno_location(void) __attribute__((weak));
int *__errno_location()
{
  __pthread_tls_t *tls;
  tls = cpu_get_tls();
  return (int*)__pthread_tls_getlocation(tls,__PT_TLS_ERRNO);
}

