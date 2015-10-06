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

/*! \file otg_rx.h
* \brief Data structure and functions for OTG receiver
* \author navid nikaein A. Hafsaoui
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/


#ifndef __OTG_RX_H__
# define __OTG_RX_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otg.h"





/*! \fn int otg_rx_pkt(const int dst_instanceP, const int ctime, const char * const buffer_tx, const unsigned int size);
* \brief check if the packet is well received and do measurements: one way delay, throughput,etc.
* \param[in] the destination
* \param[in] time of the emulation
* \param[in] The packet
* \param[in] Size of the packet
* \param[out] return NULL is the packet is well received,  else the packet to forward
* \note
* @ingroup  _otg
*/
int otg_rx_pkt(const int dst_instanceP, const int ctime, const char * const buffer_tx, const unsigned int size);


/*! \fn void owd_const_gen(int src,int dst);
*\brief compute the one way delay introduced in LTE/LTE-A network REF PAPER: "Latency for Real-Time Machine-to-Machine Communication in LTE-Based System Architecture"
*\param[in] the source
*\param[in] the destination
*\param[out] void
*\note
*@ingroup  _otg
*/
void owd_const_gen(const int src, const int dst, const int flow_id, const unsigned int flag);

/*! \fn float owd_const_capillary();
*\brief compute the one way delay introduced in LTE/LTE-A network REF PAPER: "Latency for Real-Time Machine-to-Machine Communication in LTE-Based System Architecture"
*\param[out] float: capillary delay constant
*\note
*@ingroup  _otg
*/
float owd_const_capillary(void);

/*! \fn float owd_const_mobile_core();
*\brief compute the one way delay introduced in LTE/LTE-A network REF PAPER: "Latency for Real-Time Machine-to-Machine Communication in LTE-Based System Architecture"
*\param[out] float: mobile core delay constant
*\note
*@ingroup  _otg
*/
float owd_const_mobile_core(void);

/*! \fn float owd_const_IP_backbone();
*\brief compute the one way delay introduced in LTE/LTE-A network REF PAPER: "Latency for Real-Time Machine-to-Machine Communication in LTE-Based System Architecture"
*\param[out] float: IP backbone delay constant
*\note
*@ingroup  _otg
*/
float owd_const_IP_backbone(void);

/*! \fn float owd_const_applicatione();
*\brief compute the one way delay introduced in LTE/LTE-A network REF PAPER: "Latency for Real-Time Machine-to-Machine Communication in LTE-Based System Architecture"
*\param[out] float: application delay constant
*\note
*@ingroup  _otg
*/
float owd_const_application(void);


/*! \fn void rx_check_loss(const int src, const int dst, const unsigned int flag, const int seq_num, unsigned int * const seq_num_rx, unsigned int * const nb_loss_pkts);
*\brief check the number of loss packet/out of sequence
*\param[in] src
*\param[in] dst
*\param[in] flag: background or data
*\param[in] seq_num: packet sequence number
*\param[in] seq_num_rx:RX sequence number
*\param[in] nb_loss_pkts: number of lost packet
*\param[out] lost_packet: (0) no lost packets, (1) lost packets
*\note
*@ingroup  _otg
*/
int rx_check_loss(
  const int src,
  const int dst,
  const unsigned int flag,
  const int seq_num,
  unsigned int * const seq_num_rx,
  unsigned int * const nb_loss_pkts);

#endif
