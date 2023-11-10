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

#ifndef CUUP_CUCP_IF_H
#define CUUP_CUCP_IF_H

#include <stdbool.h>

struct e1ap_bearer_setup_resp_s;
struct e1ap_bearer_modif_resp_s;
struct e1ap_bearer_release_cplt_s;
typedef void (*e1_bearer_setup_response_func_t)(const struct e1ap_bearer_setup_resp_s *resp);
typedef void (*e1_bearer_modif_response_func_t)(const struct e1ap_bearer_modif_resp_s *resp);
typedef void (*e1_bearer_release_complete_func_t)(const struct e1ap_bearer_release_cplt_s *cplt);

typedef struct e1_if_t {
  e1_bearer_setup_response_func_t bearer_setup_response;
  e1_bearer_modif_response_func_t bearer_modif_response;
  e1_bearer_release_complete_func_t bearer_release_complete;
} e1_if_t;

e1_if_t *get_e1_if(void);
bool e1_used(void);
void nr_pdcp_e1_if_init(bool uses_e1);

void cuup_cucp_init_direct(e1_if_t *iface);
void cuup_cucp_init_e1ap(e1_if_t *iface);

#endif /* CUUP_CUCP_IF_H */
