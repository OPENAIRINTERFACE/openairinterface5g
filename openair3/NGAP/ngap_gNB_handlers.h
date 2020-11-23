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
 
/*! \file ngap_gNB_handlers.h
 * \brief ngap messages handlers for gNB part
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */
 
#ifndef NGAP_GNB_HANDLERS_H_
#define NGAP_GNB_HANDLERS_H_

void ngap_handle_ng_setup_message(ngap_gNB_amf_data_t *amf_desc_p, int sctp_shutdown);

int ngap_gNB_handle_message(uint32_t assoc_id, int32_t stream,
                            const uint8_t * const data, const uint32_t data_length);

#endif /* NGAP_GNB_HANDLERS_H_ */
