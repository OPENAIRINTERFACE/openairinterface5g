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

/*! \file m3ap_MME_interface_management.h
 * \brief m3ap interface management for MME
 * \author Javier Morgade
 * \date 2019
 * \version 0.1
 * \company Vicomtech
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#ifndef M3AP_MME_INTERFACE_MANAGEMENT_H_
#define M3AP_MME_INTERFACE_MANAGEMENT_H_
/*
 * MBMS Session start
 */
int MME_send_MBMS_SESSION_START_REQUEST(instance_t instance/*,
                                uint32_t assoc_id*/,m3ap_session_start_req_t* m3ap_session_start_req);
int MME_handle_MBMS_SESSION_START_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M3AP_M3AP_PDU_t *pdu);

int MME_handle_MBMS_SESSION_START_FAILURE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M3AP_M3AP_PDU_t *pdu);
/*
 * MBMS Session stop
 */
int MME_send_MBMS_SESSION_STOP_REQUEST(instance_t instance,
                                m3ap_session_stop_req_t* m3ap_session_stop_req);

int MME_handle_MBMS_SESSION_STOP_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M3AP_M3AP_PDU_t *pdu);

/*
 * Update 
 */
int MME_handle_MBMS_SESSION_UPDATE_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M3AP_M3AP_PDU_t *pdu);

int MME_handle_MBMS_SESSION_UPDATE_FAILURE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M3AP_M3AP_PDU_t *pdu);



/*
 * Reset
 */
int MME_send_RESET(instance_t instance, m3ap_reset_t * m3ap_reset);

int MME_handle_RESET_ACKKNOWLEDGE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M3AP_M3AP_PDU_t *pdu);

int MME_handle_RESET(instance_t instance,
                    uint32_t assoc_id,
                    uint32_t stream,
                    M3AP_M3AP_PDU_t *pdu);
int MME_send_RESET_ACKNOWLEDGE(instance_t instance, M3AP_ResetAcknowledge_t *ResetAcknowledge);

/*
 * M3AP Setup
 */
int MME_handle_M3_SETUP_REQUEST(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M3AP_M3AP_PDU_t *pdu);

int MME_send_M3_SETUP_RESPONSE(instance_t instance,/*uint32_t assoc_id,*/  m3ap_setup_resp_t *m3ap_setup_resp);

int MME_send_M3_SETUP_FAILURE(instance_t instance, /*uint32_t assoc_id*/ m3ap_setup_failure_t * m3ap_setup_failure);






#endif /* M3AP_MME_INTERFACE_MANAGEMENT_H_ */



