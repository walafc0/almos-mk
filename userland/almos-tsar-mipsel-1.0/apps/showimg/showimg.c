/*
  This file is part of ALMOS.
  
  ALMOS is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  ALMOS is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with ALMOS; if not, write to the Free Software Foundation,
  Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
  UPMC / LIP6 / SOC (c) 2009
  Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include "sgiImg.h"

enum
{
	ECPUNR   = 1,
	EARGS,
	EBUFFER,
	ESRCOPEN,
	EDSTOPEN,
	ETMPOPEN,
	ESEEKINFO,
	EREADINFO,
	ESEEKSCREEN
};

#define THREAD_MAX_NR 128
#define BUFFER_SIZE   3*256

void* worker_thread(void *arg);

static char* buffer   = NULL;
static char *src_name = NULL;
static char *dst_name = NULL;
static uint16_t x_size = 0;
static uint16_t y_size = 0;
static int cpu_nr = 1;
static void *fb;
static void *img;
static int isStatic = 0;

int main(int argc, char *argv[])
{
	struct sgi_info_s *sgi_info;
	int err,c;
	uint32_t i;
	ssize_t size;
	int src_fd;
	int dst_fd;
	int tmp_fd;
	uint16_t z_size;
	pthread_t th[THREAD_MAX_NR];
	pthread_attr_t attr;
	uint32_t state;
	clock_t tm_start;		/* start of command */
	clock_t tm_header;		/* image's header paressing */
	clock_t tm_launch;		/* Threads launching*/
	clock_t tm_end;		        /* end of command */
	clock_t tm_tmp;
	int hasInput, hasOutput;

	tm_start = clock();
	hasInput = 0;
	hasOutput = 0;

	while ((c = getopt(argc, argv, "p:P:i:I:o:O:sS")) != -1)
	{
		switch(c) 
		{
		case 'P':
		case 'p':
			cpu_nr = atoi(optarg); 
			if ((cpu_nr < 1) || (cpu_nr > THREAD_MAX_NR)) 
			{
				fprintf(stderr, "P must be in [1 .. 128]\n");
				return -ECPUNR;
			}
			break;  
		case 'I':
		case 'i':
			src_name = strdup(optarg);
			hasInput = 1;
			break;
		case 'o':
		case 'O':
			dst_name = strdup(optarg);
			hasOutput = 1;
			break;
		case 'S':
		case 's':
			isStatic = 1;
			break;
		default:
			fprintf(stderr, "%s: invalid option %c\n", argv[0], c);
			fprintf(stderr, "Usage: >%s -pP -iInputFile.rgb -oOutpuFile.dat\n", argv[0]);
			return -EARGS;
		}
	}

	if(!hasInput || !hasOutput)
	{
		if(!hasInput)
			fprintf(stderr,"Inputfile is required\n");

		if(!hasOutput)
			fprintf(stderr,"Outputfile is required\n");

		fprintf(stderr, "Usage: %s -pP -iInputFile.rgb -oOutpuFile.dat\n", argv[0]);
		return -EARGS;
	}

	printf("Image Source      : %s\n", src_name);
	printf("Destination file  : %s\n", dst_name);
	printf("Processors Number : %d\n", cpu_nr); 

	if((buffer = calloc(1,BUFFER_SIZE)) == NULL)
	{
		fprintf(stderr, "Error: Faild to allocate inital buffer\n");
		return -EBUFFER;
	}

	err = 0;
  
	if((dst_fd = open(dst_name,O_WRONLY | O_CREAT,420)) == -1)
	{
		perror("error while opening dst_name");
		return -EDSTOPEN;
	} 

	if((src_fd = open(src_name,O_RDONLY,0)) == -1){
		perror("error while opening file");
		printf("error while openig %s\n",src_name);
		close(dst_fd);
		return -ESRCOPEN;
	}
  
	if((size = read(src_fd,buffer,sizeof(*sgi_info))) < 0)
	{
		perror("error while reading file");
		err = -EREADINFO;
		goto SHOWIMG_ERR;
	}

	sgi_info = (struct sgi_info_s*) buffer;
  
	x_size = SGI_TO_LE2(sgi_info->x_size);
	y_size = SGI_TO_LE2(sgi_info->y_size);
	z_size = SGI_TO_LE2(sgi_info->z_size);
  
	if(z_size != 1)
	{
		fprintf(stderr, "Unsupported SGI image, %dx%d with Channels number %d\n", x_size, y_size, z_size);
		goto SHOWIMG_ERR;
	}

	printf("Image Dimension   : XSIZE %d, YSIZE %d\n\n", x_size, y_size);

	fb = mmap(NULL, x_size * y_size * 2, PROT_WRITE, MAP_PRIVATE, dst_fd, 0);
  
	if(fb == MAP_FAILED) 
	{
		err = -errno;
		perror("Error while mapping dest file");
		goto SHOWIMG_ERR;
	}

	img = mmap(NULL, x_size * y_size, PROT_READ, MAP_PRIVATE, src_fd, 0);

	if(img == MAP_FAILED) 
	{
		err = -errno;
		perror("Error while mapping src file");
		goto SHOWIMG_ERR;
	}

	img = (uint8_t*)img + sizeof(struct sgi_info_s);

	tm_tmp = clock();
	tm_header = tm_tmp - tm_start;

	if((err=pthread_attr_init (&attr)) != 0)
	{
		fprintf (stderr, "Error initialization of thread's attribute (err %d)\n",err);
		pthread_exit ((void *) EXIT_FAILURE);
	}

	for(i = 0; i < cpu_nr; i++)
	{ 

#ifdef _ALMOS_
		if(isStatic)
			pthread_attr_setcpuid_np (&attr, i % cpu_nr, NULL);
#endif

		if((err = pthread_create (&th[i], &attr, &worker_thread, (void *) i)) != 0)
		{
			fprintf (stderr, "Error creating thread, Error code :%d\n",err);
			pthread_attr_destroy(&attr);
			pthread_exit ((void *) EXIT_FAILURE);
		}

		printf("Thread %d <=> %x has been created\n", i, (uint32_t)th[i]);
	}

	printf("Waiting end of all threads\n");
  
	tm_launch = clock() - tm_tmp;

	for (i = 0; i < cpu_nr; i++)
	{
		if ((err = pthread_join (th[i], (void **) &state)) != 0)
		{
			fprintf (stderr, "Error, faild to join on thread %d , with this error code: %d\n", (uint32_t)th[i], err);
			pthread_exit ((void *) EXIT_FAILURE);
		}

		printf("Thread #%x has finished, elapsed time: %u\n", (uint32_t)th[i], state);
	}
  
	tm_end = clock();

	printf("\n\nCommand Statistics\nStart Time          : %g\n"
	       "End Time            : %g\nElapsed Time        : %g\n"
	       "Header Time         : %g\nThreads Launch Time : %g\n",
	       (double)tm_start,
	       (double)tm_end,
	       (double)tm_end - tm_start,
	       (double)tm_header,
	       (double)tm_launch);
  
	return 0;

SHOWIMG_ERR:
	close(dst_fd);
	close(src_fd);
	free(src_name);
	free(dst_name);
	free(buffer);
	pthread_attr_destroy(&attr);
	return err;
}


void* worker_thread(void *arg)
{
	uint32_t id = (uint32_t) arg;
	size_t size;
	uint32_t width;
	uint32_t higth;
	int8_t buff[x_size];
	clock_t tm_start;
	clock_t tm_elapsed;
	uint8_t *screen;
	uint8_t *image;

	tm_start = clock();
	width = x_size;
	higth = y_size / cpu_nr;
	screen = (uint8_t*)((uintptr_t)fb + id * higth * width);
	image = (uint8_t*)((uintptr_t)img + id * higth * width);
  
	while(higth--)
	{
		memcpy(screen, image, width);
		screen += width;
		image += width;
		if((id == (cpu_nr-1)) && (higth == 1))
			break;
	}

	tm_elapsed = clock() - tm_start;

	pthread_exit((void*)((size_t)tm_elapsed));
}
