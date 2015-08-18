/*
 * memc_perfmon.h - TSAR MEMC Performance Monitor API
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

#ifndef _MEMC_PERFMON_H_
#define _MEMC_PERFMON_H_

struct memc_context_s;

/**
 * TSAR MEMC Performance Monitor, an opac 
 * perfmon context declaration.
 **/
struct memc_perfmon_s
{
	struct memc_context_s *tbl;
	int count;
};

/**
 * Initialize a perfmon context.
 * (All online MEMC are handled)
 *
 * @perfmon        perfmon context to be initialized
 * @return         Non-zero value in case of error
 */
int  memc_perfmon_init(struct memc_perfmon_s *perfmon);

/**
 * Start activity accounting.
 *
 * @perfmon        perfmon context holding counters
 * @retrun         Non-zero value in case of error
*/
int  memc_perfmon_start(struct memc_perfmon_s *perfmon);

/**
 * Stop activity accounting.
 *
 * @perfmon        perfmon context holding counters
 * @retrun         Non-zero value in case of error
*/
int  memc_perfmon_stop(struct memc_perfmon_s *perfmon);

/**
 * Print on stdout the value of perfmon counters.
 * (All online MEMC are handled)
 *
 * @perfmon        perfmon context holding counters
*/
void memc_perfmon_print(struct memc_perfmon_s *perfmon);

/**
 * Print on stdout the value of internal perfmon counters.
 * (All online MEMC are handled, START and STOP)
 *
 * @perfmon        perfmon context holding counters
*/
void memc_perfmon_dump(struct memc_perfmon_s *perfmon);

/**
 * Destroy a given perfmon and free all of its resources.
 *
 * @perfmon        perfmon context holding counters
*/
void memc_perfmon_destroy(struct memc_perfmon_s *perfmon);

#endif	/* _MEMC_PERFMON_ */
