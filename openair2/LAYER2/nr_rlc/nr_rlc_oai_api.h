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

/*! \file       nr_rlc_oai_api.h
 * \brief       Header file for nr_rlc_oai_api
 * \author      Guido Casati
 * \date        2020
 * \email:      guido.casati@iis.fraunhofe.de
 * \version     1.0
 * @ingroup     _rlc

 */

#include "NR_RLC-BearerConfig.h"
#include "NR_RLC-Config.h"
#include "NR_LogicalChannelIdentity.h"
#include "NR_RadioBearerConfig.h"
#include "NR_CellGroupConfig.h"
#include "openair2/RRC/NR/nr_rrc_proto.h"

/* from OAI */
#include "pdcp.h"

struct NR_RLC_Config;
struct NR_LogicalChannelConfig;

void nr_rlc_bearer_init(NR_RLC_BearerConfig_t *RLC_BearerConfig, NR_RLC_BearerConfig__servedRadioBearer_PR rb_type);

void nr_drb_config(struct NR_RLC_Config *rlc_Config, NR_RLC_Config_PR rlc_config_pr);

void nr_rlc_bearer_init_ul_spec(struct NR_LogicalChannelConfig *mac_LogicalChannelConfig);