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

/*! \file flexran_agent_phy.h
 * \brief FlexRAN agent Control Module PHY header
 * \author Robert Schmidt
 * \date Oct 2018
 */

#ifndef FLEXRAN_AGENT_PHY_H_
#define FLEXRAN_AGENT_PHY_H_

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"


#include "flexran_agent_common.h"
#include "flexran_agent_defs.h"
#include "flexran_agent_phy_defs.h"
#include "flexran_agent_ran_api.h"
// for flexran_agent_get_phy_xface()
#include "flexran_agent_extern.h"

/**********************************
 * FlexRAN agent - technology PHY API
 **********************************/

/* Fill the PHY part of an cell_config message */
void flexran_agent_fill_phy_cell_config(mid_t mod_id, uint8_t cc_id,
    Protocol__FlexCellConfig *conf);

/* Register technology specific interface callbacks */
int flexran_agent_register_phy_xface(mid_t mod_id);

/* Unregister technology specific callbacks */
int flexran_agent_unregister_phy_xface(mid_t mod_id);

#endif
