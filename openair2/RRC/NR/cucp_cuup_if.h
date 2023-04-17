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
 *      conmnc_digit_lengtht@openairinterface.org
 */

#ifndef CUCP_CUUP_IF_H
#define CUCP_CUUP_IF_H

#include <netinet/in.h>
#include "platform_types.h"

struct e1ap_bearer_setup_req_s;
struct e1ap_bearer_setup_resp_s;
typedef void (*cucp_cuup_bearer_context_setup_func_t)(struct e1ap_bearer_setup_req_s *const req, instance_t instance, uint8_t xid);

struct gNB_RRC_INST_s;
void cucp_cuup_message_transfer_direct_init(struct gNB_RRC_INST_s *rrc);
void cucp_cuup_message_transfer_e1ap_init(struct gNB_RRC_INST_s *rrc);
void fill_e1ap_bearer_setup_resp(struct e1ap_bearer_setup_resp_s *resp,
                                 struct e1ap_bearer_setup_req_s *const req,
                                 instance_t gtpInst,
                                 ue_id_t ue_id,
                                 int remote_port,
                                 in_addr_t my_addr);
void CU_update_UP_DL_tunnel(struct e1ap_bearer_setup_req_s *const req, instance_t instance, ue_id_t ue_id);
#endif
