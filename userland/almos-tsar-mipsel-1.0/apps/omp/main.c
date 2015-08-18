#include <omp.h>
#include <stdio.h>

main ()  {

int   i, n, chunk;
float a[2*1024], b[2*1024], result;

/* Some initializations */
n = 2*1024;
chunk = 512;
result = 0.0;
for (i=0; i < n; i++)
  {
  a[i] = i * 1.0;
  b[i] = i * 2.0;
  }

#pragma omp parallel for      \  
  default(shared) private(i)  \  
  schedule(static,chunk)      \  
  reduction(+:result)

  for (i=0; i < n; i++)
    result = result + (a[i] * b[i]);

printf("Final result= %f\n",result);

}
