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

static inline int ngap_gNB_encode_initiating(NGAP_NGAP_PDU_t *pdu, uint8_t **buffer, uint32_t *len)
{
  DevAssert(pdu != NULL);

  const NGAP_ProcedureCode_t tmp[] = {NGAP_ProcedureCode_id_NGSetup,
                                      NGAP_ProcedureCode_id_UplinkNASTransport,
                                      NGAP_ProcedureCode_id_UERadioCapabilityInfoIndication,
                                      NGAP_ProcedureCode_id_InitialUEMessage,
                                      NGAP_ProcedureCode_id_NASNonDeliveryIndication,
                                      NGAP_ProcedureCode_id_UEContextReleaseRequest,
                                      NGAP_ProcedureCode_id_PathSwitchRequest,
                                      NGAP_ProcedureCode_id_PDUSessionResourceModifyIndication};
  int i;
  for (i = 0; i < sizeofArray(tmp); i++)
    if (pdu->choice.initiatingMessage->procedureCode == tmp[i])
      break;
  if (i == sizeofArray(tmp)) {
    NGAP_DEBUG("Unknown procedure ID (%d) for initiating message\n", (int)pdu->choice.initiatingMessage->procedureCode);
    return -1;
  }

  asn_encode_to_new_buffer_result_t res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_NGAP_PDU, pdu);
  AssertFatal(res.result.encoded > 0, "failed to encode NGAP msg\n");
  *buffer = res.buffer;
  *len = res.result.encoded;
  return 0;
}

static inline int ngap_gNB_encode_successfull_outcome(NGAP_NGAP_PDU_t *pdu, uint8_t **buffer, uint32_t *len)
{
  DevAssert(pdu != NULL);
  const NGAP_ProcedureCode_t tmp[] = {NGAP_ProcedureCode_id_InitialContextSetup,
                                      NGAP_ProcedureCode_id_UEContextRelease,
                                      NGAP_ProcedureCode_id_PDUSessionResourceSetup,
                                      NGAP_ProcedureCode_id_PDUSessionResourceModify,
                                      NGAP_ProcedureCode_id_PDUSessionResourceRelease};
  int i;
  for (i = 0; i < sizeofArray(tmp); i++)
    if (pdu->choice.successfulOutcome->procedureCode == tmp[i])
      break;
  if (i == sizeofArray(tmp)) {
    NGAP_WARN("Unknown procedure ID (%ld) for successfull outcome message\n", pdu->choice.successfulOutcome->procedureCode);
    return -1;
  }

  asn_encode_to_new_buffer_result_t res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_NGAP_PDU, pdu);
  AssertFatal(res.result.encoded > 0, "failed to encode NGAP msg\n");
  *buffer = res.buffer;
  *len = res.result.encoded;
  return 0;
}

static inline int ngap_gNB_encode_unsuccessfull_outcome(NGAP_NGAP_PDU_t *pdu, uint8_t **buffer, uint32_t *len)
{
  DevAssert(pdu != NULL);

  if (pdu->choice.unsuccessfulOutcome->procedureCode != NGAP_ProcedureCode_id_InitialContextSetup) {
    NGAP_DEBUG("Unknown procedure ID (%d) for unsuccessfull outcome message\n", (int)pdu->choice.unsuccessfulOutcome->procedureCode);
    return -1;
  }

  asn_encode_to_new_buffer_result_t res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_NGAP_PDU, pdu);
  AssertFatal(res.result.encoded > 0, "failed to encode NGAP msg\n");
  *buffer = res.buffer;
  *len = res.result.encoded;
  return 0;
}

int ngap_gNB_encode_pdu(NGAP_NGAP_PDU_t *pdu, uint8_t **buffer, uint32_t *len)
{
  int ret = -1;
  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  DevAssert(len != NULL);
  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_NGAP_NGAP_PDU, (void *)pdu);
  }
  switch (pdu->present) {
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
      NGAP_DEBUG("Unknown message outcome (%d) or not implemented", (int)pdu->present);
      return -1;
  }
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, pdu);
  return ret;
}
