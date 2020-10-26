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

/*! \file x2ap_eNB_generate_messages.h
 * \brief x2ap procedures for eNB
 * \author Konstantinos Alexandris <Konstantinos.Alexandris@eurecom.fr>, Cedric Roux <Cedric.Roux@eurecom.fr>, Navid Nikaein <Navid.Nikaein@eurecom.fr>
 * \date 2018
 * \version 1.0
 */

#ifndef X2AP_ENB_GENERATE_MESSAGES_H_
#define X2AP_ENB_GENERATE_MESSAGES_H_

#include "x2ap_eNB_defs.h"
#include "x2ap_common.h"

int x2ap_eNB_generate_x2_setup_request(x2ap_eNB_instance_t *instance_p,
				       x2ap_eNB_data_t *x2ap_eNB_data_p);

int x2ap_eNB_generate_x2_setup_response(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p);

int x2ap_eNB_generate_x2_setup_failure(instance_t instance,
                                       uint32_t assoc_id,
                                       X2AP_Cause_PR cause_type,
                                       long cause_value,
                                       long time_to_wait);

int x2ap_eNB_set_cause (X2AP_Cause_t * cause_p,
                        X2AP_Cause_PR cause_type,
                        long cause_value);

int x2ap_eNB_generate_x2_handover_request (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                           x2ap_handover_req_t *x2ap_handover_req, int ue_id);

int x2ap_eNB_generate_x2_handover_request_ack (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                               x2ap_handover_req_ack_t *x2ap_handover_req_ack);

int x2ap_eNB_generate_x2_ue_context_release (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                             x2ap_ue_context_release_t *x2ap_ue_context_release);

int x2ap_eNB_generate_x2_handover_cancel (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                          int x2_ue_id,
                                          x2ap_handover_cancel_cause_t cause);

int x2ap_eNB_generate_senb_addition_request (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p
                                             /* TODO: pass needed parameters */);

int x2ap_eNB_generate_senb_addition_request_ack (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                               x2ap_senb_addition_req_ack_t *x2ap_addition_req_ack);

int x2ap_gNB_generate_ENDC_x2_setup_request(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p);

int x2ap_eNB_generate_ENDC_x2_setup_response( x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p);

int x2ap_eNB_generate_ENDC_x2_SgNB_addition_request( x2ap_eNB_instance_t *instance_p, x2ap_ENDC_sgnb_addition_req_t *x2ap_ENDC_sgnb_addition_req, x2ap_eNB_data_t *x2ap_eNB_data_p, int ue_id);

int x2ap_gNB_generate_ENDC_x2_SgNB_addition_request_ACK( x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p, x2ap_ENDC_sgnb_addition_req_ACK_t *x2ap_sgnb_addition_req_ACK, int ue_id);

int x2ap_eNB_generate_ENDC_x2_SgNB_reconfiguration_complete(
  x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p, int ue_id, int SgNB_ue_id);

int x2ap_eNB_generate_ENDC_x2_SgNB_release_request(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                                   int x2_id_source, int x2_id_target, x2ap_cause_t cause);

int x2ap_gNB_generate_ENDC_x2_SgNB_release_request_acknowledge(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                                               int menb_ue_x2ap_id, int sgnb_ue_x2ap_id);

int x2ap_eNB_generate_ENDC_x2_SgNB_release_required(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                                    int x2_id_source, int x2_id_target, x2ap_cause_t cause);

int x2ap_gNB_generate_ENDC_x2_SgNB_release_confirm(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                                   int menb_ue_x2ap_id, int sgnb_ue_x2ap_id);

#endif /*  X2AP_ENB_GENERATE_MESSAGES_H_ */
