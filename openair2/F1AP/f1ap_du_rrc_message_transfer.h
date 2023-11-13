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

/*! \file f1ap_du_rrc_message_transfer.h
 * \brief f1ap rrc message transfer for DU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */


#ifndef F1AP_DU_RRC_MESSAGE_TRANSFER_H_
#define F1AP_DU_RRC_MESSAGE_TRANSFER_H_

#include "f1ap_common.h"

int DU_handle_DL_RRC_MESSAGE_TRANSFER(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu);

int DU_send_UL_NR_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, const f1ap_ul_rrc_message_t *msg);

int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, const f1ap_initial_ul_rrc_message_t *msg);

#endif /* F1AP_DU_RRC_MESSAGE_TRANSFER_H_ */
