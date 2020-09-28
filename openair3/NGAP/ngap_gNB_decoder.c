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

/*! \file ngap_gNB_decoder.c
 * \brief ngap pdu decode procedures for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */

 
#include <stdio.h>

#include "assertions.h"

#include "intertask_interface.h"

#include "ngap_common.h"
#include "ngap_gNB_decoder.h"

static int ngap_gNB_decode_initiating_message(NGAP_NGAP_PDU_t *pdu) {
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.initiatingMessage->procedureCode) {
    case NGAP_ProcedureCode_id_DownlinkNASTransport:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_InitialContextSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_UEContextRelease:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_Paging:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      NGAP_INFO("Paging initiating message\n");
      free(res.buffer);
      break;

    case NGAP_ProcedureCode_id_PDUSessionResourceSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      NGAP_INFO("PDUSESSIONSetup initiating message\n");
      break;

    case NGAP_ProcedureCode_id_PDUSessionResourceModify:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      NGAP_INFO("PDUSESSIONModify initiating message\n");
      break;

    case NGAP_ProcedureCode_id_PDUSessionResourceRelease:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      NGAP_INFO("TODO PDUSESSIONRelease initiating message\n");
      break;

    case NGAP_ProcedureCode_id_ErrorIndication:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      NGAP_INFO("TODO ErrorIndication initiating message\n");
      break;

    default:
      NGAP_ERROR("Unknown procedure ID (%d) for initiating message\n",
                 (int)pdu->choice.initiatingMessage->procedureCode);
      AssertFatal( 0, "Unknown procedure ID (%d) for initiating message\n",
                   (int)pdu->choice.initiatingMessage->procedureCode);
      return -1;
  }

  return 0;
}

static int ngap_gNB_decode_successful_outcome(NGAP_NGAP_PDU_t *pdu) {
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.successfulOutcome->procedureCode) {
    case NGAP_ProcedureCode_id_NGSetup:
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
      NGAP_ERROR("Unknown procedure ID (%d) for successfull outcome message\n",
                 (int)pdu->choice.successfulOutcome->procedureCode);
      return -1;
  }

  return 0;
}

static int ngap_gNB_decode_unsuccessful_outcome(NGAP_NGAP_PDU_t *pdu) {
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.unsuccessfulOutcome->procedureCode) {
    case NGAP_ProcedureCode_id_NGSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;
   case NGAP_ProcedureCode_id_PathSwitchRequest:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_NGAP_NGAP_PDU, pdu);
      free(res.buffer);
      break;

    default:
      NGAP_ERROR("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
                 (int)pdu->choice.unsuccessfulOutcome->procedureCode);
      return -1;
  }

  return 0;
}

int ngap_gNB_decode_pdu(NGAP_NGAP_PDU_t *pdu, const uint8_t *const buffer,
                        const uint32_t length) {
  asn_dec_rval_t dec_ret;
  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  dec_ret = aper_decode(NULL,
                        &asn_DEF_NGAP_NGAP_PDU,
                        (void **)&pdu,
                        buffer,
                        length,
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    NGAP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  switch(pdu->present) {
    case NGAP_NGAP_PDU_PR_initiatingMessage:
      return ngap_gNB_decode_initiating_message(pdu);

    case NGAP_NGAP_PDU_PR_successfulOutcome:
      return ngap_gNB_decode_successful_outcome(pdu);

    case NGAP_NGAP_PDU_PR_unsuccessfulOutcome:
      return ngap_gNB_decode_unsuccessful_outcome(pdu);

    default:
      NGAP_DEBUG("Unknown presence (%d) or not implemented\n", (int)pdu->present);
      break;
  }

  return -1;
}
