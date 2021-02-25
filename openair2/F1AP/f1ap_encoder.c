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

/*! \file f1ap_encoder.c
 * \brief f1ap pdu encode procedures
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"

int asn1_encoder_xer_print = 1;

/*
static inline int f1ap_encode_initiating(f1ap_message *message,
    uint8_t **buffer,
    uint32_t *len);

static inline int f1ap_encode_successfull_outcome(f1ap_message *message,
    uint8_t **buffer, uint32_t *len);

static inline int f1ap_encode_unsuccessfull_outcome(f1ap_message *message,
    uint8_t **buffer, uint32_t *len);

static inline int f1ap_encode_f1_setup_request(
  F1ap_F1SetupRequestIEs_t *f1SetupRequestIEs, uint8_t **buffer, uint32_t *length);

static inline int f1ap_encode_trace_failure(F1ap_TraceFailureIndicationIEs_t
    *trace_failure_ies_p, uint8_t **buffer,
    uint32_t *length);

static inline int f1ap_encode_initial_context_setup_response(
  F1ap_InitialContextSetupResponseIEs_t *initialContextSetupResponseIEs,
  uint8_t **buffer,
  uint32_t *length);

static inline
int f1ap_encode_ue_context_release_complete(
  F1ap_UEContextReleaseCompleteIEs_t *f1ap_UEContextReleaseCompleteIEs,
  uint8_t                           **buffer,
  uint32_t                           *length);

static inline
int f1ap_encode_ue_context_release_request(
  F1ap_UEContextReleaseRequestIEs_t *f1ap_UEContextReleaseRequestIEs,
  uint8_t                              **buffer,
  uint32_t                              *length);
  */

int f1ap_encode_pdu(F1AP_F1AP_PDU_t *pdu, uint8_t **buffer, uint32_t *length)
{
  ssize_t    encoded;
  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  DevAssert(length != NULL);

  if (asn1_encoder_xer_print) {
    LOG_E(F1AP, "----------------- ASN1 ENCODER PRINT START ----------------- \n");
    xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, pdu);
    LOG_E(F1AP, "----------------- ASN1 ENCODER PRINT END----------------- \n");
  }

  encoded = aper_encode_to_new_buffer(&asn_DEF_F1AP_F1AP_PDU, 0, pdu, (void **)buffer);

  if (encoded < 0) {
    LOG_E(F1AP, "Failed to encode F1AP message\n");
    return -1;
  }

  *length = encoded;

  return encoded;
}

