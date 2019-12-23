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

/*! \file m2ap_eNB_interface_management.h
 * \brief m2ap interface management for eNB
 * \author Javier Morgade
 * \date 2019
 * \version 0.1
 * \company Vicomtech
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#ifndef M2AP_ENB_INTERFACE_MANAGEMENT_H_
#define M2AP_ENB_INTERFACE_MANAGEMENT_H_

/*
 * Session Start 
 */

int eNB_handle_MBMS_SESSION_START_REQUEST(instance_t instance, 
					uint32_t assoc_id,
                                	uint32_t stream,
                                	M2AP_M2AP_PDU_t *pdu);

int eNB_send_MBMS_SESSION_START_RESPONSE(instance_t instance, m2ap_session_start_resp_t * m2ap_session_start_resp);
int eNB_send_MBMS_SESSION_START_FAILURE(instance_t instance, m2ap_session_start_failure_t * m2ap_session_start_failure  );

/*
 * Session Stop
 */

int eNB_handle_MBMS_SESSION_STOP_REQUEST(instance_t instance, 
					uint32_t assoc_id,
                                	uint32_t stream,
                                	M2AP_M2AP_PDU_t *pdu);
int eNB_send_MBMS_SESSION_STOP_RESPONSE(instance_t instance, m2ap_session_stop_resp_t * m2ap_session_stop_resp);


/*
 * MBMS Scheduling Information
 */

int eNB_handle_MBMS_SCHEDULING_INFORMATION(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);

int eNB_send_MBMS_SCHEDULING_INFORMATION_RESPONSE(instance_t instance, m2ap_mbms_scheduling_information_resp_t * m2ap_mbms_scheduling_information_resp);


/*
 * Reset
 */
int eNB_handle_RESET(instance_t instance,
                    uint32_t assoc_id,
                    uint32_t stream,
                    M2AP_M2AP_PDU_t *pdu);
int eNB_send_RESET_ACKKNOWLEDGE(instance_t instance, M2AP_ResetAcknowledge_t *ResetAcknowledge);
int eNB_send_RESET(instance_t instance, M2AP_Reset_t *Reset);
int eNB_handle_RESET_ACKNOWLEDGE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);

/*
 * M2AP Setup
 */
int eNB_send_M2_SETUP_REQUEST( m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p);

int eNB_handle_M2_SETUP_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);

int eNB_handle_M2_SETUP_FAILURE(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M2AP_M2AP_PDU_t *pdu);

/*
 * eNB Configuration Update
 */ 
int eNB_send_eNB_CONFIGURATION_UPDATE(instance_t instance, m2ap_enb_configuration_update_t * m2ap_enb_configuration_update);

int eNB_handle_eNB_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
					 uint32_t assoc_id,
                                         uint32_t stream,
                                         M2AP_M2AP_PDU_t *pdu);

int eNB_handle_eNB_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
				         uint32_t assoc_id,
                                         uint32_t stream,
                                         M2AP_M2AP_PDU_t *pdu);

/*
 * MCE Configuration Update
 */
int eNB_handle_MCE_CONFIGURATION_UPDATE(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          M2AP_M2AP_PDU_t *pdu);

int eNB_send_MCE_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                    M2AP_MCEConfigurationUpdateFailure_t *MCEConfigurationUpdateFailure);

int eNB_send_MCE_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                    M2AP_MCEConfigurationUpdateAcknowledge_t *MCEConfigurationUpdateAcknowledge);

/*
 * Error Indication
 */
int eNB_send_ERROR_INDICATION(instance_t instance, m2ap_error_indication_t * m2ap_error_indication);
int eNB_handle_ERROR_INDICATION(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M2AP_M2AP_PDU_t *pdu);


/*
 * Session Update Request
 */
int eNB_handle_MBMS_SESSION_UPDATE_REQUEST(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu);

int eNB_send_MBMS_SESSION_UPDATE_RESPONSE(instance_t instance, m2ap_mbms_session_update_resp_t * m2ap_mbms_session_update_resp); //??

int eNB_send_MBMS_SESSION_UPDATE_FAILURE(instance_t instance, m2ap_mbms_session_update_failure_t * m2ap_mbms_session_update_failure);

/*
 * Service Counting
 */ 

int eNB_handle_MBMS_SERVICE_COUNTING_REQ(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu);
int eNB_send_MBMS_SERVICE_COUNTING_REPORT(instance_t instance, m2ap_mbms_service_counting_report_t * m2ap_mbms_service_counting_report);
int eNB_send_MBMS_SERVICE_COUNTING_RESP(instance_t instance, m2ap_mbms_service_counting_resp_t * m2ap_mbms_service_counting_resp);
int eNB_send_MBMS_SERVICE_COUNTING_FAILURE(instance_t instance, m2ap_mbms_service_counting_failure_t * m2ap_mbms_service_counting_failure);

 
/* 
 * Overload Notification
 */
int eNB_send_MBMS_OVERLOAD_NOTIFICATION(instance_t instance, m2ap_mbms_overload_notification_t * m2ap_mbms_overload_notification);


#endif /* M2AP_ENB_INTERFACE_MANAGEMENT_H_ */



