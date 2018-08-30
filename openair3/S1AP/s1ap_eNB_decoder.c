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

/*! \file s1ap_eNB_decoder.c
 * \brief s1ap pdu decode procedures for eNB
 * \author Sebastien ROUX and Navid Nikaein
 * \email navid.nikaein@eurecom.fr
 * \date 2013 - 2015
 * \version 0.1
 */

#include <stdio.h>

#include "assertions.h"

#include "intertask_interface.h"

#include "s1ap_common.h"
#include "s1ap_eNB_decoder.h"

static int s1ap_eNB_decode_initiating_message(S1AP_S1AP_PDU_t *pdu)
{
  MessageDef *message_p;
  MessagesIds message_id;
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.initiatingMessage.procedureCode) {
    case S1AP_ProcedureCode_id_downlinkNASTransport:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_DOWNLINK_NAS_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_downlink_nas_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_downlink_nas_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      break;

    case S1AP_ProcedureCode_id_InitialContextSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_INITIAL_CONTEXT_SETUP_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_initial_context_setup_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_initial_context_setup_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      break;

    case S1AP_ProcedureCode_id_UEContextRelease:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_UE_CONTEXT_RELEASE_COMMAND_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_ue_context_release_command_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_ue_context_release_command_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      break;

    case S1AP_ProcedureCode_id_Paging:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_PAGING_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_paging_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_paging_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      S1AP_INFO("Paging initiating message\n");
      free(res.buffer);
      break;

    case S1AP_ProcedureCode_id_E_RABSetup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_E_RAB_SETUP_REQUEST_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_e_rab_setup_request_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_e_rab_setup_request_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      S1AP_INFO("E_RABSetup initiating message\n");
      break;

    case S1AP_ProcedureCode_id_E_RABModify:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_E_RAB_MODIFY_REQUEST_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_e_rab_modify_request_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_e_rab_modify_request_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      S1AP_INFO("E_RABModify initiating message\n");
      break;

    case S1AP_ProcedureCode_id_E_RABRelease:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_E_RAB_RELEASE_REQUEST_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_e_rab_release_request_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_e_rab_release_request_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      S1AP_INFO("TODO E_RABRelease initiating message\n");
      break;

    case S1AP_ProcedureCode_id_ErrorIndication:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_ERROR_INDICATION_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id,
                  res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_error_indication_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_error_indication_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      S1AP_INFO("TODO ErrorIndication initiating message\n");
      break;

    default:
      S1AP_ERROR("Unknown procedure ID (%d) for initiating message\n",
                 (int)pdu->choice.initiatingMessage.procedureCode);
      AssertFatal( 0, "Unknown procedure ID (%d) for initiating message\n",
                   (int)pdu->choice.initiatingMessage.procedureCode);
      return -1;
  }

  return 0;
}

static int s1ap_eNB_decode_successful_outcome(S1AP_S1AP_PDU_t *pdu)
{
  MessageDef *message_p;
  MessagesIds message_id;
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.successfulOutcome.procedureCode) {
    case S1AP_ProcedureCode_id_S1Setup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_S1_SETUP_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_s1_setup_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_s1_setup_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      break;

    default:
      S1AP_ERROR("Unknown procedure ID (%d) for successfull outcome message\n",
                 (int)pdu->choice.successfulOutcome.procedureCode);
      return -1;
  }

  return 0;
}

static int s1ap_eNB_decode_unsuccessful_outcome(S1AP_S1AP_PDU_t *pdu)
{
  MessageDef *message_p;
  MessagesIds message_id;
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.unsuccessfulOutcome.procedureCode) {
    case S1AP_ProcedureCode_id_S1Setup:
      res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_S1AP_S1AP_PDU, pdu);
      message_id = S1AP_S1_SETUP_LOG;
      message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, res.result.encoded + sizeof (IttiMsgText));
      message_p->ittiMsg.s1ap_s1_setup_log.size = res.result.encoded;
      memcpy(&message_p->ittiMsg.s1ap_s1_setup_log.text, res.buffer, res.result.encoded);
      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
      free(res.buffer);
      break;

    default:
      S1AP_ERROR("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
                 (int)pdu->choice.unsuccessfulOutcome.procedureCode);
      return -1;
  }

  return 0;
}

int s1ap_eNB_decode_pdu(S1AP_S1AP_PDU_t *pdu, const uint8_t *const buffer,
                        const uint32_t length)
{
  asn_dec_rval_t dec_ret;

  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);

  dec_ret = aper_decode(NULL,
                        &asn_DEF_S1AP_S1AP_PDU,
                        (void **)&pdu,
                        buffer,
                        length,
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    S1AP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  switch(pdu->present) {
    case S1AP_S1AP_PDU_PR_initiatingMessage:
      return s1ap_eNB_decode_initiating_message(pdu);

    case S1AP_S1AP_PDU_PR_successfulOutcome:
      return s1ap_eNB_decode_successful_outcome(pdu);

    case S1AP_S1AP_PDU_PR_unsuccessfulOutcome:
      return s1ap_eNB_decode_unsuccessful_outcome(pdu);

    default:
      S1AP_DEBUG("Unknown presence (%d) or not implemented\n", (int)pdu->present);
      break;
  }

  return -1;
}
