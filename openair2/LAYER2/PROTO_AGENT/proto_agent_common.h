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

/*! \file enb_agent_common.h
 * \brief common message primitves and utilities 
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */



#ifndef PROTO_AGENT_COMMON_H_
#define PROTO_AGENT_COMMON_H_

#include <time.h>

#include "flexsplit.pb-c.h"

// Do not need these
//#include "stats_messages.pb-c.h"
//#include "stats_common.pb-c.h"

#include "proto_agent_defs.h"
//#include "ENB_APP/enb_config.h"
#include "UTIL/MEM/mem_block.h"

//#include "LAYER2/MAC/extern.h"
//#include "LAYER2/RLC/rlc.h"

# include "tree.h"
# include "intertask_interface.h"

#define FLEXSPLIT_VERSION 0

typedef int (*proto_agent_message_decoded_callback)(
	mod_id_t mod_id,
       	const void *params,
	Protocol__FlexsplitMessage **msg
);

typedef int (*proto_agent_message_destruction_callback)(
	Protocol__FlexsplitMessage *msg
);


uint32_t ack_result;

/**********************************
 * progRAN protocol messages helper 
 * functions and generic handlers
 **********************************/

int proto_agent_serialize_message(Protocol__FlexsplitMessage *msg, uint8_t **buf, int *size);
int proto_agent_deserialize_message(void *data, int size, Protocol__FlexsplitMessage **msg);

uint8_t *proto_agent_pack_message(Protocol__FlexsplitMessage *msg, int *size);

err_code_t proto_agent_destroy_flexsplit_message(Protocol__FlexsplitMessage *msg);

int fsp_create_header(xid_t xid, Protocol__FspType type, Protocol__FspHeader **header);

int proto_agent_hello(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);
int proto_agent_destroy_hello(Protocol__FlexsplitMessage *msg);
int proto_agent_echo_request(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);
int proto_agent_destroy_echo_request(Protocol__FlexsplitMessage *msg);
int proto_agent_echo_reply(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);
int proto_agent_destroy_echo_reply(Protocol__FlexsplitMessage *msg);

int proto_agent_pdcp_data_req(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);
int proto_agent_pdcp_data_req_process(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);
int proto_agent_destroy_pdcp_data_req(Protocol__FlexsplitMessage *msg);
int proto_agent_pdcp_data_ind(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);
int proto_agent_destroy_pdcp_data_ind(Protocol__FlexsplitMessage *msg);
int proto_agent_pdcp_data_ind_process(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);

int just_print(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);


int proto_agent_get_ack_result(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg);


Protocol__FlexsplitMessage* proto_agent_handle_message (mod_id_t mod_id, 
						    uint8_t *data, 
						    int size);

Protocol__FlexsplitMessage *proto_agent_handle_timed_task(void *args);

typedef struct _data_req_args data_req_args;
typedef struct _dl_data_args dl_data_args;

struct _data_req_args{
  const protocol_ctxt_t* ctxt;
  srb_flag_t srb_flag;
  MBMS_flag_t MBMS_flag;
  rb_id_t rb_id; 
  mui_t mui;
  confirm_t confirm;
  sdu_size_t sdu_size;
  mem_block_t *sdu_p;
};

struct _dl_data_args{
  uint8_t pdu_type;
  uint32_t sn;
  frame_t frame;
  sub_frame_t subframe;
  rnti_t rnti;
  sdu_size_t sdu_size;
  mem_block_t *sdu_p;
};


#endif







