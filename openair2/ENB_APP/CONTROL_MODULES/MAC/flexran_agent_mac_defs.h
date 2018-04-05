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

/*! \file flexran_agent_mac_defs.h
 * \brief FlexRAN agent - mac interface primitives
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 * \mail x.foukas@sms.ed.ac.uk
 */

#ifndef __FLEXRAN_AGENT_MAC_PRIMITIVES_H__
#define __FLEXRAN_AGENT_MAC_PRIMITIVES_H__

#include "flexran_agent_defs.h"
#include "flexran.pb-c.h"
#include "header.pb-c.h"

#define RINGBUFFER_SIZE 100

/* FLEXRAN AGENT-MAC Interface */
typedef struct {
  //msg_context_t *agent_ctxt;

  /// Inform the controller about the scheduling requests received during the subframe
  void (*flexran_agent_send_sr_info)(mid_t mod_id);
  
  /// Inform the controller about the current UL/DL subframe
  void (*flexran_agent_send_sf_trigger)(mid_t mod_id);

  /// Send to the controller all the mac stat updates that occured during this subframe
  /// based on the stats request configuration
  // void (*flexran_agent_send_update_mac_stats)(mid_t mod_id);

  /// Provide to the scheduler a pending dl_mac_config message
  void (*flexran_agent_get_pending_dl_mac_config)(mid_t mod_id,
						  Protocol__FlexranMessage **msg);
  
  /// Notify the controller for a state change of a particular UE, by sending the proper
  /// UE state change message (ACTIVATION, DEACTIVATION, HANDOVER)
  // int (*flexran_agent_notify_ue_state_change)(mid_t mod_id, uint32_t rnti,
		// 			       uint8_t state_change);
  
  
  void *dl_scheduler_loaded_lib;
  void *ul_scheduler_loaded_lib;
  /*TODO: Fill in with the rest of the MAC layer technology specific callbacks (UL/DL scheduling, RACH info etc)*/

} AGENT_MAC_xface;

#endif
