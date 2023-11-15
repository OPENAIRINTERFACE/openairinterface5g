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

/*! \file f1ap_cu_ue_context_management.h
 * \brief header file of CU UE Context management
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#ifndef F1AP_CU_UE_CONTEXT_MANAGEMENT_H_
#define F1AP_CU_UE_CONTEXT_MANAGEMENT_H_

/*
 * UE Context Setup
 */
int CU_send_UE_CONTEXT_SETUP_REQUEST(sctp_assoc_t assoc_id,
                                     f1ap_ue_context_setup_t *f1ap_ue_context_setup_req);

int CU_handle_UE_CONTEXT_SETUP_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

int CU_handle_UE_CONTEXT_SETUP_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

/*
 * UE Context Release Request (gNB-DU initiated)
 */
int CU_handle_UE_CONTEXT_RELEASE_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

/*
 * UE Context Release (gNB-CU initiated)
 */
int CU_send_UE_CONTEXT_RELEASE_COMMAND(sctp_assoc_t assoc_id,
                                       f1ap_ue_context_release_cmd_t *cmd);

int CU_handle_UE_CONTEXT_RELEASE_COMPLETE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

/*
 * UE Context Modification (gNB-CU initiated)
 */
int CU_send_UE_CONTEXT_MODIFICATION_REQUEST(sctp_assoc_t assoc_id, f1ap_ue_context_modif_req_t *f1ap_ue_context_modification_req);
int CU_handle_UE_CONTEXT_MODIFICATION_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);
int CU_handle_UE_CONTEXT_MODIFICATION_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

/*
 * UE Context Modification Required (gNB-DU initiated)
 */
int CU_handle_UE_CONTEXT_MODIFICATION_REQUIRED(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

int CU_send_UE_CONTEXT_MODIFICATION_CONFIRM(sctp_assoc_t assoc_id, f1ap_ue_context_modif_confirm_t *confirm);

int CU_send_UE_CONTEXT_MODIFICATION_REFUSE(sctp_assoc_t assoc_id, f1ap_ue_context_modif_refuse_t *refuse);

/*
 * UE Inactivity Notification
 */

/*
 * Notify
 */

#endif /* F1AP_CU_UE_CONTEXT_MANAGEMENT_H_ */
