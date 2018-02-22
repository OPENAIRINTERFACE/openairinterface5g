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

/*! \file s1ap_eNB_encoder.c
 * \brief s1ap pdu encode procedures for eNB
 * \author Sebastien ROUX and Navid Nikaein
 * \email navid.nikaein@eurecom.fr
 * \date 2013 - 2015
 * \version 0.1
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "assertions.h"

#include "conversions.h"

#include "intertask_interface.h"

#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
#include "s1ap_eNB_encoder.h"

static inline int s1ap_eNB_encode_initiating(s1ap_message *message,
    uint8_t **buffer,
    uint32_t *len);

static inline int s1ap_eNB_encode_successfull_outcome(s1ap_message *message,
    uint8_t **buffer, uint32_t *len);

static inline int s1ap_eNB_encode_unsuccessfull_outcome(s1ap_message *message,
    uint8_t **buffer, uint32_t *len);

static inline int s1ap_eNB_encode_s1_setup_request(
  S1ap_S1SetupRequestIEs_t *s1SetupRequestIEs, uint8_t **buffer, uint32_t *length);

static inline int s1ap_eNB_encode_trace_failure(S1ap_TraceFailureIndicationIEs_t
    *trace_failure_ies_p, uint8_t **buffer,
    uint32_t *length);

static inline int s1ap_eNB_encode_initial_ue_message(S1ap_InitialUEMessageIEs_t
    *initialUEmessageIEs_p, uint8_t **buffer,
    uint32_t *length);

static inline int s1ap_eNB_encode_uplink_nas_transport(S1ap_UplinkNASTransportIEs_t
    *uplinkNASTransportIEs,
    uint8_t **buffer,
    uint32_t *length);

static inline int s1ap_eNB_encode_ue_capability_info_indication(
  S1ap_UECapabilityInfoIndicationIEs_t *ueCapabilityInfoIndicationIEs,
  uint8_t **buffer,
  uint32_t *length);

static inline int s1ap_eNB_encode_initial_context_setup_response(
  S1ap_InitialContextSetupResponseIEs_t *initialContextSetupResponseIEs,
  uint8_t **buffer,
  uint32_t *length);

static inline
int s1ap_eNB_encode_nas_non_delivery(
  S1ap_NASNonDeliveryIndication_IEs_t *nasNonDeliveryIndicationIEs,
  uint8_t                            **buffer,
  uint32_t                            *length);

static inline
int s1ap_eNB_encode_ue_context_release_complete(
  S1ap_UEContextReleaseCompleteIEs_t *s1ap_UEContextReleaseCompleteIEs,
  uint8_t                           **buffer,
  uint32_t                           *length);

static inline
int s1ap_eNB_encode_ue_context_release_request(
  S1ap_UEContextReleaseRequestIEs_t *s1ap_UEContextReleaseRequestIEs,
  uint8_t                              **buffer,
  uint32_t                              *length);

static inline
int s1ap_eNB_encode_e_rab_setup_response(S1ap_E_RABSetupResponseIEs_t  *E_RABSetupResponseIEs,
					 uint8_t                              **buffer,
					 uint32_t                              *length);

static inline
int s1ap_eNB_encode_e_rab_modify_response(S1ap_E_RABModifyResponseIEs_t  *E_RABModifyResponseIEs,
           uint8_t                              **buffer,
           uint32_t                              *length);

static inline
int s1ap_eNB_encode_e_rab_release_response(S1ap_E_RABReleaseResponseIEs_t  *s1ap_E_RABReleaseResponseIEs,
                     uint8_t                              **buffer,
                     uint32_t                              *length);

int s1ap_eNB_encode_pdu(s1ap_message *message, uint8_t **buffer, uint32_t *len)
{
  DevAssert(message != NULL);
  DevAssert(buffer != NULL);
  DevAssert(len != NULL);

  switch(message->direction) {
  case S1AP_PDU_PR_initiatingMessage:
    return s1ap_eNB_encode_initiating(message, buffer, len);

  case S1AP_PDU_PR_successfulOutcome:
    return s1ap_eNB_encode_successfull_outcome(message, buffer, len);

  case S1AP_PDU_PR_unsuccessfulOutcome:
    return s1ap_eNB_encode_unsuccessfull_outcome(message, buffer, len);

  default:
    S1AP_DEBUG("Unknown message outcome (%d) or not implemented",
               (int)message->direction);
    break;
  }

  return -1;
}

static inline
int s1ap_eNB_encode_initiating(s1ap_message *s1ap_message_p,
                               uint8_t **buffer, uint32_t *len)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(s1ap_message_p != NULL);

  message_string = calloc(10000, sizeof(char));

  s1ap_string_total_size = 0;

  switch(s1ap_message_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_eNB_encode_s1_setup_request(
            &s1ap_message_p->msg.s1ap_S1SetupRequestIEs, buffer, len);
    s1ap_xer_print_s1ap_s1setuprequest(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id = S1AP_S1_SETUP_LOG;
    break;

  case S1ap_ProcedureCode_id_uplinkNASTransport:
    ret = s1ap_eNB_encode_uplink_nas_transport(
            &s1ap_message_p->msg.s1ap_UplinkNASTransportIEs, buffer, len);
    s1ap_xer_print_s1ap_uplinknastransport(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id = S1AP_UPLINK_NAS_LOG;
    break;

  case S1ap_ProcedureCode_id_UECapabilityInfoIndication:
    ret = s1ap_eNB_encode_ue_capability_info_indication(
            &s1ap_message_p->msg.s1ap_UECapabilityInfoIndicationIEs, buffer, len);
    s1ap_xer_print_s1ap_uecapabilityinfoindication(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id = S1AP_UE_CAPABILITY_IND_LOG;
    break;

  case S1ap_ProcedureCode_id_initialUEMessage:
    ret = s1ap_eNB_encode_initial_ue_message(
            &s1ap_message_p->msg.s1ap_InitialUEMessageIEs, buffer, len);
    s1ap_xer_print_s1ap_initialuemessage(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id = S1AP_INITIAL_UE_MESSAGE_LOG;
    break;

  case S1ap_ProcedureCode_id_NASNonDeliveryIndication:
    ret = s1ap_eNB_encode_nas_non_delivery(
            &s1ap_message_p->msg.s1ap_NASNonDeliveryIndication_IEs, buffer, len);
    s1ap_xer_print_s1ap_nasnondeliveryindication_(s1ap_xer__print2sp,
        message_string, s1ap_message_p);
    message_id = S1AP_NAS_NON_DELIVERY_IND_LOG;
    break;

  case S1ap_ProcedureCode_id_UEContextReleaseRequest:
    ret = s1ap_eNB_encode_ue_context_release_request(
            &s1ap_message_p->msg.s1ap_UEContextReleaseRequestIEs, buffer, len);
    s1ap_xer_print_s1ap_uecontextreleaserequest(s1ap_xer__print2sp,
        message_string, s1ap_message_p);
    message_id = S1AP_UE_CONTEXT_RELEASE_REQ_LOG;
    break;


  default:
    S1AP_DEBUG("Unknown procedure ID (%d) for initiating message\n",
               (int)s1ap_message_p->procedureCode);
    return ret;
    break;
  }

  message_string_size = strlen(message_string);

  message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
  message_p->ittiMsg.s1ap_s1_setup_log.size = message_string_size;
  memcpy(&message_p->ittiMsg.s1ap_s1_setup_log.text, message_string, message_string_size);

  itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

  free(message_string);

  return ret;
}

static inline
int s1ap_eNB_encode_successfull_outcome(s1ap_message *s1ap_message_p,
                                        uint8_t **buffer, uint32_t *len)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(s1ap_message_p != NULL);

  message_string = calloc(10000, sizeof(char));

  s1ap_string_total_size = 0;
  message_string_size = strlen(message_string);


  switch(s1ap_message_p->procedureCode) {
  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_eNB_encode_initial_context_setup_response(
            &s1ap_message_p->msg.s1ap_InitialContextSetupResponseIEs, buffer, len);

    s1ap_xer_print_s1ap_initialcontextsetupresponse(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id = S1AP_INITIAL_CONTEXT_SETUP_LOG;
    message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_initial_context_setup_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_initial_context_setup_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case S1ap_ProcedureCode_id_UEContextRelease:
    ret = s1ap_eNB_encode_ue_context_release_complete(
            &s1ap_message_p->msg.s1ap_UEContextReleaseCompleteIEs, buffer, len);
    s1ap_xer_print_s1ap_uecontextreleasecomplete(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id = S1AP_UE_CONTEXT_RELEASE_COMPLETE_LOG;
    message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_ue_context_release_complete_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_ue_context_release_complete_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case S1ap_ProcedureCode_id_E_RABSetup:

    ret = s1ap_eNB_encode_e_rab_setup_response (
           &s1ap_message_p->msg.s1ap_E_RABSetupResponseIEs, buffer, len);
    //s1ap_xer_print_s1ap_e_rabsetupresponse (s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id =  S1AP_E_RAB_SETUP_RESPONSE_LOG ;
    message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_e_rab_setup_response_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_e_rab_setup_response_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    S1AP_INFO("E_RABSetup successful message\n");
    break;

  case S1ap_ProcedureCode_id_E_RABModify:
    ret = s1ap_eNB_encode_e_rab_modify_response (
           &s1ap_message_p->msg.s1ap_E_RABModifyResponseIEs, buffer, len);
    message_id =  S1AP_E_RAB_MODIFY_RESPONSE_LOG ;
    message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_e_rab_modify_response_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_e_rab_modify_response_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    S1AP_INFO("E_RABModify successful message\n");
    break;

  case S1ap_ProcedureCode_id_E_RABRelease:
    ret = s1ap_eNB_encode_e_rab_release_response (
           &s1ap_message_p->msg.s1ap_E_RABReleaseResponseIEs, buffer, len);
    s1ap_xer_print_s1ap_e_rabreleaseresponse(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id =  S1AP_E_RAB_RELEASE_RESPONSE_LOG ;
    message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.s1ap_e_rab_release_response_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.s1ap_e_rab_release_response_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    S1AP_INFO("E_RAB Release successful message\n");
    break;

  default:
    S1AP_WARN("Unknown procedure ID (%d) for successfull outcome message\n",
               (int)s1ap_message_p->procedureCode);
    return ret;
    break;
  }


  return ret;
}

static inline
int s1ap_eNB_encode_unsuccessfull_outcome(s1ap_message *s1ap_message_p,
    uint8_t **buffer, uint32_t *len)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(s1ap_message_p != NULL);

  message_string = calloc(10000, sizeof(char));

  s1ap_string_total_size = 0;

  switch(s1ap_message_p->procedureCode) {
  case S1ap_ProcedureCode_id_InitialContextSetup:
    //             ret = s1ap_encode_s1ap_initialcontextsetupfailureies(
    //                 &s1ap_message_p->ittiMsg.s1ap_InitialContextSetupFailureIEs, buffer, len);
    s1ap_xer_print_s1ap_initialcontextsetupfailure(s1ap_xer__print2sp, message_string, s1ap_message_p);
    message_id = S1AP_INITIAL_CONTEXT_SETUP_LOG;
    break;

  default:
    S1AP_DEBUG("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
               (int)s1ap_message_p->procedureCode);
    return ret;
    break;
  }

  message_string_size = strlen(message_string);

  message_p = itti_alloc_new_message_sized(TASK_S1AP, message_id, message_string_size + sizeof (IttiMsgText));
  message_p->ittiMsg.s1ap_initial_context_setup_log.size = message_string_size;
  memcpy(&message_p->ittiMsg.s1ap_initial_context_setup_log.text, message_string, message_string_size);

  itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

  free(message_string);

  return ret;
}

static inline
int s1ap_eNB_encode_ue_capability_info_indication(
  S1ap_UECapabilityInfoIndicationIEs_t *ueCapabilityInfoIndicationIEs,
  uint8_t                             **buffer,
  uint32_t                             *length)
{
  S1ap_UECapabilityInfoIndication_t  ueCapabilityInfoIndication;
  S1ap_UECapabilityInfoIndication_t *ueCapabilityInfoIndication_p =
    &ueCapabilityInfoIndication;

  memset((void *)ueCapabilityInfoIndication_p, 0,  sizeof(ueCapabilityInfoIndication));

  if (s1ap_encode_s1ap_uecapabilityinfoindicationies(
        ueCapabilityInfoIndication_p, ueCapabilityInfoIndicationIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer,
                                          length,
                                          S1ap_ProcedureCode_id_UECapabilityInfoIndication,
                                          S1ap_Criticality_ignore,
                                          &asn_DEF_S1ap_UECapabilityInfoIndication,
                                          ueCapabilityInfoIndication_p);
}

static inline
int s1ap_eNB_encode_uplink_nas_transport(
  S1ap_UplinkNASTransportIEs_t *uplinkNASTransportIEs,
  uint8_t                     **buffer,
  uint32_t                     *length)
{
  S1ap_UplinkNASTransport_t  uplinkNASTransport;
  S1ap_UplinkNASTransport_t *uplinkNASTransport_p = &uplinkNASTransport;

  memset((void *)uplinkNASTransport_p, 0, sizeof(uplinkNASTransport));

  if (s1ap_encode_s1ap_uplinknastransporties(
        uplinkNASTransport_p, uplinkNASTransportIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer,
                                          length,
                                          S1ap_ProcedureCode_id_uplinkNASTransport,
                                          S1ap_Criticality_ignore,
                                          &asn_DEF_S1ap_UplinkNASTransport,
                                          uplinkNASTransport_p);
}

static inline
int s1ap_eNB_encode_nas_non_delivery(
  S1ap_NASNonDeliveryIndication_IEs_t *nasNonDeliveryIndicationIEs,
  uint8_t                            **buffer,
  uint32_t                            *length)
{
  S1ap_NASNonDeliveryIndication_t  nasNonDeliveryIndication;
  S1ap_NASNonDeliveryIndication_t *nasNonDeliveryIndication_p = &nasNonDeliveryIndication;

  memset((void *)nasNonDeliveryIndication_p, 0, sizeof(nasNonDeliveryIndication));

  if (s1ap_encode_s1ap_nasnondeliveryindication_ies(
        nasNonDeliveryIndication_p, nasNonDeliveryIndicationIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer,
                                          length,
                                          S1ap_ProcedureCode_id_NASNonDeliveryIndication,
                                          S1ap_Criticality_ignore,
                                          &asn_DEF_S1ap_NASNonDeliveryIndication,
                                          nasNonDeliveryIndication_p);
}

static inline
int s1ap_eNB_encode_s1_setup_request(
  S1ap_S1SetupRequestIEs_t *s1SetupRequestIEs,
  uint8_t                 **buffer,
  uint32_t                 *length)
{
  S1ap_S1SetupRequest_t  s1SetupRequest;
  S1ap_S1SetupRequest_t *s1SetupRequest_p = &s1SetupRequest;

  memset((void *)s1SetupRequest_p, 0, sizeof(s1SetupRequest));

  if (s1ap_encode_s1ap_s1setuprequesties(s1SetupRequest_p, s1SetupRequestIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer,
                                          length,
                                          S1ap_ProcedureCode_id_S1Setup,
                                          S1ap_Criticality_reject,
                                          &asn_DEF_S1ap_S1SetupRequest,
                                          s1SetupRequest_p);
}

static inline
int s1ap_eNB_encode_initial_ue_message(
  S1ap_InitialUEMessageIEs_t *initialUEmessageIEs_p,
  uint8_t                   **buffer,
  uint32_t                   *length)
{
  S1ap_InitialUEMessage_t  initialUEMessage;
  S1ap_InitialUEMessage_t *initialUEMessage_p = &initialUEMessage;

  memset((void *)initialUEMessage_p, 0, sizeof(initialUEMessage));

  if (s1ap_encode_s1ap_initialuemessageies(
        initialUEMessage_p, initialUEmessageIEs_p) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer,
                                          length,
                                          S1ap_ProcedureCode_id_initialUEMessage,
                                          S1ap_Criticality_ignore,
                                          &asn_DEF_S1ap_InitialUEMessage,
                                          initialUEMessage_p);
}

static inline
int s1ap_eNB_encode_trace_failure(
  S1ap_TraceFailureIndicationIEs_t *trace_failure_ies_p,
  uint8_t                         **buffer,
  uint32_t                         *length)
{
  S1ap_TraceFailureIndication_t  trace_failure;
  S1ap_TraceFailureIndication_t *trace_failure_p = &trace_failure;

  memset((void *)trace_failure_p, 0, sizeof(trace_failure));

  if (s1ap_encode_s1ap_tracefailureindicationies(
        trace_failure_p, trace_failure_ies_p) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer,
                                          length,
                                          S1ap_ProcedureCode_id_TraceFailureIndication,
                                          S1ap_Criticality_reject,
                                          &asn_DEF_S1ap_TraceFailureIndication,
                                          trace_failure_p);
}

static inline
int s1ap_eNB_encode_initial_context_setup_response(
  S1ap_InitialContextSetupResponseIEs_t *initialContextSetupResponseIEs,
  uint8_t                              **buffer,
  uint32_t                              *length)
{
  S1ap_InitialContextSetupResponse_t  initial_context_setup_response;
  S1ap_InitialContextSetupResponse_t *initial_context_setup_response_p =
    &initial_context_setup_response;

  memset((void *)initial_context_setup_response_p, 0,
         sizeof(initial_context_setup_response));

  if (s1ap_encode_s1ap_initialcontextsetupresponseies(
        initial_context_setup_response_p, initialContextSetupResponseIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome(buffer,
         length,
         S1ap_ProcedureCode_id_InitialContextSetup,
         S1ap_Criticality_reject,
         &asn_DEF_S1ap_InitialContextSetupResponse,
         initial_context_setup_response_p);
}

static inline
int s1ap_eNB_encode_ue_context_release_complete(
  S1ap_UEContextReleaseCompleteIEs_t *s1ap_UEContextReleaseCompleteIEs,
  uint8_t                              **buffer,
  uint32_t                              *length)
{
  S1ap_UEContextReleaseComplete_t  ue_context_release_complete;
  S1ap_UEContextReleaseComplete_t *ue_context_release_complete_p =
    &ue_context_release_complete;

  memset((void *)ue_context_release_complete_p, 0,
         sizeof(ue_context_release_complete));

  if (s1ap_encode_s1ap_uecontextreleasecompleteies(
        ue_context_release_complete_p, s1ap_UEContextReleaseCompleteIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome(buffer,
         length,
         S1ap_ProcedureCode_id_UEContextRelease,
         S1ap_Criticality_reject,
         &asn_DEF_S1ap_UEContextReleaseComplete,
         ue_context_release_complete_p);
}

static inline
int s1ap_eNB_encode_ue_context_release_request(
  S1ap_UEContextReleaseRequestIEs_t *s1ap_UEContextReleaseRequestIEs,
  uint8_t                              **buffer,
  uint32_t                              *length)
{
  S1ap_UEContextReleaseRequest_t  ue_context_release_request;
  S1ap_UEContextReleaseRequest_t *ue_context_release_request_p =
    &ue_context_release_request;

  memset((void *)ue_context_release_request_p, 0,
         sizeof(ue_context_release_request));

  if (s1ap_encode_s1ap_uecontextreleaserequesties(
        ue_context_release_request_p, s1ap_UEContextReleaseRequestIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer,
                                          length,
                                          S1ap_ProcedureCode_id_UEContextReleaseRequest,
                                          S1ap_Criticality_reject,
                                          &asn_DEF_S1ap_UEContextReleaseRequest,
                                          ue_context_release_request_p);
}

static inline
int s1ap_eNB_encode_e_rab_setup_response(S1ap_E_RABSetupResponseIEs_t  *s1ap_E_RABSetupResponseIEs,
					 uint8_t                              **buffer,
					 uint32_t                              *length)
{
  S1ap_E_RABSetupResponse_t  e_rab_setup_response;
  S1ap_E_RABSetupResponse_t  *e_rab_setup_response_p = &e_rab_setup_response;
  
  memset((void *)e_rab_setup_response_p, 0,
         sizeof(e_rab_setup_response));

  if (s1ap_encode_s1ap_e_rabsetupresponseies (e_rab_setup_response_p, s1ap_E_RABSetupResponseIEs) < 0) {
    return -1;
  }
  
  return s1ap_generate_successfull_outcome(buffer,
         length,
         S1ap_ProcedureCode_id_E_RABSetup,
         S1ap_Criticality_reject,
         &asn_DEF_S1ap_E_RABSetupResponse,
         e_rab_setup_response_p);
}

static inline
int s1ap_eNB_encode_e_rab_modify_response(S1ap_E_RABModifyResponseIEs_t  *s1ap_E_RABModifyResponseIEs,
           uint8_t                              **buffer,
           uint32_t                              *length)
{
  S1ap_E_RABModifyResponse_t  e_rab_modify_response;
  S1ap_E_RABModifyResponse_t  *e_rab_modify_response_p = &e_rab_modify_response;

  memset((void *)e_rab_modify_response_p, 0,
         sizeof(e_rab_modify_response));

  if (s1ap_encode_s1ap_e_rabmodifyresponseies (e_rab_modify_response_p, s1ap_E_RABModifyResponseIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome(buffer,
         length,
         S1ap_ProcedureCode_id_E_RABModify,
         S1ap_Criticality_reject,
         &asn_DEF_S1ap_E_RABModifyResponse,
         e_rab_modify_response_p);
}
static inline
int s1ap_eNB_encode_e_rab_release_response(S1ap_E_RABReleaseResponseIEs_t  *s1ap_E_RABReleaseResponseIEs,
                     uint8_t                              **buffer,
                     uint32_t                              *length)
{
    S1ap_E_RABReleaseResponse_t  e_rab_release_response;
    S1ap_E_RABReleaseResponse_t  *e_rab_release_response_p = &e_rab_release_response;

  memset((void *)e_rab_release_response_p, 0,
         sizeof(e_rab_release_response));

  if (s1ap_encode_s1ap_e_rabreleaseresponseies (e_rab_release_response_p, s1ap_E_RABReleaseResponseIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome(buffer,
         length,
         S1ap_ProcedureCode_id_E_RABRelease,
         S1ap_Criticality_reject,
         &asn_DEF_S1ap_E_RABReleaseResponse,
         e_rab_release_response_p);
}
