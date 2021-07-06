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

#ifndef F1AP_ITTI_MESSAGING_H_
#define F1AP_ITTI_MESSAGING_H_

void f1ap_itti_send_sctp_data_req(bool isCu,instance_t instance, uint8_t *buffer,
                                  uint32_t buffer_length, uint16_t stream);

void f1ap_eNB_itti_send_sctp_close_association(bool isCu,instance_t instance);


#endif /* F1AP_ITTI_MESSAGING_H_ */
