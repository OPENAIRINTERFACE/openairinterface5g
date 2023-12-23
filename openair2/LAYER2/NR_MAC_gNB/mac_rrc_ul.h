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

#ifndef MAC_RRC_UL_H
#define MAC_RRC_UL_H

#include "common/platform_types.h"
#include "f1ap_messages_types.h"

typedef void (*f1_setup_request_func_t)(const f1ap_setup_req_t* req);

typedef void (*ue_context_setup_response_func_t)(const f1ap_ue_context_setup_t* req, const f1ap_ue_context_setup_t *resp);
typedef void (*ue_context_modification_response_func_t)(const f1ap_ue_context_modif_req_t *req,
                                                        const f1ap_ue_context_modif_resp_t *resp);
typedef void (*ue_context_modification_required_func_t)(const f1ap_ue_context_modif_required_t *t);
typedef void (*ue_context_release_request_func_t)(const f1ap_ue_context_release_req_t* req);
typedef void (*ue_context_release_complete_func_t)(const f1ap_ue_context_release_complete_t *complete);

typedef void (*initial_ul_rrc_message_transfer_func_t)(module_id_t module_id, const f1ap_initial_ul_rrc_message_t *ul_rrc);

struct nr_mac_rrc_ul_if_s;
void mac_rrc_ul_direct_init(struct nr_mac_rrc_ul_if_s *mac_rrc);
void mac_rrc_ul_f1ap_init(struct nr_mac_rrc_ul_if_s *mac_rrc);

#endif /* MAC_RRC_UL_H */
