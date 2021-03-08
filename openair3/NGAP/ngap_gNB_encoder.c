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

/*! \file ngap_gNB_encoder.c
 * \brief ngap pdu encode procedures for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "ngap_common.h"
#include "ngap_gNB_encoder.h"

static inline int ngap_gNB_encode_initiating(NGAP_NGAP_PDU_t *pdu,
    uint8_t **buffer,
    uint32_t *len);

static inline int ngap_gNB_encode_successfull_outcome(NGAP_NGAP_PDU_t *pdu,
    uint8_t **buffer, uint32_t *len);

static inline int ngap_gNB_encode_unsuccessfull_outcome(NGAP_NGAP_PDU_t *pdu,
    uint8_t **buffer, uint32_t *len);

int ngap_gNB_encode_pdu(NGAP_NGAP_PDU_t *pdu, uint8_t **buffer, uint32_t *len) {
  int ret = -1;
  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  DevAssert(len != NULL);

  switch(pdu->present) {
    case NGAP_NGAP_PDU_PR_initiatingMessage:
      ret = ngap_gNB_encode_initiating(pdu, buffer, len);
      break;

    case NGAP_NGAP_PDU_PR_successfulOutcome:
      ret = ngap_gNB_encode_successfull_outcome(pdu, buffer, len);
      break;

    case NGAP_NGAP_PDU_PR_unsuccessfulOutcome:
      ret = ngap_gNB_encode_unsuccessfull_outcome(pdu, buffer, len);
      break;

    default:
      NGAP_DEBUG("Unknown message outcome (%d) or not implemented",
                 (int)pdu->present);
      return -1;
  }

  //ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, pdu);
  return ret;
}

static inline
int ngap_gNB_encode_initiating(NGAP_NGAP_PDU_t *pdu,
                               uint8_t **buffer, uint32_t *len) {
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.initiatingMessage->procedureCode) {
    case NGAP_ProcedureCode_id_NGSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_UplinkNASTransport:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_UERadioCapabilityInfoIndication:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_InitialUEMessage:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_NASNonDeliveryIndication:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_UEContextReleaseRequest:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_PathSwitchRequest:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_PDUSessionResourceModifyIndication:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    default:
      NGAP_DEBUG("Unknown procedure ID (%d) for initiating message\n",
                 (int)pdu->choice.initiatingMessage->procedureCode);
      return -1;
  }

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_NGAP_NGAP_PDU, (void *)pdu);
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_NGAP_PDU, pdu);
  *buffer = res.buffer;
  *len = res.result.encoded;
  return 0;
}

static inline
int ngap_gNB_encode_successfull_outcome(NGAP_NGAP_PDU_t *pdu,
                                        uint8_t **buffer, uint32_t *len) {
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.successfulOutcome->procedureCode) {
    case NGAP_ProcedureCode_id_InitialContextSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_UEContextRelease:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_PDUSessionResourceSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      NGAP_INFO("PDUSESSIONSetup successful message\n");
      break;

    case NGAP_ProcedureCode_id_PDUSessionResourceModify:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      NGAP_INFO("PDUSESSIONModify successful message\n");
      break;

    case NGAP_ProcedureCode_id_PDUSessionResourceRelease:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      NGAP_INFO("PDUSESSION Release successful message\n");
      break;

    default:
      NGAP_WARN("Unknown procedure ID (%d) for successfull outcome message\n",
                (int)pdu->choice.successfulOutcome->procedureCode);
      return -1;
  }

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_NGAP_NGAP_PDU, (void *)pdu);
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_NGAP_PDU, pdu);
  *buffer = res.buffer;
  *len = res.result.encoded;
  return 0;
}

static inline
int ngap_gNB_encode_unsuccessfull_outcome(NGAP_NGAP_PDU_t *pdu,
    uint8_t **buffer, uint32_t *len) {
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.unsuccessfulOutcome->procedureCode) {
    case NGAP_ProcedureCode_id_InitialContextSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    default:
      NGAP_DEBUG("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
                 (int)pdu->choice.unsuccessfulOutcome->procedureCode);
      return -1;
  }

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_NGAP_NGAP_PDU, (void *)pdu);
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_NGAP_PDU, pdu);
  *buffer = res.buffer;
  *len = res.result.encoded;
  return 0;
}
