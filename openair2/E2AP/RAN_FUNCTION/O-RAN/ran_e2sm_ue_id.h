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

#ifndef RAN_E2SM_UE_ID_H
#define RAN_E2SM_UE_ID_H

#include "openair2/E2AP/flexric/src/agent/../sm/sm_io.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"

ue_id_e2sm_t fill_e2sm_gnb_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id);

ue_id_e2sm_t fill_e2sm_cu_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id);

ue_id_e2sm_t fill_e2sm_du_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id);

ue_id_e2sm_t fill_e2sm_cucp_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id);

ue_id_e2sm_t fill_e2sm_cuup_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id);

typedef ue_id_e2sm_t (*get_ue_id)(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id);

extern get_ue_id fill_ue_id_data[END_NGRAN_NODE_TYPE];

#endif
