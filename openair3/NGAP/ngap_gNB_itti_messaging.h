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
 
/*! \file ngap_gNB_itti_messaging.h
 * \brief ngap itti messages handlers for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */

#ifndef NGAP_GNB_ITTI_MESSAGING_H_
#define NGAP_GNB_ITTI_MESSAGING_H_

void ngap_gNB_itti_send_sctp_data_req(instance_t instance,
                                      sctp_assoc_t assoc_id,
                                      uint8_t *buffer,
                                      uint32_t buffer_length,
                                      uint16_t stream);

void ngap_gNB_itti_send_nas_downlink_ind(instance_t instance, uint32_t gNB_ue_ngap_id, uint8_t *nas_pdu, uint32_t nas_pdu_length);

void ngap_gNB_itti_send_sctp_close_association(instance_t instance, sctp_assoc_t assoc_id);

#endif /* NGAP_GNB_ITTI_MESSAGING_H_ */
