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

/*! \file ngap_gNB_context_management_procedures.h
 * \brief NGAP context management procedures
 * \author  Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */

#ifndef NGAP_GNB_CONTEXT_MANAGEMENT_PROCEDURES_H_
#define NGAP_GNB_CONTEXT_MANAGEMENT_PROCEDURES_H_


int ngap_ue_context_release_complete(instance_t instance,
                                     ngap_ue_release_complete_t *ue_release_complete_p);

int ngap_ue_context_release_req(instance_t instance,
                                ngap_ue_release_req_t *ue_release_req_p);

#endif /* NGAP_GNB_CONTEXT_MANAGEMENT_PROCEDURES_H_ */
