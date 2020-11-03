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

/*! \file flexran_agent_app.h
 * \brief Common app definitions
 * \author Robert Schmidt
 * \date 2020
 * \version 0.1
 */

#ifndef BURST_ANALYSIS_LL_H_
#define BURST_ANALYSIS_LL_H_

#include "flexran_agent_common.h"
#include "flexran_agent_async.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_timer.h"
#include "flexran_agent_defs.h"
#include "flexran_agent_net_comm.h"
#include "flexran_agent_ran_api.h"
#include "flexran_agent_phy.h"
#include "flexran_agent_mac.h"
#include "flexran_agent_rrc.h"
#include "flexran_agent_pdcp.h"
#include "flexran_agent_s1ap.h"
#include "common/utils/LOG/log.h"
#include "assertions.h"

/* App type: to be implemented by shared libraries */
typedef struct {
  int (*start)(mid_t mod_id,
               Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p,
               int n_p);
  int (*reconfig)(mid_t mod_id,
                  Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p,
                  int n_p);
  int (* stop)(mid_t mod_id);
} flexran_agent_app_t;

/* FlexRAN agent app handling init */
void flexran_agent_app_init(mid_t mod_id);

/* Start an app from within the agent */
int flexran_agent_start_app_direct(mid_t mod_id, flexran_agent_app_t *app, char *name);

/* FlexRAN agent handler to setup/teardown apps */
void flexran_agent_handle_apps(
    mid_t mod_id,
    Protocol__FlexAgentReconfigurationSubsystem **subs,
    int n_subs);

/* Fills the enb_config_reply with the currently loaded (started) apps */
void flexran_agent_fill_loaded_apps(mid_t mod_id,
                                    Protocol__FlexEnbConfigReply *reply);

#endif
