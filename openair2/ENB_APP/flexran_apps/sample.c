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

/*! \file sample.c
 * \brief flexran sample app
 * \author Robert Schmidt
 * \date 2020
 * \version 0.1
 */

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
#include "flexran_agent_app.h"
#include "common/utils/LOG/log.h"
#include "assertions.h"

/* callback to be called for event: shows exemplary variable use and use of RAN
 * API */
int tick;
Protocol__FlexranMessage *sample_tick(
    mid_t mod_id,
    const Protocol__FlexranMessage *msg) {
  tick++;
  const int num = flexran_get_mac_num_ues(mod_id);
  LOG_I(FLEXRAN_AGENT, "%s(): tick %d number UEs %d\n", __func__, tick, num);
  return NULL;
}

void print_param(Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p, int n_p) {
  LOG_I(FLEXRAN_AGENT, "%s(): received %d parameters\n", __func__, n_p);
  for (int i = 0; i < n_p; ++i) {
    switch (p[i]->value->param_case) {
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM__NOT_SET:
        LOG_I(FLEXRAN_AGENT, "    param %s is not set\n", p[i]->key);
        break;
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM_INTEGER:
        LOG_I(FLEXRAN_AGENT, "    param %s is %d\n", p[i]->key, p[i]->value->integer);
        break;
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM_FLOATING:
        LOG_I(FLEXRAN_AGENT, "    param %s is %f\n", p[i]->key, p[i]->value->floating);
        break;
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM_BOOLEAN:
        LOG_I(FLEXRAN_AGENT, "    param %s is %s\n", p[i]->key, p[i]->value->boolean ? "true" : "false");
        break;
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM_STR:
        LOG_I(FLEXRAN_AGENT, "    param %s is %s\n", p[i]->key, p[i]->value->str);
        break;
      default:
        LOG_W(FLEXRAN_AGENT, "    type of param %s is unknown\n", p[i]->key);
        break;
    };
  }
}

/* sa_start: entry point for sample app. Installs a callback using the timer
 * API to have function sample_tick() called every 5s */
#define TIMER 22
int sa_start(mid_t mod_id,
             Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p,
             int n_p) {
  LOG_W(FLEXRAN_AGENT, "%s(): enable sample app timer\n", __func__);
  flexran_agent_create_timer(mod_id,
                             5000,
                             FLEXRAN_AGENT_TIMER_TYPE_PERIODIC,
                             TIMER,
                             sample_tick,
                             NULL);
  print_param(p, n_p);
  return 0;
}

int sa_reconfig(mid_t mod_id,
                Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p,
                int n_p)
{
  print_param(p, n_p);
  return 0;
}

/* stop function: cleans up before app can be unloaded. Here: stop the timer */
int sa_stop(mid_t mod_id) {
  LOG_W(FLEXRAN_AGENT, "%s(): disable sample app timer\n", __func__);
  flexran_agent_destroy_timer(mod_id, TIMER);
  return 0;
}

/* app definition. visibility should be set to default so this symbol is
 * reachable from outside */
flexran_agent_app_t __attribute__ ((visibility ("default"))) app = {
  .start = sa_start,
  .reconfig = sa_reconfig,
  .stop =  sa_stop
};
