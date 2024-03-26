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

#ifndef RAN_FUNC_SM_KPM_SUBSCRIPTION_AGENT_H
#define RAN_FUNC_SM_KPM_SUBSCRIPTION_AGENT_H

#include "openair2/E2AP/flexric/src/sm/kpm_sm/kpm_data_ie_wrapper.h"

#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"

typedef struct {
  uint32_t rrc_ue_id;
  NR_UE_info_t* ue;

} cudu_ue_info_pair_t;

typedef meas_record_lst_t (*kpm_meas_fp)(uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx);

typedef struct{ 
  char* key; 
  kpm_meas_fp value;
} kv_measure_t;


void init_kpm_subs_data(void);

meas_record_lst_t get_kpm_meas_value(char* kpm_meas_name, uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx);

#endif
