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
//FG : TODO : find a way to get the following informations from segmention_for_ld.h and kernel/tsar/arch/arch-config.h
#define HUGE_PAGE_SIZE    0x00200000 //necessary to compute rsrvd_limit

/////////////////////////////////////////////////
#define ARROUND_DOWN(val, size)  ((val) & ~((size) - 1))
#define ARROUND_UP(val, size) (((val) & ((size) -1)) ? ((val) & ~((size)-1)) + (size) : (val))

static char const rcsid[]="$Id: info2bib,v1.2 Ghassan Almaless $";

typedef struct arch_bib_device_s dev_info_t;
typedef struct arch_bib_cluster_s cluster_info_t;
typedef struct arch_bib_header_s header_info_t;

typedef struct token_s
{
	char *name;
	char *format;
	char *info;
	void *ptr;
} token_t;

static char buffer[512];  
static header_info_t header;

static token_t htokens[] = {
	{"REVISION", "\tREVISION=%hd\n", "Specification Revision Compliance", &header.revision}, 
	{"ARCH", "\tARCH=%s\n","Target Architecture Signature", &buffer[0]},
	{"XMAX", "\tXMAX=%hd\n", "Row Clusters Number", &header.x_max},
	{"YMAX", "\tYMAX=%hd\n", "Column Clusters Number", &header.y_max},
	{"CPU_NR", "\tCPU_NR=%d\n", "Per-Cluster Processors Number", &header.cpu_nr},
	{"BSCPU", "\tBSCPU=%d\n", "BootStrap Processor ID", &header.bootstrap_cpu},
	{"BSCPU_ARCH_ID", "\tBSCPU_ARCH_ID=%d\n", "BootStrap Processor ARCH_ID", &header.arch_bootstrap_cpu},
	{"BSTTY", "\tBSTTY=%llx\n", "BootStrap TTY Base Address", &header.bootstrap_tty}, 
	{"BSDMA", "\tBSDMA=%llx\n", "BootStrap DMA Base Address", &header.bootstrap_dma},
	{"IOPIC_CLUSTER", "\tIOPIC_CLUSTER=%hd\n", "Cluster in charge of dealing with IOPIC WTI", &header.iopic_cluster},
	{NULL, NULL, NULL, NULL}};

static cluster_info_t clstrinfo;

static token_t ctokens[] = {
	{"CID", "\tCID=%hhd\n", "Cluster ID", &clstrinfo.cid},
	{"ARCH_CID", "\tARCH_CID=%hhd\n", "Cluster ARCH_ID", &clstrinfo.arch_cid},
	{"CPU_NR", "\tCPU_NR=%hhd\n", "Cluster's CPUs NR", &clstrinfo.cpu_nr},
	{"DEV_NR", "\tDEV_NR=%hhd\n", "Cluster's Devices NR", &clstrinfo.dev_nr},
	{NULL, NULL, NULL, NULL}};

static dev_info_t devinfo;

static token_t dtokens[] = {
	{"DEVID", "\tDEVID=%s\n", "Device ID", &buffer[0]}, 
	{"BASE", "\tBASE=%llx\n","Mapped Device Base Address", &devinfo.base},
	{"SIZE", "\tSIZE=%x\n", "Mapped Device Size", &devinfo.size},
	{"IRQ", "\tIRQ=%hd\n", "Device's IRQ Number if any", &devinfo.irq},
	{NULL, NULL, NULL, NULL}};


static token_t hmarquer = {"[HEADER]" , "\t%s\n", "Marquer Of The Header Section", NULL};
static token_t cmarquer = {"[CLUSTER]", "\t%s\n", "Marquer Of New Cluster Description", NULL};
int verbose;

static int locate_marquer(FILE *input, token_t *marquer);
static void print_tokens(FILE *output, token_t *tokens);
static int compute_tokens(FILE *input, token_t *tokens);
static int info2bib(char *src_name, char *dst_name);
static void help(void);

int main(int argc, char *argv[])
{
	int hasInput;
	int hasOutput;
	int c, retval;
	char *src_name;
	char *dst_name;
	
	hasInput = 0;
	hasOutput = 0;
	retval = 0;
	verbose = 0;
	src_name = NULL;
	dst_name = NULL;
	 
	while ((c = getopt(argc, argv, "hHvVi:I:o:O:")) != -1)
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
		case 'H':
		case 'h':
			help();
			break;
		default:
			fprintf(stderr, "%s: invalid option %c\n", argv[0], c);
		}
	}

	if(!hasInput || !hasOutput)
	{
		fprintf(stderr, "\nUsage: %s [-vh] -iInputFile.info -oOutpuFile.bib\n\n", argv[0]);
		retval= EARGS;
	}
	else
		retval = info2bib(src_name, dst_name);
 
	free(src_name);
	free(dst_name);
	return retval;
}

