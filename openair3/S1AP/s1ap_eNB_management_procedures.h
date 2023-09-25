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

#ifndef S1AP_ENB_MANAGEMENT_PROCEDURES_H_
#define S1AP_ENB_MANAGEMENT_PROCEDURES_H_

struct s1ap_eNB_mme_data_s *s1ap_eNB_get_MME(s1ap_eNB_instance_t *instance_p, sctp_assoc_t assoc_id, uint16_t cnx_id);

struct s1ap_eNB_mme_data_s *s1ap_eNB_get_MME_from_instance(s1ap_eNB_instance_t *instance_p);

void s1ap_eNB_remove_mme_desc(s1ap_eNB_instance_t * instance);

void s1ap_eNB_insert_new_instance(s1ap_eNB_instance_t *new_instance_p);

s1ap_eNB_instance_t *s1ap_eNB_get_instance(uint8_t mod_id);

uint16_t s1ap_eNB_fetch_add_global_cnx_id(void);

void s1ap_eNB_prepare_internal_data(void);

#endif /* S1AP_ENB_MANAGEMENT_PROCEDURES_H_ */
