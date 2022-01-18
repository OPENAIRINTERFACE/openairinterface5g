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

/*! \file f1ap_cu_rrc_message_transfer.h
 * \brief f1ap rrc message transfer for CU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#ifndef F1AP_CU_RRC_MESSAGE_TRANSFER_H_
#define F1AP_CU_RRC_MESSAGE_TRANSFER_H_

int CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu);

int CU_send_DL_RRC_MESSAGE_TRANSFER(instance_t                instance,
                                    f1ap_dl_rrc_message_t    *f1ap_dl_rrc);

int CU_handle_UL_RRC_MESSAGE_TRANSFER(instance_t       instance,
                                      uint32_t         assoc_id,
                                      uint32_t         stream,
                                      F1AP_F1AP_PDU_t *pdu);


#endif /* F1AP_CU_RRC_MESSAGE_TRANSFER_H_ */
