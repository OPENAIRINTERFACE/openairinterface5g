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


/*! \file flexran_agent_rrc_defs.h
 * \brief FlexRAN agent - RRC interface primitives
 * \author shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 * \mail 
 */

#ifndef __FLEXRAN_AGENT_RRC_PRIMITIVES_H__
#define __FLEXRAN_AGENT_RRC_PRIMITIVES_H__

#include "flexran_agent_defs.h"
#include "flexran.pb-c.h"
#include "header.pb-c.h"
#include "LTE_MeasResults.h"

#define RINGBUFFER_SIZE 100


typedef struct 
{
   int32_t rnti; 
   int32_t meas_id;
   int32_t rsrp;
   int32_t rsrq;

   /*To Be Extended*/
}rrc_meas_stats;



/* FLEXRAN AGENT-RRC Interface */
typedef struct {
  
  
   /// Notify the controller for a state change of a particular UE, by sending the proper
  /// UE state change message (ACTIVATION, DEACTIVATION, HANDOVER)
  void (*flexran_agent_notify_ue_state_change)(mid_t mod_id, uint32_t rnti,
                 uint8_t state_change);

  void (*flexran_trigger_rrc_measurements)(mid_t mod_id, LTE_MeasResults_t*  measResults);
  
} AGENT_RRC_xface;

#endif
