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

/*! \file ENB_APP/extern.h
 * \brief FlexRAN agent - Extern VSF xfaces
 * \author Xenofon Foukas and shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 * \mail x.foukas@sms.ed.ac.uk
 */

#ifndef __FLEXRAN_AGENT_EXTERN_H__
#define __FLEXRAN_AGENT_EXTERN_H__

#include "flexran_agent_defs.h"
#include "flexran_agent_phy_defs.h"
#include "flexran_agent_mac_defs.h"
#include "flexran_agent_rrc_defs.h"
#include "flexran_agent_pdcp_defs.h"
#include "flexran_agent_s1ap_defs.h"

/* Control module interface for the communication of the PHY control module with the agent */
AGENT_PHY_xface *flexran_agent_get_phy_xface(mid_t mod_id);

/* Control module interface for the communication of the MAC Control Module with the agent */

extern AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB];
#define flexran_agent_get_mac_xface(mod_id) (agent_mac_xface[mod_id])
/* Control module interface for the communication of the RRC Control Module with the agent */
// AGENT_RRC_xface *flexran_agent_get_rrc_xface(mid_t mod_id);
extern AGENT_RRC_xface *agent_rrc_xface[NUM_MAX_ENB];
#define flexran_agent_get_rrc_xface(mod_id) (agent_rrc_xface[mod_id])

/* Control module interface for the communication of the RRC Control Module with the agent */
AGENT_PDCP_xface *flexran_agent_get_pdcp_xface(mid_t mod_id);

/* Control module interface for the communication of the S1AP Control Module with the agent */
AGENT_S1AP_xface *flexran_agent_get_s1ap_xface(mid_t mod_id);

/* Requried to know which UEs had a harq updated over some subframe */
extern int harq_pid_updated[NUM_MAX_UE][8];
extern int harq_pid_round[NUM_MAX_UE][8];

#endif
