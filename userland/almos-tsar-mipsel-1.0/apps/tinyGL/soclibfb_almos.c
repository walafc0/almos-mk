/*
 * File        : soclibfb_almos.c
 * Autor       : Ghassan Almaless
 * Contact     : first.last@lip6.fr
 * Copyright   : This code is in the public domain
 * Date        : 2011/02/10 (YYYY/MM/DD)
 * --------------------------------------------------------------------
 * Description : Demonstration program for SocLib FB graphics on ALMOS.
 * --------------------------------------------------------------------
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <GL/gl.h> 
#include <GL/oscontext.h> 
#include "ui.h"

static ostgl_context *ctx;

void tkSwapBuffers(void)
{
  ostgl_swap_buffers(ctx);
}


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
  FILE *fbrc;
  void *display;
  int err;

  pgsize = sysconf(_SC_PAGE_SIZE);

  if(pgsize < 0)
  {
    fprintf(stderr, "ERROR: sysconf pgsize got %u\n", pgsize);
    exit(1);
  }

  fbrc = fopen("/ETC/FBRC", "r");

  if(fbrc == NULL)
  {
    fprintf(stderr, "ERROR: cannot read /etc/fbrc configuration file\n");
    exit(2);
  }

  xsize = 0;
  ysize = 0;
  mode = 0;

  fscanf(fbrc, "\n[XSIZE]=%u\n", &xsize);
  fscanf(fbrc, "\n[YSIZE]=%u\n", &ysize);
  fscanf(fbrc, "\n[MODE]=%u\n", &mode);

  fclose(fbrc);
  
  if((xsize == 0) || (ysize == 0) || (mode != 16))
  {
    fprintf(stderr, "ERROR: dont like these info: xsize %d, ysize %d, mode %d\n", 
	    xsize,
	    ysize,
	    mode);

    exit(3);
  }
  
  size = xsize * ysize * 2;

  fprintf(stderr, "Frame Buffer diminsions: %dx%d mode %d\n", xsize, ysize, mode);

  size = (size & (pgsize - 1)) ? (size & ~(pgsize - 1)) + pgsize : size;

  if((fd = open("/DEV/FB0", O_RDONLY | O_WRONLY ,420)) == -1)
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
  
  ostgl_make_current(ctx,0);
  
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
    fprintf(stderr, "Time now %llu\n", (uint64_t)clock());
  }
    
  return 0;
}
