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

/*! \file f1ap_du_ue_context_management.h
 * \brief f1ap ue context management for DU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#ifndef F1AP_DU_UE_CONTEXT_MANAGEMENT_H_
#define F1AP_DU_UE_CONTEXT_MANAGEMENT_H_

/*
 * UE Context Setup
 */
int DU_send_UE_CONTEXT_SETUP_RESPONSE(sctp_assoc_t assoc_id, f1ap_ue_context_setup_t *req);

int DU_handle_UE_CONTEXT_SETUP_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);
int DU_send_UE_CONTEXT_SETUP_FAILURE(sctp_assoc_t assoc_id);


/*
 * UE Context Release Request (gNB-DU initiated)
 */
int DU_send_UE_CONTEXT_RELEASE_REQUEST(sctp_assoc_t assoc_id,
                                       f1ap_ue_context_release_req_t *req);


/*
 * UE Context Release Command (gNB-CU initiated)
 */
int DU_handle_UE_CONTEXT_RELEASE_COMMAND(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

/*
 * UE Context Release Complete (gNB-DU initiated)
 */
int DU_send_UE_CONTEXT_RELEASE_COMPLETE(sctp_assoc_t assoc_id, f1ap_ue_context_release_complete_t *complete);

/*
 * UE Context Modification (gNB-CU initiated)
 */
int DU_handle_UE_CONTEXT_MODIFICATION_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);
int DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(sctp_assoc_t assoc_id, f1ap_ue_context_modif_resp_t *resp);
int DU_send_UE_CONTEXT_MODIFICATION_FAILURE(sctp_assoc_t assoc_id);


/*
 * UE Context Modification Required (gNB-DU initiated)
 */
int DU_send_UE_CONTEXT_MODIFICATION_REQUIRED(sctp_assoc_t assoc_id, f1ap_ue_context_modif_required_t *required);
int DU_handle_UE_CONTEXT_MODIFICATION_CONFIRM(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);
int DU_handle_UE_CONTEXT_MODIFICATION_REFUSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

/*
 * UE Inactivity Notification
 */

/*
 * Notify
 */

#endif /* F1AP_DU_UE_CONTEXT_MANAGEMENT_H_ */
