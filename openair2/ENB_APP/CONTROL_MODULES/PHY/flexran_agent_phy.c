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

/*! \file flexran_agent_phy.c
 * \brief FlexRAN agent Control Module PHY
 * \author Robert Schmidt
 * \date Oct 2018
 */

#include "flexran_agent_phy.h"

/* Array containing the Agent-PHY interfaces */
AGENT_PHY_xface *agent_phy_xface[NUM_MAX_ENB];

int flexran_agent_register_phy_xface(mid_t mod_id)
{
  if (agent_phy_xface[mod_id]) {
    LOG_E(PHY, "PHY agent for eNB %d is already registered\n", mod_id);
    return -1;
  }
  AGENT_PHY_xface *xface = malloc(sizeof(AGENT_PHY_xface));
  if (!xface) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for PHY agent xface %d\n", mod_id);
    return -1;
  }

  agent_phy_xface[mod_id] = xface;
  return 0;
}

int flexran_agent_unregister_phy_xface(mid_t mod_id)
{
  if (!agent_phy_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "PHY agent for eNB %d is not registered\n", mod_id);
    return -1;
  }
  free(agent_phy_xface[mod_id]);
  agent_phy_xface[mod_id] = NULL;
  return 0;
}

AGENT_PHY_xface *flexran_agent_get_phy_xface(mid_t mod_id)
{
  return agent_phy_xface[mod_id];
}
