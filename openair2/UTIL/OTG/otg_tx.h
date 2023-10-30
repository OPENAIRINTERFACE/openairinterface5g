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

/*! \file otg_tx.h
* \brief Data structure and functions for OTG
* \author N. Nikaein and A. Hafsaoui
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OTG_TX_H__
# define __OTG_TX_H__



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "otg.h"

/*! \fn int time_dist(const int src, const int dst, const int state)
* \brief compute Inter Departure Time, in ms
* \param[in] Source, destination, state
* \param[out] Inter Departure Time
* \note
* @ingroup  _otg
*/
int time_dist(
  const int src_instance,
  const int dst_instance,
  const int application,
  const int state);

/*! \fn int size_dist(const int src, const int dst, const int state)
* \brief compute the payload size, in bytes
* \param[in] Source, node_dst, state
* \param[out] size of the payload, in bytes
* \note
* @ingroup  _otg
*/
int size_dist(
  const int src_instance,
  const int dst_instance,
  const int application,
  const int state);

/*! \fn char *random_string(const int size, const ALPHABET_GEN mode, const ALPHABET_TYPE data_type);
* \brief return a random string[size]
* \param[in] size  of the string to generate,
* \param[in] ALPHABET_GEN  : static or random string
* \param[in] ALPHABET_TYPE : numeric or letters + numeric
* \param[out] string of a random char
* \note
* @ingroup  _otg
*/
char * random_string(const int size, const ALPHABET_GEN mode, const ALPHABET_TYPE data_type);

/*! \fn int packet_gen(const int src, const int dst, const int state, const int ctime)
* \brief return int= 1 if the packet is generated: OTG header + header + payload, else 0
* \param[in] src source identity
* \param[in] dst destination id
* \param[in] application id that might generate the packet
* \param[out] final packet size
* \param[out] packet_t: the generated packet: otg_header + header + payload
* \note
* @ingroup  _otg
*/
unsigned char *packet_gen(
  const int src_instance,
  const int dst_instance,
  const int app,
  const int ctime,
  unsigned int * const pkt_size);

/*! \fn unsigned char * packet_gen_multicast(const int src, const int dst, const int ctime, unsigned int * const pkt_size)
* \brief return int= 1 if the packet is generated: OTG header + header + payload, else 0
* \param[in] src source identity
* \param[in] dst destination id
* \param[in] ctime
* \param[out] final packet size
* \note
* @ingroup  _otg
*/
unsigned char *packet_gen_multicast(
  const int src_instance,
  const int dst_instance,
  const int ctime,
  unsigned int * const pkt_size);

/*! \fn char *header_gen(const int  hdr_size);
* \brief generate IP (v4/v6) + transport header(TCP/UDP)
* \param[in] int : size
* \param[out] the payload corresponding to ip version and transport protocol
* \note
* @ingroup  _otg
*/
unsigned char *header_gen(const int hdr_size);

/*! \fn char *payload_pkts(const int payload_size);
* \brief generate the payload
* \param[in] int : payload size
* \param[out] char * payload
* \note
* @ingroup  _otg
*/
unsigned char *payload_pkts(const int payload_size);

/*! \fn char * serialize_buffer(char* const header, char* const payload, const unsigned int buffer_size, const int flag, const int
flow_id, const int ctime, const int seq_num, const int hdr_type, const int state)
* \brief serilize the packet and add otg control information
* \param[in] char* header pointer to the header
* \param[in] char* payload pointer to the payload
* \param[in] buffer_size size of the serilized tx buffer
* \param[in] flag for regular and background traffic
* \param[in] flow_id
* \param[in] ctime time of the transmission
* \param[in] seq_num sequence number of the packet used to calculate the loss rate
* \param[in] hdr_type indicatting the ip protocol version and transport protocol
* \param[in] state indicate in which state (PU, ED, PE) the packet is generated when generating M2M traffic
flow id, simulation time, , sequence number, header type (to know the transport/ip version in the RX)
* \param[out] tx_buffer the serilized packet
* @ingroup  _otg
*/

unsigned char * serialize_buffer(
  char* const header,
  char* const payload,
  const unsigned int buffer_size,
  const unsigned int traffic_type,
  const int flag,
  const int flow_id,
  const int ctime,
  const int seq_num,
  const int hdr_type,
  const int state,
  const unsigned int aggregation_level,
  const int src_instance,
  const int dst_instance);


/*! \fn int adjust_size(int size);
* \brief adjuste the generated packet size when size<min or size>max
* \param[in]  size
* \param[out] modified size in case
* \note
* @ingroup  _otg
*/
int adjust_size(int size);

void header_size_gen(const int src, const int dst, const int application);

void init_predef_multicast_traffic(void);

/*! \fn void init_predef_traffic();
* \brief initialise OTG with predifined value for pre-configured traffic: cbr, openarena,etc.
* \param[in]
* \param[out]
* \note
* @ingroup  _otg
*/
void init_predef_traffic(const unsigned char nb_ue_local, const unsigned char nb_enb_local);

/*! \fn int background_gen(const int src, const int dst, const int ctime);
* \brief manage idt and packet size for the backgrounf traffic.
* \param[in] src
* \param[in] dst
* \param[in] ctime
* \param[out]
* \note
* @ingroup  _otg
*/
int background_gen(const int src, const int dst, const int ctime);

int header_size_gen_background(const int src, const int dst);

void state_management(const int src, const int dst,const int application, const int ctime);

void voip_traffic(const int src, const int dst, const int application, const int ctime);

int otg_hdr_size(
  const int src_instance,
  const int dst_instance);

void init_packet_gen(const int src_instance, const int dst_instance, const int ctime);

int check_data_transmit(const int src,const int dst, const int app, const int ctime);

unsigned int get_application_state(const int src, const int dst, const int application, const int ctime);

void check_ctime(const int ctime);

#endif
