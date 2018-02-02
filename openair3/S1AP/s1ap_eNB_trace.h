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

#ifndef S1AP_ENB_TRACE_H_
#define S1AP_ENB_TRACE_H_

// int s1ap_eNB_generate_trace_failure(sctp_data_t        *sctp_data_p,
//                                     int32_t             stream,
//                                     uint32_t            eNB_ue_s1ap_id,
//                                     uint32_t            mme_ue_s1ap_id,
//                                     E_UTRAN_Trace_ID_t *trace_id,
//                                     Cause_t            *cause_p);

// int s1ap_eNB_handle_trace_start(eNB_mme_desc_t *eNB_desc_p,
//                                 sctp_queue_item_t *packet_p,
//                                 struct s1ap_message_s *message_p);
int s1ap_eNB_handle_trace_start(uint32_t               assoc_id,
                                uint32_t               stream,
                                struct s1ap_message_s *message_p);

// int s1ap_eNB_handle_deactivate_trace(eNB_mme_desc_t *eNB_desc_p,
//                                      sctp_queue_item_t *packet_p,
//                                      struct s1ap_message_s *message_p);
int s1ap_eNB_handle_deactivate_trace(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     struct s1ap_message_s *message_p);

#endif /* S1AP_ENB_TRACE_H_ */