static int info2bib(char *src_name, char *dst_name)
{
	FILE *fsrc;
	FILE *fdst;
	uint16_t cluster_nr;
	uint16_t dev_nr;
	uint16_t i,j;
	dev_info_t *dev_tbl;
	cluster_info_t *clusters_tbl;
	//dev_info_t *dev_info;
	uint16_t offset;
	uint16_t bootstrap_cid;
	int retval;
	uint16_t cid;

	retval = 0;
	fsrc = NULL;
	fdst = NULL;
	clusters_tbl = NULL;
	dev_tbl = NULL;
	//dev_info = NULL;
	header.onln_clstr_nr = 0;
	header.onln_cpu_nr = 0;
	header.iopic_cluster = -1;//FG
	header.with_cpu_clstr_nr = 0;//FG

	if((fsrc=fopen(src_name, "r")) == NULL)
	{
		fprintf(stderr, "Error: Opening %s: %s\n", src_name, (char*)strerror(errno));
		retval = ESRCOPEN;
		goto ERROR;
	}

	if((fdst=fopen(dst_name, "w+")) == NULL)
	{
		fprintf(stderr, "Error: Opening %s: %s\n", dst_name, (char*)strerror(errno));
		retval = EDSTOPEN;
		goto ERROR;
	}

	if((retval=locate_marquer(fsrc, &hmarquer)))
		goto ERROR;
	
	if((retval=compute_tokens(fsrc, htokens)))
		goto ERROR;

	if(strlen(&buffer[0]) >= TOKEN_ARCH_LEN)
	{
		fprintf(stderr, "Invalid ARCH NameCode, max len is %d\n", TOKEN_ARCH_LEN);
		retval=ETOKEN;
		goto ERROR;
	}

	strcpy(&header.signature[0], "@ALMOS ARCH BIB");
	strcpy(&header.arch[0], &buffer[0]); 
	cluster_nr = header.x_max * header.y_max;
	bootstrap_cid = header.bootstrap_cpu / header.cpu_nr;

	if((clusters_tbl = (cluster_info_t*) malloc(sizeof(cluster_info_t) * cluster_nr)) == NULL)
	{
		fprintf(stderr, "Faild to allocate cluster info table\n");
		retval = ENOMEM;
		goto ERROR;
	}

	offset = sizeof(cluster_info_t)*cluster_nr + sizeof(header_info_t);
	fseek(fdst, offset, SEEK_SET);
	
	for(i=0; i < cluster_nr; i++)
	{
		if((retval=locate_marquer(fsrc, &cmarquer)))
		{
			fprintf(stderr, "[cluster %d]\n", i);
			goto ERROR;
		}

		if((retval=compute_tokens(fsrc, ctokens)))
		{
			fprintf(stderr, "[cluster %d]\n", i);
			goto ERROR;
		}
 
		dev_nr = clstrinfo.dev_nr;
		cid = clstrinfo.cid;

		/* ignore the empty clusters (FG) */
		if(dev_nr == 0 && clstrinfo.cpu_nr ==0)
			continue;

		if((dev_tbl=(dev_info_t*) malloc(sizeof(dev_info_t) * dev_nr)) == NULL)
		{
			fprintf(stderr, "Faild to allocate dev info for cluster %d\n",cid);
			retval = ENOMEM;
			goto ERROR;
		}

		log_msg("CPU_NR %d\nDEV_NR %d\n", clstrinfo.cpu_nr, clstrinfo.dev_nr);
		
		clusters_tbl[cid].cid = clstrinfo.cid;
		clusters_tbl[cid].arch_cid = clstrinfo.arch_cid;
		clusters_tbl[cid].cpu_nr = clstrinfo.cpu_nr;
		clusters_tbl[cid].dev_nr = clstrinfo.dev_nr;
		clusters_tbl[cid].offset = (uint32_t)ftell(fdst);

		for(j=0; j < dev_nr; j++)
		{
			if((retval=compute_tokens(fsrc, dtokens)))
			{
				fprintf(stderr, "[cluster %d, dev %d]\n", cid, j);
				goto ERROR;
			}
			
			if((retval = dev_locate_by_name(&buffer[0], &dev_tbl[j].id)))
			{
				fprintf(stderr, "Unknown DEVID %s [cluster %d, dev %d]\n", 
					&buffer[0], cid, j);
				goto ERROR;
			}

			//FG : do not throw error if cluster without cpu has no RAM 
			if((j==0) && (dev_tbl[0].id != SOCLIB_RAM_ID) && clstrinfo.cpu_nr != 0 )
			{
				fprintf(stderr, "Devices description must start \
					with DEVID=%s, found %s [cluster %d, dev %d]\n", 
					dev_locate_by_id(SOCLIB_RAM_ID), &buffer[0], cid, j);
				goto ERROR;
			}

			dev_tbl[j].base = devinfo.base;
			dev_tbl[j].size = devinfo.size;
			dev_tbl[j].irq  = devinfo.irq;
			
			if((cid == bootstrap_cid) && (j == 0))//boot cluster has always some ram
			{
				 header.rsrvd_start = devinfo.base;
				 header.rsrvd_limit = devinfo.base + devinfo.size;
			}

			log_msg("Base 0x%llx, Size 0x%x, irq %d\n",(long long unsigned int)dev_tbl[j].base,dev_tbl[j].size,dev_tbl[j].irq);
		}

		header.onln_cpu_nr += clusters_tbl[cid].cpu_nr;
		if(clusters_tbl[cid].cpu_nr > 0)
			 header.with_cpu_clstr_nr ++;

		//FG : there may be online cluster without active cpu
		header.onln_clstr_nr ++;

		fwrite(dev_tbl, sizeof(dev_info_t), dev_nr, fdst);
		free(dev_tbl);
		dev_tbl = NULL;
	}

	header.size = (uint32_t)ftell(fdst);
	fseek(fdst, 0, SEEK_SET);
	fwrite(&header, sizeof(header), 1, fdst);
	fwrite(clusters_tbl, sizeof(cluster_info_t), cluster_nr, fdst);
	retval = (rcsid == NULL);

	log_msg("Header:\n\tReserved <0x%llx - 0x%llx>\n\tSignature: %s\n\tRevision: %d\n",//FG
	  (long long unsigned int)header.rsrvd_start, 
	 (long long unsigned int) header.rsrvd_limit,
	  &header.signature[0], 
	  header.revision);

	log_msg("\tArch: %s\n\tSize: %d\n\tXMAX: %d\n\tYMAX: %d\n\tCPU NR: %d\n",
	  &header.arch[0], 
	  header.size,
	  header.x_max,
	  header.y_max,
	  header.cpu_nr);

	log_msg("\tONLN Clsuter NR: %d\n\tONLN CPUs NR: %d\n\tBSCPU: %d\n\tBSTTY: 0x%llx\n\tBSDMA: 0x%llx\n",//FG long long type 
	  header.onln_clstr_nr, 
	  header.onln_cpu_nr,
	  header.bootstrap_cpu, 
	 (long long unsigned int) header.bootstrap_tty, 
	  (long long unsigned int)header.bootstrap_dma);

 ERROR:
	free(clusters_tbl);
	free(dev_tbl);
	if(fsrc != NULL) fclose(fsrc);
	if(fdst != NULL) fclose(fdst);
	return retval;
}


