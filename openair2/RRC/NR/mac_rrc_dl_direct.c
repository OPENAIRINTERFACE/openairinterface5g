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

#include "nr_rrc_defs.h"

#include "mac_rrc_dl.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"

void mac_rrc_dl_direct_init(nr_mac_rrc_dl_if_t *mac_rrc)
{
  mac_rrc->f1_setup_response = f1_setup_response;
  mac_rrc->f1_setup_failure = f1_setup_failure;
  mac_rrc->ue_context_setup_request = ue_context_setup_request;
  mac_rrc->ue_context_modification_request = ue_context_modification_request;
  mac_rrc->ue_context_modification_confirm = ue_context_modification_confirm;
  mac_rrc->ue_context_modification_refuse = ue_context_modification_refuse;
  mac_rrc->ue_context_release_command = ue_context_release_command;
  mac_rrc->dl_rrc_message_transfer = dl_rrc_message_transfer;
}
