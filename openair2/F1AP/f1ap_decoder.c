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

#include <stdio.h>

#include "assertions.h"

#include "intertask_interface.h"

#include "f1ap_common.h"
#include "f1ap_ies_defs.h"
#include "f1ap_decoder.h"

static int f1ap_decode_initiating_message(f1ap_message *message,
    F1ap_InitiatingMessage_t *initiating_p)
{
  int         ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(initiating_p != NULL);

  message_string = calloc(10000, sizeof(char));

  f1ap_string_total_size = 0;

  message->procedureCode = initiating_p->procedureCode;
  message->criticality   = initiating_p->criticality;

  switch(initiating_p->procedureCode) {

  case F1ap_ProcedureCode_id_InitialContextSetup:
    ret = f1ap_decode_f1ap_initialcontextsetuprequesties(
            &message->msg.f1ap_InitialContextSetupRequestIEs, &initiating_p->value);
    f1ap_xer_print_f1ap_initialcontextsetuprequest(f1ap_xer__print2sp, message_string, message);
    message_id = F1AP_INITIAL_CONTEXT_SETUP_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_F1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.f1ap_initial_context_setup_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.f1ap_initial_context_setup_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case F1ap_ProcedureCode_id_UEContextRelease:
    ret = f1ap_decode_f1ap_uecontextreleasecommandies(
            &message->msg.f1ap_UEContextReleaseCommandIEs, &initiating_p->value);
    f1ap_xer_print_f1ap_uecontextreleasecommand(f1ap_xer__print2sp, message_string, message);
    message_id = F1AP_UE_CONTEXT_RELEASE_COMMAND_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_F1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.f1ap_ue_context_release_command_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.f1ap_ue_context_release_command_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case F1ap_ProcedureCode_id_ErrorIndication:
    ret = f1ap_decode_f1ap_errorindicationies(
            &message->msg.f1ap_ErrorIndicationIEs, &initiating_p->value);
    f1ap_xer_print_f1ap_errorindication(f1ap_xer__print2sp, message_string, message);
    message_id = F1AP_E_RAB_ERROR_INDICATION_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_F1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.f1ap_e_rab_release_request_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.f1ap_error_indication_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    F1AP_INFO("ErrorIndication initiating message\n");
    break;

  default:
    F1AP_ERROR("Unknown procedure ID (%d) for initiating message\n",
               (int)initiating_p->procedureCode);
    AssertFatal( 0 , "Unknown procedure ID (%d) for initiating message\n",
                 (int)initiating_p->procedureCode);
    return -1;
  }


  return ret;
}

static int f1ap_decode_successful_outcome(f1ap_message *message,
    F1ap_SuccessfulOutcome_t *successfullOutcome_p)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(successfullOutcome_p != NULL);

  message_string = malloc(sizeof(char) * 10000);
  memset((void*)message_string,0,sizeof(char) * 10000);

  f1ap_string_total_size = 0;

  message->procedureCode = successfullOutcome_p->procedureCode;
  message->criticality   = successfullOutcome_p->criticality;

  switch(successfullOutcome_p->procedureCode) {
  case F1ap_ProcedureCode_id_F1Setup:
    ret = f1ap_decode_f1ap_f1setupresponseies(
            &message->msg.f1ap_F1SetupResponseIEs, &successfullOutcome_p->value);
    f1ap_xer_print_f1ap_f1setupresponse(f1ap_xer__print2sp, message_string, message);
    message_id = F1AP_F1_SETUP_LOG;
    break;

  default:
    F1AP_ERROR("Unknown procedure ID (%d) for successfull outcome message\n",
               (int)successfullOutcome_p->procedureCode);
    return -1;
  }

  message_string_size = strlen(message_string);

  message_p = itti_alloc_new_message_sized(TASK_F1AP, message_id, message_string_size + sizeof (IttiMsgText));
  message_p->ittiMsg.f1ap_f1_setup_log.size = message_string_size;
  memcpy(&message_p->ittiMsg.f1ap_f1_setup_log.text, message_string, message_string_size);

  itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

  free(message_string);

  return ret;
}

static int f1ap_decode_unsuccessful_outcome(f1ap_message *message,
    F1ap_UnsuccessfulOutcome_t *unSuccessfullOutcome_p)
{
  int ret = -1;
  DevAssert(unSuccessfullOutcome_p != NULL);

  message->procedureCode = unSuccessfullOutcome_p->procedureCode;
  message->criticality   = unSuccessfullOutcome_p->criticality;

  switch(unSuccessfullOutcome_p->procedureCode) {
  case F1ap_ProcedureCode_id_F1Setup:
    return f1ap_decode_f1ap_f1setupfailureies(
             &message->msg.f1ap_F1SetupFailureIEs, &unSuccessfullOutcome_p->value);

  default:
    F1AP_ERROR("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
               (int)unSuccessfullOutcome_p->procedureCode);
    break;
  }

  return ret;
}

int f1ap_decode_pdu(f1ap_message *message, const uint8_t * const buffer,
                        const uint32_t length)
{
  F1AP_PDU_t  pdu;
  F1AP_PDU_t *pdu_p = &pdu;
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  memset((void *)pdu_p, 0, sizeof(F1AP_PDU_t));

  dec_ret = aper_decode(NULL,
                        &asn_DEF_F1AP_PDU,
                        (void **)&pdu_p,
                        buffer,
                        length,
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    F1AP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  message->direction = pdu_p->present;

  switch(pdu_p->present) {
  case F1AP_F1AP_PDU_PR_initiatingMessage:
    return f1ap_decode_initiating_message(message,
           &pdu_p->choice.initiatingMessage);

  case F1AP_F1AP_PDU_PR_successfulOutcome:
    return f1ap_decode_successful_outcome(message,
           &pdu_p->choice.successfulOutcome);

  case F1AP_F1AP_PDU_PR_unsuccessfulOutcome:
    return f1ap_decode_unsuccessful_outcome(message,
           &pdu_p->choice.unsuccessfulOutcome);

  default:
    F1AP_DEBUG("Unknown presence (%d) or not implemented\n", (int)pdu_p->present);
    break;
  }

  return -1;
}
