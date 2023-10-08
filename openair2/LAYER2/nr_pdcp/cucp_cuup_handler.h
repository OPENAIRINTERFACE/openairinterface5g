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

#ifndef CUCP_CUUP_HANDLER_H
#define CUCP_CUUP_HANDLER_H

#include <stdbool.h>

void nr_pdcp_e1_if_init(bool uses_e1);

struct e1ap_bearer_setup_req_s;
struct e1ap_bearer_release_cmd_s;
void e1_bearer_context_setup(const struct e1ap_bearer_setup_req_s *req);
void e1_bearer_context_modif(const struct e1ap_bearer_setup_req_s *req);
void e1_bearer_release_cmd(const struct e1ap_bearer_release_cmd_s *cmd);

#endif /* CUCP_CUUP_HANDLER_H */
