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
#include "openair2/COMMON/sctp_messages_types.h"
#include "common/ngran_types.h"

typedef struct e1ap_upcp_inst_s {
  instance_t instance;
  uint32_t gnb_id; // associated gNB's ID, unused in E1 but necessary for e.g. E2 agent
  E1_t type;
  enum sctp_state_e sockState;
  struct {
    sctp_assoc_t assoc_id;
    e1ap_setup_req_t setupReq;
  } cuup;
  instance_t gtpInstN3;
  instance_t gtpInstF1U;
  e1ap_net_config_t net_config;
} e1ap_upcp_inst_t;

extern int asn1_xer_print;

// forward declaration so we don't require E1AP ASN.1 when including this
// header
struct E1AP_E1AP_PDU;
int e1ap_decode_pdu(struct E1AP_E1AP_PDU *pdu, const uint8_t *const buffer, uint32_t length);

e1ap_upcp_inst_t *getCxtE1(instance_t instance);

long E1AP_get_next_transaction_identifier();
void E1AP_free_transaction_identifier(long id);

void createE1inst(E1_t type, instance_t instance, uint64_t gnb_id, e1ap_net_config_t *nc, e1ap_setup_req_t *req);

int e1ap_encode_send(E1_t type, sctp_assoc_t assoc_id, struct E1AP_E1AP_PDU *pdu, uint16_t stream, const char *func);

void e1ap_common_init();
void cuup_init_n3(instance_t instance);

#endif /* E1AP_COMMON_H_ */
