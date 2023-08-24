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

/*! \file m2ap_eNB_management_procedures.h
 * \brief m2ap tasks for eNB
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#ifndef __M2AP_ENB_MANAGEMENT_PROCEDURES__H__
#define __M2AP_ENB_MANAGEMENT_PROCEDURES__H__

void m2ap_eNB_prepare_internal_data(void);

void dump_trees_m2(void);

void m2ap_eNB_insert_new_instance(m2ap_eNB_instance_t *new_instance_p);

m2ap_eNB_instance_t *m2ap_eNB_get_instance(uint8_t mod_id);

uint16_t m2ap_eNB_fetch_add_global_cnx_id(void);

//void m2ap_eNB_prepare_internal_data(void);

m2ap_eNB_data_t* m2ap_is_eNB_id_in_list(uint32_t eNB_id);

m2ap_eNB_data_t* m2ap_is_eNB_assoc_id_in_list(uint32_t sctp_assoc_id);

m2ap_eNB_data_t* m2ap_is_eNB_pci_in_list (const uint32_t pci);

struct m2ap_eNB_data_s *m2ap_get_eNB(m2ap_eNB_instance_t *instance_p, sctp_assoc_t assoc_id, uint16_t cnx_id);

#endif /* __M2AP_ENB_MANAGEMENT_PROCEDURES__H__ */
