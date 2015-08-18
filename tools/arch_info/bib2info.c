/*
    This file is part of AlmOS.

    AlmOS is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    AlmOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AlmOS; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

    UPMC / LIP6 / SOC (c) 2010
    Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include "arch-bib.h"
#include "devdb.h"
#include "params.h"

static char const rcsid[]="$Id: bib2info,v1.2 Ghassan Almaless $";

typedef struct arch_bib_device_s dev_info_t;
typedef struct arch_bib_cluster_s cluster_info_t;
typedef struct arch_bib_header_s header_info_t;

int verbose;

static int bib2info(char *input_name, char *output_name);
static void print_auto_msg(FILE *output, char *src_name, char *dst_name);

int main(int argc, char *argv[])
{
  int retval;
  int hasInput;
  int hasOutput;
  int c;
  char *src_name;
  char *dst_name;

  hasInput = 0;
  hasOutput = 0;
  retval = 0;
  verbose = 0;
  src_name = NULL;
  dst_name = NULL;

  while ((c = getopt(argc, argv, "vVi:I:o:O:")) != -1)
  {
    switch(c) 
    {
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
    case 'v':
    case 'V':
      verbose = 1;
    break;
    default:
      fprintf(stderr, "%s: invalid option %c\n", argv[0], c);
    }
  }

  if(!hasInput || !hasOutput)
  {
    fprintf(stderr, "\nUsage: %s -iInputFile.bib -oOutpuFile.info\n\n", argv[0]);
    retval= EARGS;
  }
  else
    retval = bib2info(src_name, dst_name);

  free(src_name);
  free(dst_name);
  return retval;
}


static int bib2info(char *input_name, char *output_name)
{
  int retval;
  int i, j;
  char *devid;
  FILE *fsrc;
  FILE *fdst;
  dev_info_t *dev_tbl;
  cluster_info_t *clusters_tbl;
  header_info_t header;
  int clstr_nr;
  char * arch_bib_signature = "@ALMOS ARCH BIB";
  
  dev_tbl = NULL;
  clusters_tbl = NULL;
  
  if((fsrc=fopen(input_name, "r")) == NULL)
  {
    fprintf(stderr, "Error: Opening %s: %s\n", input_name, (char*)strerror(errno));
    return ESRCOPEN;
  }
  
  if((fdst=fopen(output_name, "w+")) == NULL)
  {
    fprintf(stderr, "Error: Opening %s: %s\n", input_name, (char*)strerror(errno));
    fclose(fsrc);
    return EDSTOPEN;
  }

  print_auto_msg(fdst, input_name, output_name);
  memset(&header, 0, sizeof(header));

  if((retval=fread(&header, sizeof(header), 1, fsrc)) != 1)
  {
    fprintf(stderr, "Faild to read clusters tbl from input file [retval %d]\n", retval);
    retval = EREADINFO;
    goto BIN2INFO_ERROR;
  }

  if(strncmp(&header.signature[0], arch_bib_signature, strlen(arch_bib_signature)))
  {
    fprintf(stderr, "Input file (%s) is not a Boot Information Block\n", input_name);
    retval = ESIGNATURE;
    goto BIN2INFO_ERROR;
  }

  fprintf(fdst, "\n[HEADER]\n\tREVISION=%d\n\tARCH=%s\n\tXMAX=%d\n\tYMAX=%d\n\tCPU_NR=%d\n\tBSCPU=%d\n\tBSTTY=0x%llx\n\tBSDMA=0x%llx\n",
	  header.revision, 
	  &header.arch[0],
	  header.x_max,
	  header.y_max,
	  header.cpu_nr,
	  header.bootstrap_cpu,
	  (long long unsigned int)header.bootstrap_tty,
	  (long long unsigned int)header.bootstrap_dma);
  
  clstr_nr = header.x_max * header.y_max;

  if((clusters_tbl = (cluster_info_t*) malloc(clstr_nr * sizeof(cluster_info_t))) == NULL)
  {
    fprintf(stderr, "Faild to allocate cluster info table\n");
    retval = ENOMEM;
    goto BIN2INFO_ERROR;
  }

  if((retval=fread(clusters_tbl, sizeof(cluster_info_t), clstr_nr, fsrc)) != clstr_nr)
  {
    fprintf(stderr, "Faild to read clusters tbl from input file [retval %d]\n", retval);
    retval = EREADINFO;
    goto BIN2INFO_ERROR;
  }

  for(i=0; i < clstr_nr; i++)
  {
    fprintf(fdst, "\n[CLUSTER]\n\tCID=%d\n\tCPU_NR=%d\n\tDEV_NR=%d\n",
	    clusters_tbl[i].cid, 
	    clusters_tbl[i].cpu_nr, 
	    clusters_tbl[i].dev_nr);
    
    log_msg("Cluster %d: CPU_NR %d, DEV_NR %d, DEV_OFFSET %d\n", 
	    clusters_tbl[i].cid, clusters_tbl[i].cpu_nr, clusters_tbl[i].dev_nr, clusters_tbl[i].offset);
    
    if((dev_tbl = (dev_info_t*) malloc(clusters_tbl[i].dev_nr * sizeof(dev_info_t))) == NULL)
    {
      fprintf(stderr, "Faild to allocate dev info for cluster %d\n",i);
      retval = ENOMEM;
      goto BIN2INFO_ERROR;
    }
    
    if((retval=fseek(fsrc, clusters_tbl[i].offset, SEEK_SET)))
    {
      fprintf(stderr, "Faild to seek to offset %d into input file [cluster %d]\n", clusters_tbl[i].offset, i);
      retval = ESEEKINFO;
      goto BIN2INFO_ERROR;
    }
    
    if((retval=fread(dev_tbl, sizeof(dev_info_t), clusters_tbl[i].dev_nr, fsrc)) != (int)clusters_tbl[i].dev_nr)
    {
      fprintf(stderr, "Faild to read devices tbl from input file [retval %d, cluster %d]\n", retval, i);
      retval = EREADINFO;
      goto BIN2INFO_ERROR;
    }

    for(j=0; j < (int)clusters_tbl[i].dev_nr; j++)
    {
      if((devid = dev_locate_by_id(dev_tbl[j].id)) == NULL)
      {
	fprintf(stderr, "Unknown Device [cluster %d, device %d]\n", i, j);
	retval = EDEVID;
	goto BIN2INFO_ERROR;
      }
      
      fprintf(fdst, "\t\tDEVID=%s\tBASE=0x%llx\tSIZE=0x%x\tIRQ=%d\n",
	      devid,
	      (long long unsigned int)(dev_tbl[j].base),
	      dev_tbl[j].size,
	      dev_tbl[j].irq);
	      
      log_msg("Base 0x%llx, Size 0x%x, irq %d\n",(long long unsigned int)dev_tbl[j].base,dev_tbl[j].size,dev_tbl[j].irq);
    }

    free(dev_tbl);
    dev_tbl = NULL;
  }

  retval = (&rcsid[0] == NULL);

  log_msg("Header:\n\tReserved <0x%llx - 0x%llx>\n\tSignature: %s\n\tRevision: %d\n",
	  (long long unsigned int)header.rsrvd_start, 
	  (long long unsigned int)header.rsrvd_limit,
	  &header.signature[0], 
	  header.revision);

  log_msg("\tArch: %s\n\tSize: %d\n\tXMAX: %d\n\tYMAX: %d\n\tONLN Clsuter NR: %d\n\tONLN CPU NR: %d\n", 
	  &header.arch[0], 
	  header.size,
	  header.x_max,
	  header.y_max,
	  header.onln_clstr_nr, 
	  header.onln_cpu_nr);

  log_msg("\tBSCPU: %d\n\tBSTTY: 0x%llx\n\tBSDMA: 0x%llx\n", 
	  header.bootstrap_cpu, 
	  (long long unsigned int)header.bootstrap_tty, 
	  (long long unsigned int) header.bootstrap_dma);
  
 BIN2INFO_ERROR:
  free(clusters_tbl);
  free(dev_tbl);
  fclose(fsrc);
  fclose(fdst);
  return 0;
}

static void print_auto_msg(FILE *output, char *src_name, char *dst_name)
{
  char cmd[128];
  long offset;
  int count;

  fprintf(output,"# Boot Information Block To Hardware Information Format\n\n");
  fprintf(output,"# This file is autogenerated by the command: bib2info -i %s -o %s\n",src_name, dst_name);
  fprintf(output,"# It revers the binary format and builds the hardware platfrom description\n");
  fprintf(output,"# This file can be used to rebuild the corrsponding Boot Information Block\n\n");

  fflush(output);
  fflush(output);
  
  offset = ftell(output);
  count = sprintf(&cmd[0], "echo $USER $(date) >> %s; echo >> %s; echo >> %s", dst_name, dst_name, dst_name);

  system(&cmd[0]);
  offset += (count - 28);
  fseek(output, offset, SEEK_SET);
}
