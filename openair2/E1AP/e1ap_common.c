/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <time.h>
#include <stdlib.h>
#include "e1ap_common.h"
#include "e1ap_default_values.h"
#include "e1ap_asnc.h"
#include "common/openairinterface5g_limits.h"
#include "common/utils/ocp_itti/intertask_interface.h"

static e1ap_upcp_inst_t *e1ap_inst[NUMBER_OF_gNB_MAX] = {0};

e1ap_upcp_inst_t *getCxtE1(instance_t instance)
{
  AssertFatal(instance < sizeofArray(e1ap_inst), "instance exceeds limit\n");
  return e1ap_inst[instance];
}

void createE1inst(E1_t type, instance_t instance, uint64_t gnb_id, e1ap_net_config_t *nc, e1ap_setup_req_t *req)
{
  AssertFatal(e1ap_inst[instance] == NULL, "Double call to E1 instance %d\n", (int)instance);
  e1ap_inst[instance] = calloc(1, sizeof(e1ap_upcp_inst_t));
  e1ap_inst[instance]->type = type;
  e1ap_inst[instance]->instance = instance;
  e1ap_inst[instance]->gnb_id = gnb_id;
  e1ap_inst[instance]->cuup.assoc_id = -1;
  if (nc)
    memcpy(&e1ap_inst[instance]->net_config, nc, sizeof(*nc));
  if (req) {
    AssertFatal(type == UPtype, "E1 setup request only to be stored for CU-UP\n");
    memcpy(&e1ap_inst[instance]->cuup.setupReq, req, sizeof(*req));
  }
  e1ap_inst[instance]->gtpInstN3 = -1;
  e1ap_inst[instance]->gtpInstF1U = -1;
}

static E1AP_TransactionID_t transacID[E1AP_MAX_NUM_TRANSAC_IDS] = {0};

void e1ap_common_init() {
  srand(time(NULL));
}

static bool check_transac_id(E1AP_TransactionID_t id, int *freeIdx)
{
  bool isFreeIdxSet = false;
  for (int i=0; i < E1AP_MAX_NUM_TRANSAC_IDS; i++) {
    if (id == transacID[i])
      return false;
    else if (!isFreeIdxSet && (transacID[i] == 0)) {
      *freeIdx = i;
      isFreeIdxSet = true;
    }
  }

  return true;
}

long E1AP_get_next_transaction_identifier() {
  E1AP_TransactionID_t genTransacId;
  bool isTransacIdValid = false;
  int freeIdx = 0;

  while (!isTransacIdValid) {
    genTransacId = rand() & 255;
    isTransacIdValid = check_transac_id(genTransacId, &freeIdx);
  }

  AssertFatal(freeIdx < E1AP_MAX_NUM_TRANSAC_IDS, "Free Index exceeds array length\n");
  transacID[freeIdx] = genTransacId;
  return genTransacId;
}

void E1AP_free_transaction_identifier(long id) {

  for (int i=0; i < E1AP_MAX_NUM_TRANSAC_IDS; i++) {
    if (id == transacID[i]) {
      transacID[i] = 0;
      return;
    }
  }
  LOG_E(E1AP, "Couldn't find transaction ID %ld in list\n", id);
}

int e1ap_decode_pdu(E1AP_E1AP_PDU_t *pdu, const uint8_t *const buffer, uint32_t length)
{
  DevAssert(buffer != NULL);
  DevAssert(pdu != NULL);

  asn_dec_rval_t dec_ret = aper_decode(NULL, &asn_DEF_E1AP_E1AP_PDU, (void **)&pdu, buffer, length, 0, 0);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    LOG_E(E1AP, "----------------- ASN1 DECODER PRINT START----------------- \n");
    xer_fprint(stdout, &asn_DEF_E1AP_E1AP_PDU, pdu);
    LOG_E(E1AP, "----------------- ASN1 DECODER PRINT END ----------------- \n");
  }

  if (dec_ret.code != RC_OK) {
    LOG_E(E1AP, "Failed to decode E1AP PDU\n");
    return -1;
  }

  return 0;
}

int e1ap_encode_send(E1_t type, sctp_assoc_t assoc_id, E1AP_E1AP_PDU_t *pdu, uint16_t stream, const char *func)
{
  DevAssert(pdu != NULL);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    LOG_E(E1AP, "----------------- ASN1 ENCODER PRINT START ----------------- \n");
    xer_fprint(stdout, &asn_DEF_E1AP_E1AP_PDU, pdu);
    LOG_E(E1AP, "----------------- ASN1 ENCODER PRINT END----------------- \n");
  }

  char errbuf[2048]; /* Buffer for error message */
  size_t errlen = sizeof(errbuf); /* Size of the buffer */
  if (asn_check_constraints(&asn_DEF_E1AP_E1AP_PDU, pdu, errbuf, &errlen)) {
    xer_fprint(stdout, &asn_DEF_E1AP_E1AP_PDU, pdu);
    LOG_E(E1AP, "%s: Constraint validation failed: %s\n", func, errbuf);
    ASN_STRUCT_FREE(asn_DEF_E1AP_E1AP_PDU, pdu);
    return -1;
  }

  void *buffer = NULL;
  ssize_t encoded = aper_encode_to_new_buffer(&asn_DEF_E1AP_E1AP_PDU, 0, pdu, &buffer);
  ASN_STRUCT_FREE(asn_DEF_E1AP_E1AP_PDU, pdu);

  if (encoded < 0) {
    LOG_E(E1AP, "%s: Failed to encode E1AP message\n", func);
    return -1;
  }

  MessageDef *message = itti_alloc_new_message((type == CPtype) ? TASK_CUCP_E1 : TASK_CUUP_E1, 0, SCTP_DATA_REQ);
  sctp_data_req_t *s = &message->ittiMsg.sctp_data_req;
  s->assoc_id = assoc_id;
  s->buffer = buffer;
  s->buffer_length = encoded;
  s->stream = stream;
  LOG_D(E1AP, "%s: Sending ITTI message to SCTP Task\n", func);
  itti_send_msg_to_task(TASK_SCTP, 0 /*unused by callee*/, message);

  return encoded;
}
