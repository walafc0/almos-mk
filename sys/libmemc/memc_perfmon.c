/*
 * memc_perfmon.c - TSAR MEMC Performance Monitor
 *
 * Copyright (c) 2013 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS.
 *
 * ALMOS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "memc_perfmon.h"

#define MEMC_REGS_NR   27

struct memc_regs_s
{
	const char *name;
	const char *info;
};

const struct memc_regs_s memc_regs[MEMC_REGS_NR] = {
	{"MEMC_RD_LOCAL", "Number of local read commands (instruction, data, TLB)"},
	{"MEMC_RD_REMOTE", "Number of remote read commands"},
	{"MEMC_RD_COST", "Cost (flits x distance) recieved read commands"},
	{"MEMC_WR_LOCAL", "Number of local write flits"},
	{"MEMC_WR_REMOTE", "Number of remote write flits"},
	{"MEMC_WR_COST", "Cost (flits x distance) of recieved write commands"},
	{"MEMC_LL_LOCAL", "Number of local linked-load commands"},
	{"MEMC_LL_REMOTE", "Number of remote linked-load commands"},
	{"MEMC_LL_COST", "Cost (flits x distance) of recieved linked-load commands"},
       	{"MEMC_SC_LOCAL", "Number of local store-conditional commands"},
	{"MEMC_SC_REMOTE", "Number of remote store-conditional commands"},
	{"MEMC_SC_COST", "Cost (flits x distance) of recieved store-conditional commands"},
       	{"MEMC_CS_LOCAL", "Number of local CAS commands"},
	{"MEMC_CS_REMOTE", "Number of remote CAS commands"},
	{"MEMC_CS_COST", "Cost (flits x distance) of recieved CAS commands"},
	{"MEMC_MUPDT_LOCAL", "Number of local multi-update commands"},
	{"MEMC_MUPDT_REMOTE", "Number of remote multi-update commands"},
	{"MEMC_MUPDT_COST", "Cost (flits x distance) of issued multi-update commands"},
	{"MEMC_MINVL_LOCAL", "Number of local multi-invalidate commands"},
	{"MEMC_MINVL_REMOTE", "Number of remote multi-invalidate commands"},
	{"MEMC_MINVL_COST", "Cost (flits x distance) of issued multi-invalidate commands"},
	{"MEMC_CLNUP_LOCAL", "Number of local clean-up commands"},
	{"MEMC_CLNUP_REMOTE", "Number of remote clean-up commands"},
	{"MEMC_CLNUP_COST", "Cost (flits x distance) of issued clean-up commands"},
	{"MEMC_MUPDT_TOTAL", "Total number of multi-update commands"},
	{"MEMC_MINVL_TOTAL", "Total number of multi-invalidate commands"},
	{"MEMC_BINVL_TOTAL", "Total number of broadcast-invalidate commands"}};

struct memc_context_s
{
	uint32_t start_tbl[MEMC_REGS_NR];
	uint32_t stop_tbl[MEMC_REGS_NR];
	int fd;
};

void memc_perfmon_destroy(struct memc_perfmon_s *perfmon)
{
	struct memc_context_s *ctx;
	int i;

	for(i = 0; i < perfmon->count; i++)
	{
		ctx = &perfmon->tbl[i];

		if(ctx->fd != -1)
			close(ctx->fd);
	}

	perfmon->count = 0;
	free(perfmon->tbl);
}

int memc_perfmon_init(struct memc_perfmon_s *perfmon)
{
	struct memc_context_s *ctx;
	int fd,count,i,j;
	char name[16];

	count = sysconf(_SC_NCLUSTERS_ONLN);
	count = (count < 1) ? 1 : count;
 
	perfmon->tbl = valloc(sizeof(*ctx) * count);

	if(perfmon->tbl == NULL)
		return -1;

	for(i = 0; i < count; i++)
	{
		sprintf(&name[0], "/DEV/MEMC%d", i);
		fd = open(&name[0], O_RDONLY, 0);

		perfmon->tbl[i].fd = fd;

		if(fd == -1)
			break;
		
		for(j = 0; j < MEMC_REGS_NR; j++)
		{
			perfmon->tbl[i].start_tbl[j] = 0;
			perfmon->tbl[i].stop_tbl[j]  = 0;
		}
	}

	perfmon->count = i;

	if(fd == -1)
	{
		memc_perfmon_destroy(perfmon);
		return -2;
	}

	return 0;
}

int memc_perfmon_start(struct memc_perfmon_s *perfmon)
{
	struct memc_context_s *ctx;
	int count, size, i;

	for(i = 0; i < perfmon->count; i++)
	{
		ctx   = &perfmon->tbl[i];
		size  = sizeof(ctx->start_tbl) - 12; /* last three registers seem to be bugious */
		count = read(ctx->fd, &ctx->start_tbl[0], size);

		if(count != size)
		{
			fprintf(stderr, 
				"Warning: %s - memc%d registers were"
				"not read entirely\n", __FUNCTION__, i);
		}
	}

	return 0;
}

int memc_perfmon_stop(struct memc_perfmon_s *perfmon)
{
	struct memc_context_s *ctx;
	int count, size, i;

	for(i = 0; i < perfmon->count; i++)
	{
		ctx   = &perfmon->tbl[i];
		size  = sizeof(ctx->stop_tbl) - 12; /* last three registers seem to be bugious */
		count = read(ctx->fd, &ctx->stop_tbl[0], size);

		if(count != size)
		{
			fprintf(stderr, 
				"Warning: %s - memc%d registers were"
				"not read entirely\n", __FUNCTION__, i);
		}
	}

	return 0;
}

void memc_perfmon_print(struct memc_perfmon_s *perfmon)
{
	struct memc_context_s *ctx;
	uint32_t value;
	int i,j;

	for(i = 0; i < perfmon->count; i++)
	{
		ctx = &perfmon->tbl[i];
		printf("[MEMC%d]\n", i);

		for(j = 0; j < MEMC_REGS_NR; j++)
		{
			value = ctx->stop_tbl[j] - ctx->start_tbl[j];
			printf("\t[%d] %s\t = %u\n", j, memc_regs[j].name, value);
		}
	}
}

void memc_perfmon_dump(struct memc_perfmon_s *perfmon)
{
	struct memc_context_s *ctx;
	uint32_t value;
	int i,j;

	for(i = 0; i < perfmon->count; i++)
	{
		ctx = &perfmon->tbl[i];
		printf("[MEMC%d]\n\t[START]\n", i);

		for(j = 0; j < MEMC_REGS_NR; j++)
			printf("\t\t[%d] %s\t = %u\n", j, memc_regs[j].name, ctx->start_tbl[j]);

		printf("\t[STOP]\n");

		for(j = 0; j < MEMC_REGS_NR; j++)
			printf("\t\t[%d] %s\t = %u\n", j, memc_regs[j].name, ctx->stop_tbl[j]);
	}
}
