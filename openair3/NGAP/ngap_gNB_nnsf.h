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

/*! \file ngap_gNB_nnsf.h
 * \brief ngap NAS node selection functions
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 */
 
#ifndef NGAP_GNB_NNSF_H_
#define NGAP_GNB_NNSF_H_

struct ngap_gNB_amf_data_s *
ngap_gNB_nnsf_select_amf(ngap_gNB_instance_t       *instance_p,
                         ngap_rrc_establishment_cause_t  cause);

struct ngap_gNB_amf_data_s *
ngap_gNB_nnsf_select_amf_by_plmn_id(ngap_gNB_instance_t       *instance_p,
                                    ngap_rrc_establishment_cause_t  cause,
                                    int                        selected_plmn_identity);

struct ngap_gNB_amf_data_s*
ngap_gNB_nnsf_select_amf_by_amf_setid(ngap_gNB_instance_t       *instance_p,
                                     ngap_rrc_establishment_cause_t  cause,
                                     int                        selected_plmn_identity,
                                     uint8_t                    amf_setid);

struct ngap_gNB_amf_data_s*
ngap_gNB_nnsf_select_amf_by_guami(ngap_gNB_instance_t       *instance_p,
                                   ngap_rrc_establishment_cause_t  cause,
                                   ngap_guami_t                   guami);

struct ngap_gNB_amf_data_s*
ngap_gNB_nnsf_select_amf_by_guami_no_cause(ngap_gNB_instance_t       *instance_p,
                                   ngap_guami_t                   guami);

#endif /* NGAP_GNB_NNSF_H_ */
