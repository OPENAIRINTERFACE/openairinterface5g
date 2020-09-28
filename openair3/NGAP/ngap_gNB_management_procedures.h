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

/*! \file ngap_gNB_management_procedures.h
 * \brief NGAP gNB task 
 * \author  Yoshio INOUE, Masayuki HARADA 
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */

#ifndef NGAP_GNB_MANAGEMENT_PROCEDURES_H_
#define NGAP_GNB_MANAGEMENT_PROCEDURES_H_

struct ngap_gNB_amf_data_s *ngap_gNB_get_AMF(
  ngap_gNB_instance_t *instance_p,
  int32_t assoc_id, uint16_t cnx_id);

struct ngap_gNB_amf_data_s *ngap_gNB_get_AMF_from_instance(ngap_gNB_instance_t *instance_p);

void ngap_gNB_remove_amf_desc(ngap_gNB_instance_t * instance);

void ngap_gNB_insert_new_instance(ngap_gNB_instance_t *new_instance_p);

ngap_gNB_instance_t *ngap_gNB_get_instance(uint8_t mod_id);

uint16_t ngap_gNB_fetch_add_global_cnx_id(void);

void ngap_gNB_prepare_internal_data(void);

#endif /* NGAP_GNB_MANAGEMENT_PROCEDURES_H_ */
