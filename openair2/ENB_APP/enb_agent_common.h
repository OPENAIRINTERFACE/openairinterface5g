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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file 
 * \brief 
 * \author 
 * \date 2016
 * \version 0.1
 */



#ifndef ENB_AGENT_COMMON_H_
#define ENB_AGENT_COMMON_H_


#include "header.pb-c.h"
#include "progran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"

#define PROGRAN_VERSION 0

typedef int (*enb_agent_message_decoded_callback)(
	uint32_t xid,
	Protocol__ProgranMessage **msg
);

typedef int32_t  err_code_t; 

int enb_agent_serialize_message(Protocol__ProgranMessage *msg, void **buf, int *size);
int enb_agent_deserialize_message(void *data, int size, Protocol__ProgranMessage **msg);

int prp_create_header(uint32_t xid, Protocol__PrpType type, Protocol__PrpHeader **header);

int enb_agent_hello(uint32_t xid, Protocol__ProgranMessage **msg);
int enb_agent_destroy_hello(Protocol__ProgranMessage *msg);

int enb_agent_echo_request(uint32_t xid, Protocol__ProgranMessage **msg);
int enb_agent_destroy_echo_request(Protocol__ProgranMessage *msg);

int enb_agent_echo_reply(uint32_t xid, Protocol__ProgranMessage **msg);
int enb_agent_destroy_echo_reply(Protocol__ProgranMessage *msg);


Protocol__ProgranMessage* enb_agent_handle_message (uint32_t xid, 
						    uint8_t *data, 
						    uint32_t size);

void * enb_agent_send_message(uint32_t xid, 
			      Protocol__ProgranMessage *msg, 
			      uint32_t * size);


#endif







