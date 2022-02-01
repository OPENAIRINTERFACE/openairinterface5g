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

/*! \file otg_tx_socket.h
* \brief brief explain how this block is organized, and how it works: OTG TX traffic generation functions with sockets
* \author A. Hafsaoui
* \date 2012
* \version 0.1
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/

#ifndef __OTG_TX_SOCKET_H__
#define __OTG_TX_SOCKET_H__


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "otg.h"
#include "otg_tx.h"


//-----------------------begin func proto-------------------

/*! \fn socket_packet_send(int, int , int)
* \brief this function allow to run the client, with the appropriate parameters.
* \param[in] src, dst and the state
* \param[out]  void
* \return void
* \note
* @ingroup  _otg
*/
void socket_packet_send(int src, int dst, int state,int ctime);

/*! \fn client_socket_tcp_ip4(int, int , int)
* \brief this function allow to run the client, with IPv4 and TCP protocol.
* \param[in] src, dst and the state
* \param[out]  void
* \return void
* \note
* @ingroup  _otg
*/
void client_socket_tcp_ip4(int src, int dst, int state,int ctime);

/*! \fn client_socket_tcp_ip6(int, int , int)
* \brief this function allow to run the client, with IPv6 and TCP protocol.
* \param[in] src, dst and the state
* \param[out]  void
* \return void
* \note
* @ingroup  _otg
*/
void client_socket_tcp_ip6(int src, int dst, int state,int ctime);

/*! \fn client_socket_udp_ip4(int, int , int)
* \brief this function allow to run the client, with IPv4 and UDP protocol.
* \param[in] src, dst and the state
* \param[out]  void
* \return void
* \note
* @ingroup  _otg
*/
void client_socket_udp_ip4(int src, int dst, int state,int ctime);

/*! \fn client_socket_udp_ip6(int, int , int)
* \brief this function allow to run the client, with IPv6 and UDP protocol.
* \param[in] src, dst and the state
* \param[out]  void
* \return void
* \note
* @ingroup  _otg
*/
void client_socket_udp_ip6(int src, int dst, int state,int ctime);



/*! \fn int packet_gen(int src, int dst, int state, int ctime)
* \brief return char *  pointer over the payload, else NULL
* \param[in] source,
* \param[out] packet_t: the generated packet: otg_header + header + payload
* \note
* @ingroup  _otg
*/
char* packet_gen_socket(int src, int dst, int state, int ctime);




control_hdr_t *otg_info_hdr_gen(int src, int dst, int trans_proto, int ip_v);




void init_control_header();


//-----------------------end func proto-------------------



#endif
