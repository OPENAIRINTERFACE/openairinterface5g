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

/*! \file flexran_agent_common.h
 * \brief common message primitves and utilities 
 * \author Xenofon Foukas, Mohamed Kassem and Navid Nikaein and shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */



#ifndef FLEXRAN_AGENT_COMMON_H_
#define FLEXRAN_AGENT_COMMON_H_

#include <time.h>

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"
#include "flexran_agent_ran_api.h"
#include "flexran_agent_net_comm.h"
#include "flexran_agent_defs.h"
#include "enb_config.h"

#include "LAYER2/RLC/rlc.h"

# include "tree.h"
# include "intertask_interface.h"

#define FLEXRAN_VERSION 0

typedef int (*flexran_agent_message_decoded_callback)(
	mid_t mod_id,
       	const void *params,
	Protocol__FlexranMessage **msg
);

typedef int (*flexran_agent_message_destruction_callback)(
	Protocol__FlexranMessage *msg
);

typedef struct {
 
  uint8_t is_initialized;
  volatile uint8_t cont_update;
  xid_t xid;
  Protocol__FlexranMessage *stats_req;
  Protocol__FlexranMessage *prev_stats_reply;

  pthread_mutex_t *mutex;
} stats_updates_context_t;

stats_updates_context_t stats_context[NUM_MAX_ENB];

/**********************************
 * FlexRAN protocol messages helper 
 * functions and generic handlers
 **********************************/

/* Helper functions for message (de)serialization */
int flexran_agent_serialize_message(Protocol__FlexranMessage *msg, void **buf, int *size);
int flexran_agent_deserialize_message(void *data, int size, Protocol__FlexranMessage **msg);

/* Serialize message and then destroy the input flexran msg. Should be called when protocol
   message is created dynamically */
void * flexran_agent_pack_message(Protocol__FlexranMessage *msg, 
			      int * size);

/* Calls destructor of the given message */
err_code_t flexran_agent_destroy_flexran_message(Protocol__FlexranMessage *msg);

/* Function to create the header for any FlexRAN protocol message */
int flexran_create_header(xid_t xid, Protocol__FlexType type, Protocol__FlexHeader **header);

/* Hello protocol message constructor and destructor */
int flexran_agent_hello(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_hello(Protocol__FlexranMessage *msg);

/* Echo request protocol message constructor and destructor */
int flexran_agent_echo_request(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_echo_request(Protocol__FlexranMessage *msg);

/* Echo reply protocol message constructor and destructor */
int flexran_agent_echo_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_echo_reply(Protocol__FlexranMessage *msg);

/* eNodeB configuration reply message constructor and destructor */
int flexran_agent_enb_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_enb_config_reply(Protocol__FlexranMessage *msg);

/* UE configuration reply message constructor and destructor */
int flexran_agent_ue_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_ue_config_reply(Protocol__FlexranMessage *msg);

/* Logical channel reply configuration message constructor and destructor */
int flexran_agent_lc_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_lc_config_reply(Protocol__FlexranMessage *msg);

/* eNodeB configuration request message constructor and destructor */
int flexran_agent_enb_config_request(mid_t mod_id, const void* params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_enb_config_request(Protocol__FlexranMessage *msg);

/* UE configuration request message constructor */
/* TODO: Need to define and implement destructor */
int flexran_agent_destroy_ue_config_request(Protocol__FlexranMessage *msg);

/* Logical channel configuration request message constructor */
/* TODO: Need to define and implement destructor */
int flexran_agent_destroy_lc_config_request(Protocol__FlexranMessage *msg);

/* Control delegation message constructor and destructor */
int flexran_agent_control_delegation(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_control_delegation(Protocol__FlexranMessage *msg);

/* Policy reconfiguration message constructor and destructor */
int flexran_agent_reconfiguration(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_agent_reconfiguration(Protocol__FlexranMessage *msg);

/* rrc triggering measurement message constructor and destructor */
int flexran_agent_rrc_reconfiguration(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_rrc_reconfiguration(Protocol__FlexranMessage *msg);

/* rrc triggering handover command message constructor and destructor */
int flexran_agent_rrc_trigger_handover(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_rrc_trigger_handover(Protocol__FlexranMessage *msg);

/* FlexRAN protocol message dispatcher function */
Protocol__FlexranMessage* flexran_agent_handle_message (mid_t mod_id, 
						    uint8_t *data, 
						    uint32_t size);

/* Function to be used to send a message to a dispatcher once the appropriate event is triggered. */
Protocol__FlexranMessage *flexran_agent_handle_timed_task(void *args);

/*Top level Statistics hanlder*/
int flexran_agent_handle_stats(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);

/* Function to be used to handle reply message . */
int flexran_agent_stats_reply(mid_t enb_id, xid_t xid, const report_config_t *report_config, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_stats_reply(Protocol__FlexranMessage *msg);

/* Top level Statistics request protocol message constructor and destructor */
int flexran_agent_stats_request(mid_t mod_id, xid_t xid, const stats_request_config_t *report_config, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_stats_request(Protocol__FlexranMessage *msg);

err_code_t flexran_agent_init_cont_stats_update(mid_t mod_id);

err_code_t flexran_agent_enable_cont_stats_update(mid_t mod_id, xid_t xid, stats_request_config_t *stats_req) ;
err_code_t flexran_agent_disable_cont_stats_update(mid_t mod_id);

/* Handle a received eNB config reply message as an "order" to reconfigure. It
 * does not come as a reconfiguration message as this is a "structured"
 * ProtoBuf message (as opposed to "unstructured" YAML). There is no destructor
 * since we do not reply to this message (yet). Instead, the controller has to
 * issue another eNB config request message. */
int flexran_agent_handle_enb_config_reply(mid_t mod_id, const void* params, Protocol__FlexranMessage **msg);

/* Handle a received UE config reply message as an "order" to reconfigure the
 * association of a UE to a slice.  It does not come as a reconfiguration
 * message as this is a "structured" ProtoBuf message (as opposed to
 * "unstructured" YAML). There is no destructor since we do not reply to this
 * message (yet). Instead, the controller has to issue another eNB config
 * request message. */
int flexran_agent_handle_ue_config_reply(mid_t mod_id, const void* params, Protocol__FlexranMessage **msg);

#endif
