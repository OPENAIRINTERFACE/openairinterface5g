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

/*! \file m3ap_MCE_management_procedures.h
 * \brief m3ap tasks for MCE
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#ifndef M3AP_MCE_MANAGEMENT_PROCEDURES_H_
#define M3AP_MCE_MANAGEMENT_PROCEDURES_H

void m3ap_MCE_prepare_internal_data(void);

void dump_trees_m3(void);

void m3ap_MCE_insert_new_instance(m3ap_MCE_instance_t *new_instance_p);

m3ap_MCE_instance_t *m3ap_MCE_get_instance(uint8_t mod_id);

uint16_t m3ap_MCE_fetch_add_global_cnx_id(void);


m3ap_MCE_data_t* m3ap_is_MCE_id_in_list(uint32_t MCE_id);

m3ap_MCE_data_t* m3ap_is_MCE_assoc_id_in_list(uint32_t sctp_assoc_id);

m3ap_MCE_data_t* m3ap_is_MCE_pci_in_list (const uint32_t pci);

struct m3ap_MCE_data_s *m3ap_get_MCE(m3ap_MCE_instance_t *instance_p,
                                     int32_t assoc_id,
                                     uint16_t cnx_id);

#endif /* M3AP_MCE_MANAGEMENT_PROCEDURES_H_ */
