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

#include "ran_e2sm_ue_id.h"

ue_id_e2sm_t fill_e2sm_gnb_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, __attribute__((unused))const uint32_t rrc_ue_id, __attribute__((unused))const ue_id_t cucp_ue_id)
{
  ue_id_e2sm_t ue_id = {0};

  ue_id.type = GNB_UE_ID_E2SM;

  // 6.2.3.16
  // Mandatory
  // AMF UE NGAP ID
  ue_id.gnb.amf_ue_ngap_id = rrc_ue_context->amf_ue_ngap_id;

  // Mandatory
  //GUAMI 6.2.3.17 
  ue_id.gnb.guami.plmn_id = (e2sm_plmn_t) {
                                    .mcc = rrc_ue_context->ue_guami.mcc,
                                    .mnc = rrc_ue_context->ue_guami.mnc,
                                    .mnc_digit_len = rrc_ue_context->ue_guami.mnc_len
                                    };
  
  ue_id.gnb.guami.amf_region_id = rrc_ue_context->ue_guami.amf_region_id;
  ue_id.gnb.guami.amf_set_id = rrc_ue_context->ue_guami.amf_set_id;
  ue_id.gnb.guami.amf_ptr = rrc_ue_context->ue_guami.amf_pointer;

  // RAN UE ID
  // Optional
  // 6.2.3.25
  // OCTET STRING (SIZE (8))
  // Defined in TS 38.473 (F1AP) 
  // clause 9.2.2.1
  // UE CONTEXT SETUP REQUEST
  ue_id.gnb.ran_ue_id = calloc(1, sizeof(uint64_t));
  assert(ue_id.gnb.ran_ue_id != NULL);
  *ue_id.gnb.ran_ue_id = rrc_ue_context->rrc_ue_id;

  return ue_id;
}

ue_id_e2sm_t fill_e2sm_cu_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id)
{
  ue_id_e2sm_t ue_id = fill_e2sm_gnb_ue_id_data(rrc_ue_context, rrc_ue_id, cucp_ue_id);

  // gNB-CU UE F1AP ID List
  // C-ifCUDUseparated
  ue_id.gnb.gnb_cu_ue_f1ap_lst_len = 1;
  ue_id.gnb.gnb_cu_ue_f1ap_lst = calloc(ue_id.gnb.gnb_cu_ue_f1ap_lst_len, sizeof(uint32_t));
  assert(ue_id.gnb.gnb_cu_ue_f1ap_lst != NULL && "Memory exhausted");
  ue_id.gnb.gnb_cu_ue_f1ap_lst[0] = rrc_ue_context->rrc_ue_id;

  return ue_id;
}

ue_id_e2sm_t fill_e2sm_du_ue_id_data(__attribute__((unused))const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, __attribute__((unused))const ue_id_t cucp_ue_id)
{
  ue_id_e2sm_t ue_id = {0};

  ue_id.type = GNB_DU_UE_ID_E2SM;

  // 6.2.3.21
  // gNB CU UE F1AP
  // Mandatory
  ue_id.gnb_du.gnb_cu_ue_f1ap = rrc_ue_id;

  // 6.2.3.25
  // RAN UE ID
  // Optional
  ue_id.gnb_du.ran_ue_id = calloc(1, sizeof(uint64_t));
  assert(ue_id.gnb_du.ran_ue_id != NULL);
  *ue_id.gnb_du.ran_ue_id = rrc_ue_id;

  return ue_id;
}

ue_id_e2sm_t fill_e2sm_cucp_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context, const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id)
{
  ue_id_e2sm_t ue_id = fill_e2sm_gnb_ue_id_data(rrc_ue_context, rrc_ue_id, cucp_ue_id);

  //gNB-CU-CP UE E1AP ID List
  //C-ifCPUPseparated 
  ue_id.gnb.gnb_cu_cp_ue_e1ap_lst_len = 1;
  ue_id.gnb.gnb_cu_cp_ue_e1ap_lst = calloc(ue_id.gnb.gnb_cu_cp_ue_e1ap_lst_len, sizeof(uint32_t));
  assert(ue_id.gnb.gnb_cu_cp_ue_e1ap_lst != NULL && "Memory exhausted");
  ue_id.gnb.gnb_cu_cp_ue_e1ap_lst[0] = rrc_ue_context->rrc_ue_id;

  return ue_id;
}

ue_id_e2sm_t fill_e2sm_cuup_ue_id_data(__attribute__((unused))const gNB_RRC_UE_t *rrc_ue_context, __attribute__((unused))const uint32_t rrc_ue_id, const ue_id_t cucp_ue_id)
{
  ue_id_e2sm_t ue_id = {0};

  ue_id.type = GNB_CU_UP_UE_ID_E2SM;

  // 6.2.3.20
  // Mandatory
  ue_id.gnb_cu_up.gnb_cu_cp_ue_e1ap = cucp_ue_id;  // RAN UE NGAP ID = gNB UE NGAP ID = rrc_ue_id

  // 6.2.3.25
  // RAN UE ID
  // Optional
  ue_id.gnb_cu_up.ran_ue_id = calloc(1, sizeof(uint64_t));
  assert(ue_id.gnb_cu_up.ran_ue_id != NULL);
  *ue_id.gnb_cu_up.ran_ue_id = cucp_ue_id;  // RAN UE NGAP ID = gNB UE NGAP ID = rrc_ue_id

  return ue_id;
}

get_ue_id fill_ue_id_data[END_NGRAN_NODE_TYPE] =
{
  NULL,
  NULL,
  fill_e2sm_gnb_ue_id_data,
  NULL,
  NULL,
  fill_e2sm_cu_ue_id_data,
  NULL,
  fill_e2sm_du_ue_id_data,
  NULL,
  fill_e2sm_cucp_ue_id_data,
  fill_e2sm_cuup_ue_id_data,
};
