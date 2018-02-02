/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
