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
 * Author and copyright: Laurent Thomas, open-cells.com
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

#ifndef E1AP_API_H
#define E1AP_API_H

#include "platform_types.h"
#include "openair2/COMMON/e1ap_messages_types.h"
#include "openair2/E1AP/e1ap_common.h"
void cuup_init_n3(instance_t instance);
void process_e1_bearer_context_setup_req(instance_t, e1ap_bearer_setup_req_t *const req);
void CUUP_process_bearer_context_mod_req(instance_t, e1ap_bearer_setup_req_t *const req);

void CUUP_process_bearer_release_command(instance_t, e1ap_bearer_release_cmd_t *const cmd);
#endif
