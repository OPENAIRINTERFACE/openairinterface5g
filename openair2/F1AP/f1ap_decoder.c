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

/*! \file f1ap_decoder.c
 * \brief f1ap pdu decode procedures
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_decoder.h"

int asn1_decoder_xer_print = 0;

static int f1ap_decode_initiating_message(F1AP_F1AP_PDU_t *pdu)
{
  //MessageDef *message_p;
  //MessagesIds message_id;
  //asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.initiatingMessage->procedureCode) {
    
    case F1AP_ProcedureCode_id_F1Setup:
      //res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_F1AP_F1AP_PDU, pdu);
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_F1Setup\n", __func__);
      break;

    case F1AP_ProcedureCode_id_InitialULRRCMessageTransfer:
      //res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_F1AP_F1AP_PDU, pdu);
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_InitialULRRCMessageTransfer\n", __func__);
      break;

    case F1AP_ProcedureCode_id_DLRRCMessageTransfer:
      //res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_F1AP_F1AP_PDU, pdu);
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_DLRRCMessageTransfer\n", __func__);
      break;

    case F1AP_ProcedureCode_id_ULRRCMessageTransfer:
      //res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_F1AP_F1AP_PDU, pdu);
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_ULRRCMessageTransfer\n", __func__);
      break;
    case F1AP_ProcedureCode_id_UEContextRelease:
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_UEContextRelease\n", __func__);
      break;
    case F1AP_ProcedureCode_id_UEContextReleaseRequest:
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_UEContextReleaseRequest\n", __func__);
      break;
    // case F1AP_ProcedureCode_id_InitialContextSetup:
    //   res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_F1AP_F1AP_PDU, pdu);
    //   message_id = F1AP_INITIAL_CONTEXT_SETUP_LOG;
    //   message_p = itti_alloc_new_message_sized(TASK_F1AP, message_id,
    //               res.result.encoded + sizeof (IttiMsgText));
    //   message_p->ittiMsg.f1ap_initial_context_setup_log.size = res.result.encoded;
    //   memcpy(&message_p->ittiMsg.f1ap_initial_context_setup_log.text, res.buffer, res.result.encoded);
    //   itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    //   free(res.buffer);
    //   break;

    default:
      // F1AP_ERROR("Unknown procedure ID (%d) for initiating message\n",
      //            (int)pdu->choice.initiatingMessage->procedureCode);
      LOG_E(F1AP, "Unknown procedure ID (%d) for initiating message\n",
                  (int)pdu->choice.initiatingMessage->procedureCode);
      AssertFatal( 0, "Unknown procedure ID (%d) for initiating message\n",
                   (int)pdu->choice.initiatingMessage->procedureCode);
      return -1;
  }

  return 0;
}

static int f1ap_decode_successful_outcome(F1AP_F1AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);

  switch(pdu->choice.successfulOutcome->procedureCode) {
    case F1AP_ProcedureCode_id_F1Setup:
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_F1Setup\n", __func__);
      break;

    case F1AP_ProcedureCode_id_UEContextRelease:
      LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_UEContextRelease\n", __func__);
      break;

    default:
      LOG_E(F1AP,"Unknown procedure ID (%d) for successfull outcome message\n",
                 (int)pdu->choice.successfulOutcome->procedureCode);
      return -1;
  }

  return 0;
}

static int f1ap_decode_unsuccessful_outcome(F1AP_F1AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);

  switch(pdu->choice.unsuccessfulOutcome->procedureCode) {
    case F1AP_ProcedureCode_id_F1Setup:
    LOG_I(F1AP, "%s(): F1AP_ProcedureCode_id_F1Setup\n", __func__);
    break;

    default:
      // F1AP_ERROR("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
      //            (int)pdu->choice.unsuccessfulOutcome->procedureCode);
      LOG_E(F1AP, "Unknown procedure ID (%d) for unsuccessfull outcome message\n",
                 (int)pdu->choice.unsuccessfulOutcome->procedureCode);
      return -1;
  }

  return 0;
}

int f1ap_decode_pdu(F1AP_F1AP_PDU_t *pdu, const uint8_t *const buffer, uint32_t length)
{
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  dec_ret = aper_decode(NULL,
                        &asn_DEF_F1AP_F1AP_PDU,
                        (void **)&pdu,
                        buffer,
                        length,
                        0,
                        0);

  if (asn1_decoder_xer_print) {
    LOG_E(F1AP, "----------------- ASN1 DECODER PRINT START----------------- \n");
    xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, pdu);
    LOG_E(F1AP, "----------------- ASN1 DECODER PRINT END ----------------- \n");
  }
  //LOG_I(F1AP, "f1ap_decode_pdu.dec_ret.code = %d\n", dec_ret.code);

  if (dec_ret.code != RC_OK) {
    LOG_E(F1AP, "Failed to decode pdu\n");
    return -1;
  }

  switch(pdu->present) {
    case F1AP_F1AP_PDU_PR_initiatingMessage:
      return f1ap_decode_initiating_message(pdu);

    case F1AP_F1AP_PDU_PR_successfulOutcome:
      return f1ap_decode_successful_outcome(pdu);

    case F1AP_F1AP_PDU_PR_unsuccessfulOutcome:
      return f1ap_decode_unsuccessful_outcome(pdu);

    default:
      LOG_E(F1AP, "Unknown presence (%d) or not implemented\n", (int)pdu->present);
      break;
  }


  return -1;
}
