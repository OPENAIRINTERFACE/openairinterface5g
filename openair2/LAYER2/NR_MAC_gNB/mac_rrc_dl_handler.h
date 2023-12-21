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

#ifndef MAC_RRC_DL_HANDLER_H
#define MAC_RRC_DL_HANDLER_H

#include "common/platform_types.h"
#include "f1ap_messages_types.h"
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"

void f1_setup_response(const f1ap_setup_resp_t *resp);
void f1_setup_failure(const f1ap_setup_failure_t *failure);
NR_CellGroupConfig_t *clone_CellGroupConfig(const NR_CellGroupConfig_t *orig);
void ue_context_setup_request(const f1ap_ue_context_setup_t *req);
void ue_context_modification_request(const f1ap_ue_context_modif_req_t *req);
void ue_context_modification_confirm(const f1ap_ue_context_modif_confirm_t *confirm);
void ue_context_modification_refuse(const f1ap_ue_context_modif_refuse_t *refuse);
void ue_context_release_command(const f1ap_ue_context_release_cmd_t *cmd);

void dl_rrc_message_transfer(const f1ap_dl_rrc_message_t *dl_rrc);

#endif /* MAC_RRC_DL_HANDLER_H */
