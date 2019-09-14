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

/*! \file m3ap_eNB_generate_messages.h
 * \brief m3ap procedures for eNB
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#ifndef M3AP_ENB_GENERATE_MESSAGES_H_
#define M3AP_ENB_GENERATE_MESSAGES_H_

#include "m3ap_eNB_defs.h"
#include "m3ap_common.h"

int m3ap_eNB_generate_m3_setup_request(m3ap_eNB_instance_t *instance_p,
				       m3ap_eNB_data_t *m3ap_eNB_data_p);

int m3ap_eNB_generate_m3_setup_response(m3ap_eNB_instance_t *instance_p, m3ap_eNB_data_t *m3ap_eNB_data_p);

int m3ap_eNB_generate_m3_setup_failure(instance_t instance,
                                       uint32_t assoc_id,
                                       M3AP_Cause_PR cause_type,
                                       long cause_value,
                                       long time_to_wait);

int m3ap_eNB_set_cause (M3AP_Cause_t * cause_p,
                        M3AP_Cause_PR cause_type,
                        long cause_value);

//int m3ap_eNB_generate_m2_handover_request (m3ap_eNB_instance_t *instance_p, m3ap_eNB_data_t *m3ap_eNB_data_p,
//                                           m3ap_handover_req_t *m3ap_handover_req, int ue_id);
//
//int m3ap_eNB_generate_m2_handover_request_ack (m3ap_eNB_instance_t *instance_p, m3ap_eNB_data_t *m3ap_eNB_data_p,
//                                               m3ap_handover_req_ack_t *m3ap_handover_req_ack);
//
//int m3ap_eNB_generate_m2_ue_context_release (m3ap_eNB_instance_t *instance_p, m3ap_eNB_data_t *m3ap_eNB_data_p,
//                                             m3ap_ue_context_release_t *m3ap_ue_context_release);
//
//int m3ap_eNB_generate_m2_handover_cancel (m3ap_eNB_instance_t *instance_p, m3ap_eNB_data_t *m3ap_eNB_data_p,
//                                          int m2_ue_id,
//                                          m3ap_handover_cancel_cause_t cause);

#endif /*  M3AP_ENB_GENERATE_MESSAGES_H_ */
