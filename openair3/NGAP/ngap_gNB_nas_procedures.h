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
 
/*! \file ngap_gNB_nas_procedures.h
 * \brief NGAP gNb NAS procedure handler
 * \author  Yoshio INOUE, Masayuki HARADA 
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */

#ifndef NGAP_GNB_NAS_PROCEDURES_H_
#define NGAP_GNB_NAS_PROCEDURES_H_

int ngap_gNB_handle_nas_downlink(sctp_assoc_t assoc_id, uint32_t stream, NGAP_NGAP_PDU_t *pdu);

int ngap_gNB_nas_uplink(instance_t instance, ngap_uplink_nas_t *ngap_uplink_nas_p);

int ngap_gNB_nas_non_delivery_ind(instance_t instance,
                                  ngap_nas_non_delivery_ind_t *ngap_nas_non_delivery_ind);

int ngap_gNB_handle_nas_first_req(
  instance_t instance, ngap_nas_first_req_t *ngap_nas_first_req_p);

int ngap_gNB_initial_ctxt_resp(
  instance_t instance, ngap_initial_context_setup_resp_t *initial_ctxt_resp_p);

int ngap_gNB_ue_capabilities(instance_t instance,
                             ngap_ue_cap_info_ind_t *ue_cap_info_ind_p);

int ngap_gNB_pdusession_setup_resp(instance_t instance,
                              ngap_pdusession_setup_resp_t *pdusession_setup_resp_p);

int ngap_gNB_pdusession_modify_resp(instance_t instance,
                               ngap_pdusession_modify_resp_t *pdusession_modify_resp_p);

int ngap_gNB_pdusession_release_resp(instance_t instance,
                                ngap_pdusession_release_resp_t *pdusession_release_resp_p);

int ngap_gNB_path_switch_req(instance_t instance,
                             ngap_path_switch_req_t *path_switch_req_p);

int ngap_gNB_generate_PDUSESSION_Modification_Indication(
		instance_t instance, ngap_pdusession_modification_ind_t *pdusession_modification_ind);

#endif /* NGAP_GNB_NAS_PROCEDURES_H_ */