/*
static inline
int f1ap_encode_initiating(f1ap_message *f1ap_message_p,
                               uint8_t **buffer, uint32_t *len)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(f1ap_message_p != NULL);

  message_string = calloc(10000, sizeof(char));

  f1ap_string_total_size = 0;

  switch(f1ap_message_p->procedureCode) {
  case F1ap_ProcedureCode_id_F1Setup:
    ret = f1ap_encode_f1_setup_request(
            &f1ap_message_p->msg.f1ap_F1SetupRequestIEs, buffer, len);
    //f1ap_xer_print_f1ap_f1setuprequest(f1ap_xer__print2sp, message_string, f1ap_message_p);
    message_id = F1AP_F1_SETUP_LOG;
    break;

  case F1ap_ProcedureCode_id_UEContextReleaseRequest:
    ret = f1ap_encode_ue_context_release_request(
            &f1ap_message_p->msg.f1ap_UEContextReleaseRequestIEs, buffer, len);
    //f1ap_xer_print_f1ap_uecontextreleaserequest(f1ap_xer__print2sp,
    //    message_string, f1ap_message_p);
    message_id = F1AP_UE_CONTEXT_RELEASE_REQ_LOG;
    break;


  default:
    F1AP_DEBUG("Unknown procedure ID (%d) for initiating message\n",
               (int)f1ap_message_p->procedureCode);
    return ret;
    break;
  }

  message_string_size = strlen(message_string);

  message_p = itti_alloc_new_message_sized(TASK_F1AP, message_id, message_string_size + sizeof (IttiMsgText));
  message_p->ittiMsg.f1ap_f1_setup_log.size = message_string_size;
  memcpy(&message_p->ittiMsg.f1ap_f1_setup_log.text, message_string, message_string_size);

  itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

  free(message_string);

  return ret;
}

static inline
int f1ap_encode_successfull_outcome(f1ap_message *f1ap_message_p,
                                        uint8_t **buffer, uint32_t *len)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(f1ap_message_p != NULL);

  message_string = calloc(10000, sizeof(char));

  f1ap_string_total_size = 0;
  message_string_size = strlen(message_string);


  switch(f1ap_message_p->procedureCode) {
  case F1ap_ProcedureCode_id_InitialContextSetup:
    ret = f1ap_encode_initial_context_setup_response(
            &f1ap_message_p->msg.f1ap_InitialContextSetupResponseIEs, buffer, len);

    f1ap_xer_print_f1ap_initialcontextsetupresponse(f1ap_xer__print2sp, message_string, f1ap_message_p);
    message_id = F1AP_INITIAL_CONTEXT_SETUP_LOG;
    message_p = itti_alloc_new_message_sized(TASK_F1AP, message_id, message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.f1ap_initial_context_setup_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.f1ap_initial_context_setup_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  case F1ap_ProcedureCode_id_UEContextRelease:
    ret = f1ap_encode_ue_context_release_complete(
            &f1ap_message_p->msg.f1ap_UEContextReleaseCompleteIEs, buffer, len);
    f1ap_xer_print_f1ap_uecontextreleasecomplete(f1ap_xer__print2sp, message_string, f1ap_message_p);
    message_id = F1AP_UE_CONTEXT_RELEASE_COMPLETE_LOG;
    message_p = itti_alloc_new_message_sized(TASK_F1AP, message_id, message_string_size + sizeof (IttiMsgText));
    message_p->ittiMsg.f1ap_ue_context_release_complete_log.size = message_string_size;
    memcpy(&message_p->ittiMsg.f1ap_ue_context_release_complete_log.text, message_string, message_string_size);
    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
    free(message_string);
    break;

  default:
    F1AP_WARN("Unknown procedure ID (%d) for successfull outcome message\n",
               (int)f1ap_message_p->procedureCode);
    return ret;
    break;
  }


  return ret;
}

static inline
int f1ap_encode_unsuccessfull_outcome(f1ap_message *f1ap_message_p,
    uint8_t **buffer, uint32_t *len)
{
  int ret = -1;
  MessageDef *message_p;
  char       *message_string = NULL;
  size_t      message_string_size;
  MessagesIds message_id;

  DevAssert(f1ap_message_p != NULL);

  message_string = calloc(10000, sizeof(char));

  f1ap_string_total_size = 0;

  switch(f1ap_message_p->procedureCode) {
  case F1ap_ProcedureCode_id_InitialContextSetup:
    //             ret = f1ap_encode_f1ap_initialcontextsetupfailureies(
    //                 &f1ap_message_p->ittiMsg.f1ap_InitialContextSetupFailureIEs, buffer, len);
    f1ap_xer_print_f1ap_initialcontextsetupfailure(f1ap_xer__print2sp, message_string, f1ap_message_p);
    message_id = F1AP_INITIAL_CONTEXT_SETUP_LOG;
    break;

  default:
    F1AP_DEBUG("Unknown procedure ID (%d) for unsuccessfull outcome message\n",
               (int)f1ap_message_p->procedureCode);
    return ret;
    break;
  }

  message_string_size = strlen(message_string);

  message_p = itti_alloc_new_message_sized(TASK_F1AP, message_id, message_string_size + sizeof (IttiMsgText));
  message_p->ittiMsg.f1ap_initial_context_setup_log.size = message_string_size;
  memcpy(&message_p->ittiMsg.f1ap_initial_context_setup_log.text, message_string, message_string_size);

  itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

  free(message_string);

  return ret;
}

static inline
int f1ap_encode_f1_setup_request(
  F1ap_F1SetupRequestIEs_t *f1SetupRequestIEs,
  uint8_t                 **buffer,
  uint32_t                 *length)
{
  F1ap_F1SetupRequest_t  f1SetupRequest;
  F1ap_F1SetupRequest_t *f1SetupRequest_p = &f1SetupRequest;

  memset((void *)f1SetupRequest_p, 0, sizeof(f1SetupRequest));

  if (f1ap_encode_f1ap_f1setuprequesties(f1SetupRequest_p, f1SetupRequestIEs) < 0) {
    return -1;
  }

  return f1ap_generate_initiating_message(buffer,
                                          length,
                                          F1ap_ProcedureCode_id_F1Setup,
                                          F1ap_Criticality_reject,
                                          &asn_DEF_F1ap_F1SetupRequest,
                                          f1SetupRequest_p);
}

static inline
int f1ap_encode_trace_failure(
  F1ap_TraceFailureIndicationIEs_t *trace_failure_ies_p,
  uint8_t                         **buffer,
  uint32_t                         *length)
{
  F1ap_TraceFailureIndication_t  trace_failure;
  F1ap_TraceFailureIndication_t *trace_failure_p = &trace_failure;

  memset((void *)trace_failure_p, 0, sizeof(trace_failure));

  if (f1ap_encode_f1ap_tracefailureindicationies(
        trace_failure_p, trace_failure_ies_p) < 0) {
    return -1;
  }

  return f1ap_generate_initiating_message(buffer,
                                          length,
                                          F1ap_ProcedureCode_id_TraceFailureIndication,
                                          F1ap_Criticality_reject,
                                          &asn_DEF_F1ap_TraceFailureIndication,
                                          trace_failure_p);
}

static inline
int f1ap_encode_initial_context_setup_response(
  F1ap_InitialContextSetupResponseIEs_t *initialContextSetupResponseIEs,
  uint8_t                              **buffer,
  uint32_t                              *length)
{
  F1ap_InitialContextSetupResponse_t  initial_context_setup_response;
  F1ap_InitialContextSetupResponse_t *initial_context_setup_response_p =
    &initial_context_setup_response;

  memset((void *)initial_context_setup_response_p, 0,
         sizeof(initial_context_setup_response));

  if (f1ap_encode_f1ap_initialcontextsetupresponseies(
        initial_context_setup_response_p, initialContextSetupResponseIEs) < 0) {
    return -1;
  }

  return f1ap_generate_successfull_outcome(buffer,
         length,
         F1ap_ProcedureCode_id_InitialContextSetup,
         F1ap_Criticality_reject,
         &asn_DEF_F1ap_InitialContextSetupResponse,
         initial_context_setup_response_p);
}

static inline
int f1ap_encode_ue_context_release_complete(
  F1ap_UEContextReleaseCompleteIEs_t *f1ap_UEContextReleaseCompleteIEs,
  uint8_t                              **buffer,
  uint32_t                              *length)
{
  F1ap_UEContextReleaseComplete_t  ue_context_release_complete;
  F1ap_UEContextReleaseComplete_t *ue_context_release_complete_p =
    &ue_context_release_complete;

  memset((void *)ue_context_release_complete_p, 0,
         sizeof(ue_context_release_complete));

  if (f1ap_encode_f1ap_uecontextreleasecompleteies(
        ue_context_release_complete_p, f1ap_UEContextReleaseCompleteIEs) < 0) {
    return -1;
  }

  return f1ap_generate_successfull_outcome(buffer,
         length,
         F1ap_ProcedureCode_id_UEContextRelease,
         F1ap_Criticality_reject,
         &asn_DEF_F1ap_UEContextReleaseComplete,
         ue_context_release_complete_p);
}

static inline
int f1ap_encode_ue_context_release_request(
  F1ap_UEContextReleaseRequestIEs_t *f1ap_UEContextReleaseRequestIEs,
  uint8_t                              **buffer,
  uint32_t                              *length)
{
  F1ap_UEContextReleaseRequest_t  ue_context_release_request;
  F1ap_UEContextReleaseRequest_t *ue_context_release_request_p =
    &ue_context_release_request;

  memset((void *)ue_context_release_request_p, 0,
         sizeof(ue_context_release_request));

  if (f1ap_encode_f1ap_uecontextreleaserequesties(
        ue_context_release_request_p, f1ap_UEContextReleaseRequestIEs) < 0) {
    return -1;
  }

  return f1ap_generate_initiating_message(buffer,
                                          length,
                                          F1ap_ProcedureCode_id_UEContextReleaseRequest,
                                          F1ap_Criticality_reject,
                                          &asn_DEF_F1ap_UEContextReleaseRequest,
                                          ue_context_release_request_p);
}
*/
