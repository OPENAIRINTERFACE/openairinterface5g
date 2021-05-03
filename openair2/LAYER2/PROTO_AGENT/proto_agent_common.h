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







