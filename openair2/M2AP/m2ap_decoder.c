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

/*! \file m2ap_decoder.c
 * \brief m2ap decoder procedures 
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include <stdio.h>

#include "assertions.h"
#include "intertask_interface.h"
#include "m2ap_common.h"
#include "m2ap_decoder.h"

static int m2ap_decode_initiating_message(M2AP_M2AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);

  switch(pdu->choice.initiatingMessage.procedureCode) {
    case M2AP_ProcedureCode_id_sessionStart:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;
    case M2AP_ProcedureCode_id_sessionStop:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;
    case M2AP_ProcedureCode_id_m2Setup:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;
    case M2AP_ProcedureCode_id_mbmsSchedulingInformation:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;


   
//    case M2AP_ProcedureCode_id_handoverPreparation:
//      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
//      M2AP_INFO("m2ap__decode_initiating_message!\n");
//      break;
//
//    case M2AP_ProcedureCode_id_uEContextRelease:
//      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
//      M2AP_INFO("m2ap__decode_initiating_message!\n");
//      break;
//
//    case M2AP_ProcedureCode_id_handoverCancel:
//      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
//      M2AP_INFO("m2ap__decode_initiating_message!\n");
//      break;
//
    default:
      M2AP_ERROR("Unknown procedure ID (%d) for initiating message\n",
                  (int)pdu->choice.initiatingMessage.procedureCode);
      AssertFatal( 0, "Unknown procedure ID (%d) for initiating message\n",
                   (int)pdu->choice.initiatingMessage.procedureCode);
      return -1;
  }

  return 0;
}

static int m2ap_decode_successful_outcome(M2AP_M2AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);

  switch(pdu->choice.successfulOutcome.procedureCode) {
    case M2AP_ProcedureCode_id_sessionStart:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_successfuloutcome_message!\n");
      break;
    case M2AP_ProcedureCode_id_sessionStop:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_succesfuloutcome_message!\n");
      break;
    case M2AP_ProcedureCode_id_m2Setup:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_succesfuloutcome_message!\n");
      break;
    case M2AP_ProcedureCode_id_mbmsSchedulingInformation:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_succesfuloutcome_message!\n");
      break;


//    case M2AP_ProcedureCode_id_handoverPreparation:
//      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
//      M2AP_INFO("m2ap__decode_successfuloutcome_message!\n");
//      break;
//
    default:
      M2AP_ERROR("Unknown procedure ID (%d) for successfull outcome message\n",
                  (int)pdu->choice.successfulOutcome.procedureCode);
      return -1;
  }

  return 0;
}

static int m2ap_decode_unsuccessful_outcome(M2AP_M2AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);

  switch(pdu->choice.unsuccessfulOutcome.procedureCode) {
    case M2AP_ProcedureCode_id_sessionStart:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;
    case M2AP_ProcedureCode_id_sessionStop:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;
    case M2AP_ProcedureCode_id_m2Setup:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;
    case M2AP_ProcedureCode_id_mbmsSchedulingInformation:
      //asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_M2AP_M2AP_PDU, pdu);
      M2AP_INFO("m2ap__decode_initiating_message!\n");
      break;


    default:
       M2AP_ERROR("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
                  (int)pdu->choice.unsuccessfulOutcome.procedureCode);
      return -1;
  }

  return 0;
}

int m2ap_decode_pdu(M2AP_M2AP_PDU_t *pdu, const uint8_t *const buffer, uint32_t length)
{
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  dec_ret = aper_decode(NULL,
                        &asn_DEF_M2AP_M2AP_PDU,
                        (void **)&pdu,
                        buffer,
                        length,
                        0,
                        0);
  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_M2AP_M2AP_PDU, pdu);
  }

  if (dec_ret.code != RC_OK) {
    M2AP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  switch(pdu->present) {
    case M2AP_M2AP_PDU_PR_initiatingMessage:
      return m2ap_decode_initiating_message(pdu);

    case M2AP_M2AP_PDU_PR_successfulOutcome:
      return m2ap_decode_successful_outcome(pdu);

    case M2AP_M2AP_PDU_PR_unsuccessfulOutcome:
      return m2ap_decode_unsuccessful_outcome(pdu);

    default:
      M2AP_DEBUG("Unknown presence (%d) or not implemented\n", (int)pdu->present);
      break;
  }


  return -1;
}
