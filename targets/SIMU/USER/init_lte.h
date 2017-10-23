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

#include "PHY/types.h"
#include "PHY/defs.h"

PHY_VARS_eNB* init_lte_eNB(LTE_DL_FRAME_PARMS *frame_parms,
                           uint8_t eNB_id,
                           uint8_t Nid_cell,
			   node_function_t node_function,
                           uint8_t abstraction_flag);

PHY_VARS_UE* init_lte_UE(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t UE_id,
                         uint8_t abstraction_flag);

PHY_VARS_RN* init_lte_RN(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t RN_id,
                         uint8_t eMBMS_active_state);

void init_lte_vars(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
                   uint8_t frame_type,
                   uint8_t tdd_config,
                   uint8_t tdd_config_S,
                   uint8_t extended_prefix_flag,
                   uint8_t N_RB_DL,
                   uint16_t Nid_cell,
                   uint8_t cooperation_flag,
                   uint8_t nb_antenna_ports,
                   uint8_t abstraction_flag,
                   int nb_antennas_rx,
                   int nb_antennas_tx,
                   int nb_antennas_rx_ue,
                   uint8_t eMBMS_active_state);
