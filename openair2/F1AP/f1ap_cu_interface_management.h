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

/*! \file f1ap_cu_interface_management.h
 * \brief f1ap interface management for CU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#ifndef F1AP_CU_INTERFACE_MANAGEMENT_H_
#define F1AP_CU_INTERFACE_MANAGEMENT_H_

/*
 * Reset
 */
int CU_send_RESET(sctp_assoc_t assoc_id, F1AP_Reset_t *Reset);
int CU_handle_RESET_ACKKNOWLEDGE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);
int CU_handle_RESET(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);
int CU_send_RESET_ACKNOWLEDGE(sctp_assoc_t assoc_id, F1AP_ResetAcknowledge_t *ResetAcknowledge);

/*
 * Error Indication
 */
int CU_handle_ERROR_INDICATION(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);
int CU_send_ERROR_INDICATION(sctp_assoc_t assoc_id, F1AP_ErrorIndication_t *ErrorIndication);

/*
 * F1 Setup
 */
int CU_handle_F1_SETUP_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

int CU_send_F1_SETUP_RESPONSE(sctp_assoc_t assoc_id, f1ap_setup_resp_t *f1ap_setup_resp);

int CU_send_F1_SETUP_FAILURE(sctp_assoc_t assoc_id, const f1ap_setup_failure_t *fail);

/*
 * gNB-DU Configuration Update
 */
int CU_handle_gNB_DU_CONFIGURATION_UPDATE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

int CU_send_gNB_DU_CONFIGURATION_FAILURE(sctp_assoc_t assoc_id,
    F1AP_GNBDUConfigurationUpdateFailure_t *GNBDUConfigurationUpdateFailure);

int CU_send_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(sctp_assoc_t assoc_id,
    F1AP_GNBDUConfigurationUpdateAcknowledge_t *GNBDUConfigurationUpdateAcknowledge);

/*
 * gNB-CU Configuration Update
 */
int CU_send_gNB_CU_CONFIGURATION_UPDATE(sctp_assoc_t assoc_id, f1ap_gnb_cu_configuration_update_t *f1ap_gnb_cu_configuration_update);
int CU_handle_gNB_CU_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                                                  sctp_assoc_t assoc_id,
                                                  uint32_t stream,
                                                  F1AP_F1AP_PDU_t *pdu);

int CU_handle_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                                      sctp_assoc_t assoc_id,
                                                      uint32_t stream,
                                                      F1AP_F1AP_PDU_t *pdu);

/*
 * gNB-DU Resource Coordination
 */
int CU_handle_gNB_DU_RESOURCE_COORDINATION_REQUEST(instance_t instance,
                                                   sctp_assoc_t assoc_id,
                                                   uint32_t stream,
                                                   F1AP_F1AP_PDU_t *pdu);

int CU_send_gNB_DU_RESOURCE_COORDINATION_RESPONSE(sctp_assoc_t assoc_id,
    F1AP_GNBDUResourceCoordinationResponse_t *GNBDUResourceCoordinationResponse);

#endif /* F1AP_CU_INTERFACE_MANAGEMENT_H_ */
