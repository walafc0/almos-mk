/*
 * soclib_iopic.c - soclib iopic driver
 *
 * This file is part of ALMOS-kernel.
 *
 * ALMOS-kernel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS-kernel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS-kernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _SOCLIB_IOPIC_H_
#define _SOCLIB_IOPIC_H_

#include <bits.h>

/** IOPIC REGISTER **/
#define IOPIC_ADDRESS           0
#define IOPIC_EXTEND            1
#define IOPIC_STATUS            2
/**       SPAN     **/
#define IOPIC_SPAN              4

/** When distributing wti in clusters, give, if possible, IOPIC_WTI_PER_CLSTR
 * wti to each cluster for IOPIC_CLUSTER (cf arch-info) to cluster 0**/
#define IOPIC_WTI_PER_CLSTR     32

error_t soclib_iopic_init(struct device_s *iopic);
void iopic_bind_wti_and_irq(struct device_s *iopic, struct device_s *dev);
void iopic_get_status(struct device_s *iopic, uint_t wti_channel, uint_t * status);

extern driver_t soclib_iopic_driver;

#endif	/* _SOCLIB_IOPIC_H_ */
