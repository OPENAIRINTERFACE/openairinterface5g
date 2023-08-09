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

#ifndef _OPENAIR2_LAYER2_RLC_V2_ASN1_UTILS_H_
#define _OPENAIR2_LAYER2_RLC_V2_ASN1_UTILS_H_

int decode_t_reordering(int v);
int decode_t_status_prohibit(int v);
int decode_t_poll_retransmit(int v);
int decode_poll_pdu(int v);
int decode_poll_byte(int v);
int decode_max_retx_threshold(int v);
int decode_sn_field_length(int v);

#endif /* _OPENAIR2_LAYER2_RLC_V2_ASN1_UTILS_H_ */