static int compute_tokens(FILE *input, token_t *tokens)
{
	int i;
	int retval;
	
	for(i=0; tokens[i].name != NULL; i++)
	{
		if((retval=fscanf(input, tokens[i].format, tokens[i].ptr)) != 1)
		{
			fprintf(stderr, "I can't understand token: %s\n", tokens[i].name);
			return ETOKEN;
		}
	}
	return 0;
}

static int locate_marquer(FILE *input, token_t *marquer)
{
	int retval;

	do
	{
		retval=fscanf(input, marquer->format, &buffer[0]);
		if(retval == EOF)
		{
			fprintf(stderr, "Faild to locate %s marquer\n", marquer->name);
			return EMARQUER;
		}
	}while(strcmp(&buffer[0], marquer->name));

	return 0;
}

static void print_tokens(FILE *output, token_t *tokens)
{
	int i;

	for(i=0; tokens[i].name != NULL; i++)
		fprintf(output, "Token Name: %s, It's order %d, Info: %s\n", tokens[i].name, i, tokens[i].info);
}


static void help(void)
{
	printf("\nHardware Description Format\n\n");
	printf("One header marquer per file : [HEADER]\n");
	printf("Header section tokens are\n");
	print_tokens(stdout, htokens);
	printf("One cluster marquer per cluster description: [CLUSTER]\n");
	printf("Cluster section tokens:\n");
	print_tokens(stdout, ctokens);
	printf("Cluster's devices description:\n");
	print_tokens(stdout, dtokens);
}
