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

#ifndef S1AP_ENB_OVERLOAD_H_
#define S1AP_ENB_OVERLOAD_H_

/**
 * \brief Handle an overload start message
 **/
// int s1ap_eNB_handle_overload_start(eNB_mme_desc_t *eNB_desc_p,
//                                    sctp_queue_item_t *packet_p,
//                                    struct s1ap_message_s *message_p);
int s1ap_eNB_handle_overload_start(sctp_assoc_t assoc_id, uint32_t stream, S1AP_S1AP_PDU_t *pdu);

/**
 * \brief Handle an overload stop message
 **/
// int s1ap_eNB_handle_overload_stop(eNB_mme_desc_t *eNB_desc_p,
//                                   sctp_queue_item_t *packet_p,
//                                   struct s1ap_message_s *message_p);
int s1ap_eNB_handle_overload_stop(sctp_assoc_t assoc_id, uint32_t stream, S1AP_S1AP_PDU_t *pdu);

#endif /* S1AP_ENB_OVERLOAD_H_ */
