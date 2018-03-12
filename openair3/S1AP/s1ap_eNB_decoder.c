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
#include "s1ap_ies_defs.h"
#include "s1ap_eNB_decoder.h"

static int s1ap_eNB_decode_initiating_message(s1ap_message *message,
    S1ap_InitiatingMessage_t *initiating_p)
{
  int         ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(initiating_p != NULL);

  message_string = calloc(10000, sizeof(char));

  s1ap_string_total_size = 0;

  message->procedureCode = initiating_p->procedureCode;
  message->criticality   = initiating_p->criticality;

  switch(initiating_p->procedureCode) {
  case S1ap_ProcedureCode_id_downlinkNASTransport:
    ret = s1ap_decode_s1ap_downlinknastransporties(
            &message->msg.s1ap_DownlinkNASTransportIEs,
            &initiating_p->value);
    s1ap_xer_print_s1ap_downlinknastransport(s1ap_xer__print2sp,
        message_string,
        message);
    message_id          = S1AP_DOWNLINK_NAS_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_S1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_downlink_nas_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_downlink_nas_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_decode_s1ap_initialcontextsetuprequesties(
            &message->msg.s1ap_InitialContextSetupRequestIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_initialcontextsetuprequest(s1ap_xer__print2sp, message_string, message);
    message_id = S1AP_INITIAL_CONTEXT_SETUP_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_S1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_initial_context_setup_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_initial_context_setup_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case S1ap_ProcedureCode_id_UEContextRelease:
    ret = s1ap_decode_s1ap_uecontextreleasecommandies(
            &message->msg.s1ap_UEContextReleaseCommandIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_uecontextreleasecommand(s1ap_xer__print2sp, message_string, message);
    message_id = S1AP_UE_CONTEXT_RELEASE_COMMAND_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_S1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_ue_context_release_command_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_ue_context_release_command_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case S1ap_ProcedureCode_id_Paging:
    ret = s1ap_decode_s1ap_pagingies(
            &message->msg.s1ap_PagingIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_paging(s1ap_xer__print2sp, message_string, message);
    message_id = S1AP_PAGING_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_S1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_paging_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_paging_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    S1AP_INFO("Paging initiating message\n");
    free(message_string);
    break;


  case S1ap_ProcedureCode_id_E_RABSetup:
    ret = s1ap_decode_s1ap_e_rabsetuprequesties(
						&message->msg.s1ap_E_RABSetupRequestIEs, &initiating_p->value);
    //s1ap_xer_print_s1ap_e_rabsetuprequest(s1ap_xer__print2sp, message_string, message);
    message_id = S1AP_E_RAB_SETUP_REQUEST_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_S1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_e_rab_setup_request_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_e_rab_setup_request_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    S1AP_INFO("E_RABSetup initiating message\n");
    break;

  case S1ap_ProcedureCode_id_E_RABModify:
    ret = s1ap_decode_s1ap_e_rabmodifyrequesties(
            &message->msg.s1ap_E_RABModifyRequestIEs, &initiating_p->value);
    message_id = S1AP_E_RAB_MODIFY_REQUEST_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_S1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_e_rab_modify_request_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_e_rab_modify_request_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    S1AP_INFO("E_RABModify initiating message\n");
    break;

  case S1ap_ProcedureCode_id_E_RABRelease:
    ret = s1ap_decode_s1ap_e_rabreleasecommandies(
            &message->msg.s1ap_E_RABReleaseCommandIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_e_rabreleasecommand(s1ap_xer__print2sp, message_string, message);
    message_id = S1AP_E_RAB_RELEASE_REQUEST_LOG;
    message_string_size = strlen(message_string);
    message_p           = itti_alloc_new_message_sized(TASK_S1AP,
                          message_id,
                          message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_e_rab_release_request_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_e_rab_release_request_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    S1AP_INFO("TODO  E_RABRelease nitiating message\n");    
    break;

  default:
    S1AP_ERROR("Unknown procedure ID (%d) for initiating message\n",
               (int)initiating_p->procedureCode);
    AssertFatal( 0 , "Unknown procedure ID (%d) for initiating message\n",
                 (int)initiating_p->procedureCode);
    return -1;
  }


  return ret;
}

static int s1ap_eNB_decode_successful_outcome(s1ap_message *message,
    S1ap_SuccessfulOutcome_t *successfullOutcome_p)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(successfullOutcome_p != NULL);

  message_string = malloc(sizeof(char) * 10000);
  memset((void*)message_string,0,sizeof(char) * 10000);

  s1ap_string_total_size = 0;

  message->procedureCode = successfullOutcome_p->procedureCode;
  message->criticality   = successfullOutcome_p->criticality;

  switch(successfullOutcome_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_decode_s1ap_s1setupresponseies(
            &message->msg.s1ap_S1SetupResponseIEs, &successfullOutcome_p->value);
    s1ap_xer_print_s1ap_s1setupresponse(s1ap_xer__print2sp, message_string, message);
    message_id = S1AP_S1_SETUP_LOG;
    break;

  default:
    S1AP_ERROR("Unknown procedure ID (%d) for successfull outcome message\n",
               (int)successfullOutcome_p->procedureCode);
    return -1;
  }

  message_string_size = strlen(message_string);

  message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
  message_p->ittiMsg.s1ap_s1_setup_log.size = message_string_size;
  memcpy(&message_p->ittiMsg.s1ap_s1_setup_log.text, message_string, message_string_size);

  itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

  free(message_string);

  return ret;
}

static int s1ap_eNB_decode_unsuccessful_outcome(s1ap_message *message,
    S1ap_UnsuccessfulOutcome_t *unSuccessfullOutcome_p)
{
  int ret = -1;
  DevAssert(unSuccessfullOutcome_p != NULL);

  message->procedureCode = unSuccessfullOutcome_p->procedureCode;
  message->criticality   = unSuccessfullOutcome_p->criticality;

  switch(unSuccessfullOutcome_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    return s1ap_decode_s1ap_s1setupfailureies(
             &message->msg.s1ap_S1SetupFailureIEs, &unSuccessfullOutcome_p->value);

  default:
    S1AP_ERROR("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
               (int)unSuccessfullOutcome_p->procedureCode);
    break;
  }

  return ret;
}

int s1ap_eNB_decode_pdu(s1ap_message *message, const uint8_t * const buffer,
                        const uint32_t length)
{
  S1AP_PDU_t  pdu;
  S1AP_PDU_t *pdu_p = &pdu;
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  memset((void *)pdu_p, 0, sizeof(S1AP_PDU_t));

  dec_ret = aper_decode(NULL,
                        &asn_DEF_S1AP_PDU,
                        (void **)&pdu_p,
                        buffer,
                        length,
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    S1AP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  message->direction = pdu_p->present;

  switch(pdu_p->present) {
  case S1AP_PDU_PR_initiatingMessage:
    return s1ap_eNB_decode_initiating_message(message,
           &pdu_p->choice.initiatingMessage);

  case S1AP_PDU_PR_successfulOutcome:
    return s1ap_eNB_decode_successful_outcome(message,
           &pdu_p->choice.successfulOutcome);

  case S1AP_PDU_PR_unsuccessfulOutcome:
    return s1ap_eNB_decode_unsuccessful_outcome(message,
           &pdu_p->choice.unsuccessfulOutcome);

  default:
    S1AP_DEBUG("Unknown presence (%d) or not implemented\n", (int)pdu_p->present);
    break;
  }

  return -1;
}
