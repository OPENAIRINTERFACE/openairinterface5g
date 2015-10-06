/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file otg_kpi.h functions to compute OTG KPIs
* \brief desribe function for KPIs computation
* \author navid nikaein and A. Hafsaoui
* \date 2012
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning

*/

#ifndef __OTG_KPI_H__
# define __OTG_KPI_H__


#include <stdio.h>
#include <stdlib.h>
#include "otg.h"
#include "otg_externs.h" // not needed, you should compute kpi from the pkt header


extern unsigned int start_log_latency;
extern unsigned int start_log_latency_bg;
extern unsigned int start_log_GP;
extern unsigned int start_log_GP_bg;
extern unsigned int start_log_jitter;

/*! \fn void tx_throughput( int src, int dst, int application)
* \brief compute the transmitter throughput in bytes per seconds
* \param[in] Source, destination, application
* \param[out]
* \note
* @ingroup  _otg
*/
void tx_throughput( int src, int dst, int application);

/*! \fn void rx_goodput( int src, int dst)
* \brief compute the receiver goodput in bytes per seconds
* \param[in] Source, destination, application
* \param[out]
* \note
* @ingroup  _otg
*/
void rx_goodput( int src, int dst,int application);


/*void rx_loss_rate_pkts(int src, int dst, int application)
* \brief compute the loss rate in bytes at the server bytes
* \param[in] Source, destination, application
* \param[out]
* \note
* @ingroup  _otg
*/
void rx_loss_rate_pkts(int src, int dst, int application);

/*void rx_loss_rate_bytes(int src, int dst, int application)
* \brief compute the loss rate in pkts at the server bytes
* \param[in] Source, destination, application
* \param[out]
* \note
* @ingroup  _otg
*/
void rx_loss_rate_bytes(int src, int dst, int application);

/*void kpi_gen(void)
* \brief compute KPIs after the end of the simulation
* \param[in]
* \param[out]
* \note
* @ingroup  _otg
*/
void kpi_gen(void);

void add_log_metric(int src, int dst, int ctime, double metric, unsigned int label);

void  add_log_label(unsigned int label, unsigned int * start_log_metric);

void otg_kpi_nb_loss_pkts(void);

void average_total_jitter(void);
#endif
