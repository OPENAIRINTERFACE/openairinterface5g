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

#ifndef NGAP_GNB_NNSF_H_
#define NGAP_GNB_NNSF_H_

struct ngap_gNB_mme_data_s *
ngap_gNB_nnsf_select_mme(ngap_gNB_instance_t       *instance_p,
                         rrc_establishment_cause_t  cause);

struct ngap_gNB_mme_data_s *
ngap_gNB_nnsf_select_mme_by_plmn_id(ngap_gNB_instance_t       *instance_p,
                                    rrc_establishment_cause_t  cause,
                                    int                        selected_plmn_identity);

struct ngap_gNB_mme_data_s*
ngap_gNB_nnsf_select_mme_by_mme_code(ngap_gNB_instance_t       *instance_p,
                                     rrc_establishment_cause_t  cause,
                                     int                        selected_plmn_identity,
                                     uint8_t                    mme_code);

struct ngap_gNB_mme_data_s*
ngap_gNB_nnsf_select_mme_by_gummei(ngap_gNB_instance_t       *instance_p,
                                   rrc_establishment_cause_t  cause,
                                   ngap_gummei_t                   gummei);

struct ngap_gNB_mme_data_s*
ngap_gNB_nnsf_select_mme_by_gummei_no_cause(ngap_gNB_instance_t       *instance_p,
                                   ngap_gummei_t                   gummei);

#endif /* NGAP_GNB_NNSF_H_ */
