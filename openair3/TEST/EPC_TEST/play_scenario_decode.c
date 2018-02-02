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

/*
                                play_scenario_decode.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */

#include "intertask_interface.h"
#include "platform_types.h"
#include "s1ap_ies_defs.h"
#include "s1ap_eNB_decoder.h"
#include "assertions.h"
#include "play_scenario.h"

//------------------------------------------------------------------------------
int et_s1ap_decode_initiating_message(s1ap_message *message, S1ap_InitiatingMessage_t *initiating_p)
{
  int         ret = -1;

  DevAssert(initiating_p != NULL);

  message->procedureCode = initiating_p->procedureCode;
  message->criticality   = initiating_p->criticality;

  switch(initiating_p->procedureCode) {
  case S1ap_ProcedureCode_id_downlinkNASTransport:
    ret = s1ap_decode_s1ap_downlinknastransporties(&message->msg.s1ap_DownlinkNASTransportIEs,&initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_decode_s1ap_initialcontextsetuprequesties(&message->msg.s1ap_InitialContextSetupRequestIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_UEContextRelease:
    ret = s1ap_decode_s1ap_uecontextreleasecommandies(&message->msg.s1ap_UEContextReleaseCommandIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_Paging:
    ret = s1ap_decode_s1ap_pagingies(&message->msg.s1ap_PagingIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_uplinkNASTransport:
    ret = s1ap_decode_s1ap_uplinknastransporties (&message->msg.s1ap_UplinkNASTransportIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_decode_s1ap_s1setuprequesties (&message->msg.s1ap_S1SetupRequestIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_initialUEMessage:
    ret = s1ap_decode_s1ap_initialuemessageies (&message->msg.s1ap_InitialUEMessageIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_UEContextReleaseRequest:
    ret = s1ap_decode_s1ap_uecontextreleaserequesties (&message->msg.s1ap_UEContextReleaseRequestIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_UECapabilityInfoIndication:
    ret = s1ap_decode_s1ap_uecapabilityinfoindicationies (&message->msg.s1ap_UECapabilityInfoIndicationIEs, &initiating_p->value);
    break;

  case S1ap_ProcedureCode_id_NASNonDeliveryIndication:
    ret = s1ap_decode_s1ap_nasnondeliveryindication_ies (&message->msg.s1ap_NASNonDeliveryIndication_IEs, &initiating_p->value);
    break;

  default:
    AssertFatal( 0 , "Unknown procedure ID (%d) for initiating message\n",
                 (int)initiating_p->procedureCode);
    return -1;
  }
  return ret;
}

//------------------------------------------------------------------------------
int et_s1ap_decode_successful_outcome(s1ap_message *message, S1ap_SuccessfulOutcome_t *successfullOutcome_p)
{
  int         ret = -1;

  DevAssert(successfullOutcome_p != NULL);

  message->procedureCode = successfullOutcome_p->procedureCode;
  message->criticality   = successfullOutcome_p->criticality;

  switch(successfullOutcome_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_decode_s1ap_s1setupresponseies(
            &message->msg.s1ap_S1SetupResponseIEs, &successfullOutcome_p->value);
    break;

  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_decode_s1ap_initialcontextsetupresponseies (&message->msg.s1ap_InitialContextSetupResponseIEs, &successfullOutcome_p->value);
    break;

  case S1ap_ProcedureCode_id_UEContextRelease:
      ret = s1ap_decode_s1ap_uecontextreleasecompleteies (&message->msg.s1ap_UEContextReleaseCompleteIEs, &successfullOutcome_p->value);
    break;

  default:
    AssertFatal(0, "Unknown procedure ID (%d) for successfull outcome message\n",
               (int)successfullOutcome_p->procedureCode);
    return -1;
  }
  return ret;
}

//------------------------------------------------------------------------------
int et_s1ap_decode_unsuccessful_outcome(s1ap_message *message, S1ap_UnsuccessfulOutcome_t *unSuccessfullOutcome_p)
{
  int ret = -1;

  DevAssert(unSuccessfullOutcome_p != NULL);

  message->procedureCode = unSuccessfullOutcome_p->procedureCode;
  message->criticality   = unSuccessfullOutcome_p->criticality;

  switch(unSuccessfullOutcome_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_decode_s1ap_s1setupfailureies(&message->msg.s1ap_S1SetupFailureIEs, &unSuccessfullOutcome_p->value);
    break;

  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_decode_s1ap_initialcontextsetupfailureies (&message->msg.s1ap_InitialContextSetupFailureIEs, &unSuccessfullOutcome_p->value);
    break;

  default:
    AssertFatal(0,"Unknown procedure ID (%d) for unsuccessfull outcome message\n",
               (int)unSuccessfullOutcome_p->procedureCode);
    break;
  }
  return ret;
}

//------------------------------------------------------------------------------
int et_s1ap_decode_pdu(S1AP_PDU_t * const pdu, s1ap_message * const message, const uint8_t * const buffer, const uint32_t length)
{
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  memset((void *)pdu, 0, sizeof(S1AP_PDU_t));

  dec_ret = aper_decode(NULL,
                        &asn_DEF_S1AP_PDU,
                        (void **)&pdu,
                        buffer,
                        length,
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    S1AP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  message->direction = pdu->present;

  switch(pdu->present) {
  case S1AP_PDU_PR_initiatingMessage:
    return et_s1ap_decode_initiating_message(message,
           &pdu->choice.initiatingMessage);

  case S1AP_PDU_PR_successfulOutcome:
    return et_s1ap_decode_successful_outcome(message,
           &pdu->choice.successfulOutcome);

  case S1AP_PDU_PR_unsuccessfulOutcome:
    return et_s1ap_decode_unsuccessful_outcome(message,
           &pdu->choice.unsuccessfulOutcome);

  default:
    AssertFatal(0, "Unknown presence (%d) or not implemented\n", (int)pdu->present);
    break;
  }
  return -1;
}
//------------------------------------------------------------------------------
void et_decode_s1ap(et_s1ap_t * const s1ap)
{
  if (NULL != s1ap) {
    if (et_s1ap_decode_pdu(&s1ap->pdu, &s1ap->message, s1ap->binary_stream, s1ap->binary_stream_allocated_size) < 0) {
      AssertFatal (0, "ERROR %s() Cannot decode S1AP message!\n", __FUNCTION__);
    }
  }
}
