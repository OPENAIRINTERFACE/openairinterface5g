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

#ifndef S1AP_ENB_NAS_PROCEDURES_H_
#define S1AP_ENB_NAS_PROCEDURES_H_

int s1ap_eNB_handle_nas_downlink(sctp_assoc_t assoc_id, uint32_t stream, S1AP_S1AP_PDU_t *pdu);

int s1ap_eNB_nas_uplink(instance_t instance, s1ap_uplink_nas_t *s1ap_uplink_nas_p);

int s1ap_eNB_nas_non_delivery_ind(instance_t instance,
                                  s1ap_nas_non_delivery_ind_t *s1ap_nas_non_delivery_ind);

int s1ap_eNB_handle_nas_first_req(
  instance_t instance, s1ap_nas_first_req_t *s1ap_nas_first_req_p);

int s1ap_eNB_initial_ctxt_resp(
  instance_t instance, s1ap_initial_context_setup_resp_t *initial_ctxt_resp_p);

int s1ap_eNB_ue_capabilities(instance_t instance,
                             s1ap_ue_cap_info_ind_t *ue_cap_info_ind_p);

int s1ap_eNB_e_rab_setup_resp(instance_t instance,
                              s1ap_e_rab_setup_resp_t *e_rab_setup_resp_p);

int s1ap_eNB_e_rab_modify_resp(instance_t instance,
                               s1ap_e_rab_modify_resp_t *e_rab_modify_resp_p);

int s1ap_eNB_e_rab_release_resp(instance_t instance,
                                s1ap_e_rab_release_resp_t *e_rab_release_resp_p);

int s1ap_eNB_path_switch_req(instance_t instance,
                             s1ap_path_switch_req_t *path_switch_req_p);

int s1ap_eNB_generate_E_RAB_Modification_Indication(
		instance_t instance, s1ap_e_rab_modification_ind_t *e_rab_modification_ind);

#endif /* S1AP_ENB_NAS_PROCEDURES_H_ */
