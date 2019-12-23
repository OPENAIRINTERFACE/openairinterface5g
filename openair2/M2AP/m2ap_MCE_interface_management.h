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

/*! \file m2ap_MCE_interface_management.h
 * \brief m2ap interface management for MCE
 * \author Javier Morgade
 * \date 2019
 * \version 0.1
 * \company Vicomtech
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#ifndef M2AP_MCE_INTERFACE_MANAGEMENT_H_
#define M2AP_MCE_INTERFACE_MANAGEMENT_H_
/*
 * MBMS Session start
 */
int MCE_send_MBMS_SESSION_START_REQUEST(instance_t instance/*,
                                uint32_t assoc_id*/,m2ap_session_start_req_t* m2ap_session_start_req);
int MCE_handle_MBMS_SESSION_START_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);

int MCE_handle_MBMS_SESSION_START_FAILURE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);
/*
 * MBMS Session stop
 */
int MCE_send_MBMS_SESSION_STOP_REQUEST(instance_t instance,
                                m2ap_session_stop_req_t* m2ap_session_stop_req);

int MCE_handle_MBMS_SESSION_STOP_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);

/*
 * MBMS Scheduling Information
 */
int MCE_send_MBMS_SCHEDULING_INFORMATION(instance_t instance,
                                /*uint32_t assoc_id,*/ m2ap_mbms_scheduling_information_t * m2ap_mbms_scheduling_information );

int MCE_handle_MBMS_SCHEDULING_INFORMATION_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);



/*
 * Reset
 */
int MCE_send_RESET(instance_t instance, m2ap_reset_t * m2ap_reset);

int MCE_handle_RESET_ACKKNOWLEDGE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu);

int MCE_handle_RESET(instance_t instance,
                    uint32_t assoc_id,
                    uint32_t stream,
                    M2AP_M2AP_PDU_t *pdu);
int MCE_send_RESET_ACKNOWLEDGE(instance_t instance, M2AP_ResetAcknowledge_t *ResetAcknowledge);

/*
 * M2AP Setup
 */
int MCE_handle_M2_SETUP_REQUEST(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M2AP_M2AP_PDU_t *pdu);

int MCE_send_M2_SETUP_RESPONSE(instance_t instance,/*uint32_t assoc_id,*/  m2ap_setup_resp_t *m2ap_setup_resp);

int MCE_send_M2_SETUP_FAILURE(instance_t instance, /*uint32_t assoc_id*/ m2ap_setup_failure_t * m2ap_setup_failure);


/*
 * MCE Configuration Update
 */

int MCE_send_MCE_CONFIGURATION_UPDATE(instance_t instance, module_id_t du_mod_idP);

int MCE_handle_MCE_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu);

int MCE_handle_MCE_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu);

/*
 * ENB Configuration Update
 */


int MCE_handle_ENB_CONFIGURATION_UPDATE(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          M2AP_M2AP_PDU_t *pdu);

int MCE_send_ENB_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                    m2ap_enb_configuration_update_failure_t *m2ap_enb_configuration_update_failure);

int MCE_send_ENB_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                    m2ap_enb_configuration_update_ack_t *m2ap_enb_configuration_update_ack);
/*
 * Error Indication
 */
int MCE_handle_ERROR_INDICATION(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M2AP_M2AP_PDU_t *pdu);
int MCE_send_ERROR_INDICATION(instance_t instance, M2AP_ErrorIndication_t *ErrorIndication);


/*
 * Session Update Request
 */
int MCE_send_MBMS_SESSION_UPDATE_REQUEST(instance_t instance, m2ap_mbms_session_update_req_t * m2ap_mbms_session_update_req);

int MCE_handle_MBMS_SESSION_UPDATE_RESPONSE(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu);

int MCE_handle_MBMS_SESSION_UPDATE_FAILURE(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu);

/*
 * Service Counting Request
 */
int MCE_send_MBMS_SERVICE_COUNTING_REQUEST(instance_t instance, module_id_t du_mod_idP);

int MCE_handle_MBMS_SERVICE_COUNTING_RESPONSE(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu);

int MCE_handle_MBMS_SESSION_COUNTING_FAILURE(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu);
/*
 * Service Counting Results Report
 */

int MCE_handle_MBMS_SESSION_COUNTING_RESULTS_REPORT(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu);

/*
 * Overload Notification
 */
int MCE_handle_MBMS_OVERLOAD_NOTIFICATION(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu);





#endif /* M2AP_MCE_INTERFACE_MANAGEMENT_H_ */



