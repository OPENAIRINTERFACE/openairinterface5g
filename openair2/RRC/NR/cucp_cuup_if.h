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

#ifndef CUCP_CUUP_IF_H
#define CUCP_CUUP_IF_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include "common/platform_types.h"

struct e1ap_bearer_setup_req_s;
struct e1ap_bearer_setup_resp_s;
struct e1ap_bearer_release_cmd_s;
typedef void (*cucp_cuup_bearer_context_setup_func_t)(sctp_assoc_t assoc_id, const struct e1ap_bearer_setup_req_s *req);
typedef void (*cucp_cuup_bearer_context_release_func_t)(sctp_assoc_t assoc_id, const struct e1ap_bearer_release_cmd_s *cmd);

struct gNB_RRC_INST_s;
void cucp_cuup_message_transfer_direct_init(struct gNB_RRC_INST_s *rrc);
void cucp_cuup_message_transfer_e1ap_init(struct gNB_RRC_INST_s *rrc);

#endif
