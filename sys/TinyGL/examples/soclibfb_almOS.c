/*
 * Demonstration program for SOCLIB FB graphics on ALMOS.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <GL/gl.h> 
#include <GL/oscontext.h> 
#include "ui.h"

void tkSwapBuffers(void)
{
    nglXSwapBuffers(w1);
}

static ostgl_context *ctx;

int ui_loop(int argc, char **argv, const char *name)
{
  void *dsiplay;
  size_t xsize;
  size_t ysize;
  size_t mode;
  size_t pgsize;
  size_t size;
  clock_t tm_tmp;
  clock_t tm_now;
  int fd;
  
  pgsize = sysconf(_SC_PAGE_SIZE);

  if(pgsize < 0)
  {
    fprintf(stderr, "ERROR: sysconf pgsize got %u\n", pgsize);
    exit(1);
  }

  int fd = open("/ETC/FBRC", O_RDONLY, 420);

  if(fd < 0)
  {
    fprintf(stderr, "ERROR: cannot read /etc/fbrc configuration file\n");
    exit(2);
  }

  xsize = 0;
  ysize = 0;

  fscanf(fd, "XSIZE=%u\n", &xsize);
  fscanf(fd, "YSIZE=%u\n", &ysize);
  fscanf(fd, "MODE=%u\n", &mode);

  close(fd);
  
  if((xsize == 0) || (ysize == 0) || (mode != 16))
  {
    fprintf(stderr, "ERROR: dont like these info: xsize %d, ysize %d, mode %d\n", 
	    xsize,
	    ysize,
	    mode);

    exit(3);
  }
  
  size = xsize * ysize * 2;

  size = (size & (pgsize - 1)) ? (size & ~(pgsize - 1)) + pgsize : pgsize;

  if((fd = open("/DEV/FB0",O_WRONLY,420)) == -1)
  {
    fprintf(stderr, "ERROR: cannot open /dev/fb0\n");
    exit(4);
  }

  display = mmap(NULL, 
		 size,
		 PROT_WRITE | PROT_READ, 
		 MAP_SHARED | MAP_FILE, 
		 fd, 0);
  
  close(fd);

  if(display == MAP_FAILED) 
  {
    perror("mmap dest file");
    exit(5);
  }
  
  ctx = ostgl_create_context(xsize, ysize, 16, &display, 1);

  if(ctx == NULL)
  {
    fprintf(stderr, "ERROR: failed to create ostgl ctx\n");
    exit(6);
  }
  
  if(ostgl_make_current(ctx,0) != 0)
  {
    fprintf(stderr, "ERROR: failed to make current ctx\n");
    exit(7);
  }
  
  fprintf(stderr, "Initializing scene ..");
  tm_tmp = clock();
  init();
  tm_now = clock();
  fprintf(stderr, " done [%llu]\n", tm_now - tm_tmp);

  fprintf(stderr, "Shape scene ..");
  tm_tmp = clock();
  reshape(xsize,ysize);
  tm_now = clock();
  fprintf(stderr, " done [%llu]\n", tm_now - tm_tmp);

  while (1) 
  {
    idle();
    fprintf(stderr, "Time now %llu\n", clock());
  }
    
  return 0;
}
