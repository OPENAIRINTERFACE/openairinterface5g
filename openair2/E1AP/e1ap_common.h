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
 * Author and copyright: Laurent Thomas, open-cells.com
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

#ifndef E1AP_COMMON_H_
#define E1AP_COMMON_H_
#include "openair2/COMMON/e1ap_messages_types.h"
#include "e1ap_asnc.h"
#include "openair2/COMMON/sctp_messages_types.h"
#include "common/ngran_types.h"

typedef struct e1ap_upcp_inst_s {
  bool incoming_sock;
  instance_t instance;
  bool same_process;
  E1_t type;
  enum sctp_state_e sockState;
  uint32_t assoc_id;
  instance_t gtpInstN3;
  instance_t gtpInstF1U;
  e1ap_setup_req_t setupReq;
  e1ap_bearer_setup_req_t bearerSetupReq;
  e1ap_bearer_setup_resp_t bearerSetupResp;
} e1ap_upcp_inst_t;

extern int asn1_xer_print;

int e1ap_decode_pdu(E1AP_E1AP_PDU_t *pdu, const uint8_t *const buffer, uint32_t length);

e1ap_upcp_inst_t *getCxtE1(instance_t instance);

E1AP_TransactionID_t E1AP_get_next_transaction_identifier();

void createE1inst(E1_t type, instance_t instance, e1ap_setup_req_t *req);

bool check_transac_id(E1AP_TransactionID_t id, int *freeIdx);

int e1ap_assoc_id(E1_t type, instance_t instance);

int e1ap_encode_send(E1_t type, e1ap_setup_req_t *setupReq, E1AP_E1AP_PDU_t *pdu, uint16_t stream, const char *func);

void e1ap_common_init();

void E1AP_free_transaction_identifier(E1AP_TransactionID_t id);
#endif /* E1AP_COMMON_H_ */
