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

/*! \file m3ap_MCE_interface_management.c
 * \brief m3ap interface management for MCE
 * \author Javier Morgade
 * \date 2019
 * \version 0.1
 * \company Vicomtech
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#include "intertask_interface.h"

#include "m3ap_common.h"
#include "m3ap_MCE.h"
//#include "m3ap_MCE_generate_messages.h"
#include "m3ap_encoder.h"
#include "m3ap_decoder.h"
#include "m3ap_ids.h"

#include "m3ap_MCE_interface_management.h"


#include "m3ap_itti_messaging.h"

#include "msc.h"
#include "assertions.h"
#include "conversions.h"


//#include "m3ap_common.h"
//#include "m3ap_encoder.h"
//#include "m3ap_decoder.h"
//#include "m3ap_itti_messaging.h"
//#include "m3ap_MCE_interface_management.h"
//#include "assertions.h"

extern m3ap_setup_req_t * m3ap_mce_data_g;


int MCE_handle_MBMS_SESSION_START_REQUEST(instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        M3AP_M3AP_PDU_t *pdu){
//  LOG_W(M3AP, "MCE_handle_MBMS_SESSION_START_REQUEST assoc_id %d\n",assoc_id);
//
    MessageDef                         *message_p;
//  M3AP_SessionStartRequest_t              *container;
//  M3AP_SessionStartRequest_Ies_t           *ie;
//  int i = 0;
//
//  DevAssert(pdu != NULL);
//
//  container = &pdu->choice.initiatingMessage.value.choice.SessionStartRequest;

    message_p = itti_alloc_new_message(TASK_M3AP_MCE, 0, M3AP_MBMS_SESSION_START_REQ);

//
//  /* M3 Setup Request == Non UE-related procedure -> stream 0 */
//  if (stream != 0) {
//    LOG_W(M3AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
//              assoc_id, stream);
//  }
//

    itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);

//  if(1){
//	MCE_send_MBMS_SESSION_START_RESPONSE(instance,assoc_id);
//  }else
//	MCE_send_MBMS_SESSION_START_FAILURE(instance,assoc_id);
  return 0;
  
}

int MCE_send_MBMS_SESSION_START_RESPONSE(instance_t instance, m3ap_session_start_resp_t * m3ap_session_start_resp){
  //AssertFatal(1==0,"Not implemented yet\n");

//	module_id_t mce_mod_idP;
//  module_id_t enb_mod_idP;
//
//  // This should be fixed
//  enb_mod_idP = (module_id_t)0;
//  mce_mod_idP  = (module_id_t)0;
//
  M3AP_M3AP_PDU_t           pdu;
  M3AP_MBMSSessionStartResponse_t    *out;
  M3AP_MBMSSessionStartResponse_IEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_successfulOutcome;
  //pdu.choice.successfulOutcome = (M3AP_SuccessfulOutcome_t *)calloc(1, sizeof(M3AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome.procedureCode = M3AP_ProcedureCode_id_mBMSsessionStart;
  pdu.choice.successfulOutcome.criticality   = M3AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M3AP_SuccessfulOutcome__value_PR_MBMSSessionStartResponse;
  out = &pdu.choice.successfulOutcome.value.choice.MBMSSessionStartResponse;
  

  /* mandatory */
  /* c1. MCE_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionStartResponse_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartResponse_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStartResponse_IEs__value_PR_MCE_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

 /* mandatory */
  /* c1. MME_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionStartResponse_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartResponse_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStartResponse_IEs__value_PR_MME_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (m3ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M3AP, "Failed to encode M3 SessionStart Response\n");
    return -1;
  }
//
//
//  LOG_W(M3AP,"pdu.present %d\n",pdu.present);
// // MSC_LOG_TX_MESSAGE(
// // MSC_M3AP_MCE,
// // MSC_M3AP_MCE,
// // (const char *)buffer,
// // len,
// // MSC_AS_TIME_FMT" M3_SETUP_REQUEST initiatingMessage MCEname %s",
// // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
// // m3ap_MCE_data_p->MCEname);
//
   m3ap_MCE_itti_send_sctp_data_req(instance, m3ap_mce_data_g->assoc_id, buffer, len, 0);
  return 0;
}


int MCE_send_MBMS_SESSION_START_FAILURE(instance_t instance, m3ap_session_start_failure_t * m3ap_session_start_failure){
//  module_id_t enb_mod_idP;
//  module_id_t mce_mod_idP;
//
//  // This should be fixed
//  enb_mod_idP = (module_id_t)0;
//  mce_mod_idP  = (module_id_t)0;
//
//  M3AP_M3AP_PDU_t           pdu;
//  M3AP_SessionStartFailure_t    *out;
//  M3AP_SessionStartFailure_Ies_t *ie;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//
//  /* Create */
//  /* 0. Message Type */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M3AP_M3AP_PDU_PR_unsuccessfulOutcome;
//  //pdu.choice.unsuccessfulOutcome = (M3AP_UnsuccessfulOutcome_t *)calloc(1, sizeof(M3AP_UnsuccessfulOutcome_t));
//  pdu.choice.unsuccessfulOutcome.procedureCode = M3AP_ProcedureCode_id_sessionStart;
//  pdu.choice.unsuccessfulOutcome.criticality   = M3AP_Criticality_reject;
//  pdu.choice.unsuccessfulOutcome.value.present = M3AP_UnsuccessfulOutcome__value_PR_SessionStartFailure;
//  out = &pdu.choice.unsuccessfulOutcome.value.choice.SessionStartFailure;
//
//  /* mandatory */
//  /* c1. Transaction ID (integer value)*/
// // ie = (M3AP_M3SetupFailure_Ies_t *)calloc(1, sizeof(M3AP_M3SetupFailure_Ies_t));
// // ie->id                        = M3AP_ProtocolIE_ID_id_GlobalMCE_ID;
// // ie->criticality               = M3AP_Criticality_reject;
// // ie->value.present             = M3AP_M3SetupFailure_Ies__value_PR_GlobalMCE_ID;
// // ie->value.choice.GlobalMCE_ID = M3AP_get_next_transaction_identifier(enb_mod_idP, mce_mod_idP);
// // ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  /* c2. Cause */
//  ie = (M3AP_M3SetupFailure_Ies_t *)calloc(1, sizeof(M3AP_SessionStartFailure_Ies_t));
//  ie->id                        = M3AP_ProtocolIE_ID_id_Cause;
//  ie->criticality               = M3AP_Criticality_ignore;
//  ie->value.present             = M3AP_SessionStartFailure_Ies__value_PR_Cause;
//  ie->value.choice.Cause.present = M3AP_Cause_PR_radioNetwork;
//  ie->value.choice.Cause.choice.radioNetwork = M3AP_CauseRadioNetwork_unspecified;
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//
//     /* encode */
//  if (m3ap_encode_pdu(&pdu, &buffer, &len) < 0) {
//    LOG_E(M3AP, "Failed to encode M3 setup request\n");
//    return -1;
//  }
//
//  //mce_m3ap_itti_send_sctp_data_req(instance, m3ap_mce_data_from_enb->assoc_id, buffer, len, 0);
//  m3ap_MCE_itti_send_sctp_data_req(instance,assoc_id,buffer,len,0);
//

  return 0;
}

int MCE_handle_MBMS_SESSION_STOP_REQUEST(instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        M3AP_M3AP_PDU_t *pdu){

  //AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M3AP, "MCE_handle_MBMS_SESSION_STOP_REQUEST assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M3AP_MBMSSessionStopRequest_t              *container;
  //M3AP_MBMSSessionStopRequest_IEs_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.MBMSSessionStopRequest;

  /* M3 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_W(M3AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  //if(1){
	//MCE_send_MBMS_SESSION_STOP_RESPONSE(instance,assoc_id);
  //}else
	//MCE_send_MBMS_SESSION_STOP_FAILURE(instance,assoc_id);
    message_p = itti_alloc_new_message(TASK_M3AP_MCE, 0, M3AP_MBMS_SESSION_STOP_REQ);

//
//  /* M3 Setup Request == Non UE-related procedure -> stream 0 */
//  if (stream != 0) {
//    LOG_W(M3AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
//              assoc_id, stream);
//  }
//

    itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);



  return 0;
  
}
int MCE_send_MBMS_SESSION_STOP_RESPONSE(instance_t instance, m3ap_session_start_resp_t * m3ap_session_start_resp){
//	module_id_t mce_mod_idP;
//  module_id_t enb_mod_idP;
//
//  // This should be fixed
//  enb_mod_idP = (module_id_t)0;
//  mce_mod_idP  = (module_id_t)0;
//
  M3AP_M3AP_PDU_t           pdu;
  M3AP_MBMSSessionStopResponse_t    *out;
  M3AP_MBMSSessionStopResponse_IEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_successfulOutcome;
  //pdu.choice.successfulOutcome = (M3AP_SuccessfulOutcome_t *)calloc(1, sizeof(M3AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome.procedureCode = M3AP_ProcedureCode_id_mBMSsessionStop;
  pdu.choice.successfulOutcome.criticality   = M3AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M3AP_SuccessfulOutcome__value_PR_MBMSSessionStopResponse;
  out = &pdu.choice.successfulOutcome.value.choice.MBMSSessionStopResponse;
  

  /* mandatory */
  /* c1. MCE_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionStopResponse_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStopResponse_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStopResponse_IEs__value_PR_MCE_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

 /* mandatory */
  /* c1. MCE_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionStopResponse_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStopResponse_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStopResponse_IEs__value_PR_MCE_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (m3ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M3AP, "Failed to encode M3 SessionStop Response\n");
    return -1;
  }


  LOG_D(M3AP,"pdu.present %d\n",pdu.present);
 // MSC_LOG_TX_MESSAGE(
 // MSC_M3AP_MCE,
 // MSC_M3AP_MCE,
 // (const char *)buffer,
 // len,
 // MSC_AS_TIME_FMT" M3_SETUP_REQUEST initiatingMessage MCEname %s",
 // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 // m3ap_MCE_data_p->MCEname);

  m3ap_MCE_itti_send_sctp_data_req(instance, m3ap_mce_data_g->assoc_id, buffer, len, 0);
  return 0;
}

/*
 * Session Update
 */
int MCE_handle_MBMS_SESSION_UPDATE_REQUEST(instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        M3AP_M3AP_PDU_t *pdu)
{
  //AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M3AP, "MCE_handle_MBMS_SESSION_UPDATE_REQUEST assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M3AP_MBMSSessionUpdateRequest_t              *container;
  //M3AP_MBMSSessionUpdateRequest_IEs_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.MBMSSessionUpdateRequest;

  /* M3 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M3AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  //if(1){
	//MCE_send_MBMS_SESSION_STOP_RESPONSE(instance,assoc_id);
  //}else
	//MCE_send_MBMS_SESSION_STOP_FAILURE(instance,assoc_id);
    message_p = itti_alloc_new_message(TASK_M3AP_MCE, 0, M3AP_MBMS_SESSION_UPDATE_REQ);

//
//  /* M3 Setup Request == Non UE-related procedure -> stream 0 */
//  if (stream != 0) {
//    LOG_W(M3AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
//              assoc_id, stream);
//  }
//

    itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);




	return 0;
}

int MCE_send_MBMS_SESSION_UPDATE_RESPONSE(instance_t instance, m3ap_mbms_session_update_resp_t * m3ap_mbms_session_update_resp){
  M3AP_M3AP_PDU_t           pdu;
  M3AP_MBMSSessionUpdateResponse_t    *out;
  M3AP_MBMSSessionUpdateResponse_IEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_successfulOutcome;
  //pdu.choice.successfulOutcome = (M3AP_SuccessfulOutcome_t *)calloc(1, sizeof(M3AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome.procedureCode = M3AP_ProcedureCode_id_mBMSsessionUpdate;
  pdu.choice.successfulOutcome.criticality   = M3AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M3AP_SuccessfulOutcome__value_PR_MBMSSessionUpdateResponse;
  out = &pdu.choice.successfulOutcome.value.choice.MBMSSessionUpdateResponse;
  

  /* mandatory */
  /* c1. MCE_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionUpdateResponse_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateResponse_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateResponse_IEs__value_PR_MCE_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

 /* mandatory */
  /* c1. MCE_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionUpdateResponse_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateResponse_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateResponse_IEs__value_PR_MCE_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (m3ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M3AP, "Failed to encode M3 SessionUpdate Response\n");
    return -1;
  }


  LOG_D(M3AP,"pdu.present %d\n",pdu.present);
 // MSC_LOG_TX_MESSAGE(
 // MSC_M3AP_MCE,
 // MSC_M3AP_MCE,
 // (const char *)buffer,
 // len,
 // MSC_AS_TIME_FMT" M3_SETUP_REQUEST initiatingMessage MCEname %s",
 // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 // m3ap_MCE_data_p->MCEname);

  m3ap_MCE_itti_send_sctp_data_req(instance, m3ap_mce_data_g->assoc_id, buffer, len, 0);
  return 0;
}
int MCE_send_MBMS_SESSION_UPDATE_FAILURE(instance_t instance, m3ap_mbms_session_update_failure_t * m3ap_mbms_session_update_failure){
	return 0;
}

/*
    M3 Setup
*/

//uint8_t m3ap_m3setup_message[] = {0x00,0x07,0x00,0x23,0x00,0x00,0x03,0x00,0x12,0x00,0x07,0x40,0x55,0xf5,0x01,0x00,0x25,0x00,0x00,0x13,0x40,0x0a,0x03,0x80,0x4d,0x33,0x2d,0x65,0x4e,0x42,0x33,0x37,0x00,0x14,0x00,0x03,0x01,0x00,0x01};
uint8_t m3ap_m3setup_message[] = {
  0x00, 0x07, 0x00, 0x23, 0x00, 0x00, 0x03, 0x00,
  0x12, 0x00, 0x07, 0x40, 0x55, 0xf5, 0x01, 0x00,
  0x25, 0x00, 0x00, 0x13, 0x40, 0x0a, 0x03, 0x80,
  0x4d, 0x33, 0x2d, 0x65, 0x4e, 0x42, 0x33, 0x37,
  0x00, 0x14, 0x00, 0x03, 0x01, 0x00, 0x01
};


//uint8_t m3ap_start_message[] = {
//  0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x07, 0x00,
//  0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x02, 0x00,
//  0x07, 0x00, 0x55, 0xf5, 0x01, 0x00, 0x00, 0x01,
//  0x00, 0x04, 0x00, 0x0d, 0x60, 0x01, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x40, 0x01,
//  0x7a, 0x00, 0x05, 0x00, 0x03, 0x07, 0x08, 0x00,
//  0x00, 0x06, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00,
//  0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x07, 0x00,
//  0x0e, 0x00, 0xe8, 0x0a, 0x0a, 0x0a, 0x00, 0x0a,
//  0xc8, 0x0a, 0xfe, 0x00, 0x00, 0x00, 0x01
//};
//
// SETUP REQUEST
int MCE_send_M3_SETUP_REQUEST(m3ap_MCE_instance_t *instance_p, m3ap_MCE_data_t *m3ap_MCE_data_p) {
//  module_id_t enb_mod_idP=0;
//  module_id_t du_mod_idP=0;
//
  M3AP_M3AP_PDU_t          pdu;
  M3AP_M3SetupRequest_t    *out;
  M3AP_M3SetupRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;
  //int       j = 0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M3AP_InitiatingMessage_t *)calloc(1, sizeof(M3AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M3AP_ProcedureCode_id_m3Setup;
  pdu.choice.initiatingMessage.criticality   = M3AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M3AP_InitiatingMessage__value_PR_M3SetupRequest;
  out = &pdu.choice.initiatingMessage.value.choice.M3SetupRequest;

  /* mandatory */
  /* c1. GlobalMCE_ID (integer value) */
  ie = (M3AP_M3SetupRequestIEs_t *)calloc(1, sizeof(M3AP_M3SetupRequestIEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_Global_MCE_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_M3SetupRequestIEs__value_PR_Global_MCE_ID;
  //ie->value.choice.GlobalMCE_ID.MCE_ID = 1;//M3AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);
  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                  &ie->value.choice.Global_MCE_ID.pLMN_Identity);
  //ie->value.choice.Global_MCE_ID.MCE_ID.present = M3AP_MCE_ID_PR_macro_MCE_ID;
 OCTET_STRING_fromBuf(&ie->value.choice.Global_MCE_ID.mCE_ID,"02",2);

 // MACRO_ENB_ID_TO_BIT_STRING(instance_p->MCE_id,
  //                         &ie->value.choice.Global_MCE_ID.mCE_ID);
  //M3AP_INFO("%d -> %02x%02x%02x\n", instance_p->MCE_id,
   //       ie->value.choice.Global_MCE_ID.mCE_ID.buf[0],
    //      ie->value.choice.Global_MCE_ID.mCE_ID.buf[1],
     //     ie->value.choice.Global_MCE_ID.mCE_ID.buf[2]);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  ///* mandatory */
//  ///* c2. GNB_MCE_ID (integrer value) */
//  //ie = (M3AP_M3SetupRequest_Ies_t *)calloc(1, sizeof(M3AP_M3SetupRequest_Ies_t));
//  //ie->id                        = M3AP_ProtocolIE_ID_id_gNB_MCE_ID;
//  //ie->criticality               = M3AP_Criticality_reject;
//  //ie->value.present             = M3AP_M3SetupRequestIEs__value_PR_GNB_MCE_ID;
//  //asn_int642INTEGER(&ie->value.choice.GNB_MCE_ID, f1ap_du_data->gNB_MCE_id);
//  //ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//     /* optional */
  /* c3. MCEname */
  //m3ap_MCE_data_p->MCE_name="kk";
  if (m3ap_MCE_data_p->MCE_name != NULL) {
    ie = (M3AP_M3SetupRequestIEs_t *)calloc(1, sizeof(M3AP_M3SetupRequestIEs_t));
    ie->id                        = M3AP_ProtocolIE_ID_id_MCEname;
    ie->criticality               = M3AP_Criticality_ignore;
    ie->value.present             = M3AP_M3SetupRequestIEs__value_PR_MCEname;
    OCTET_STRING_fromBuf(&ie->value.choice.MCEname, m3ap_MCE_data_p->MCE_name,
                         strlen(m3ap_MCE_data_p->MCE_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */ //but removed from asn definition (optional) since it crashes when decoding ???????????
//	M3SetupRequestIEs M3AP-PROTOCOL-IES ::= {
//		{ ID id-Global-MCE-ID                   CRITICALITY reject      TYPE Global-MCE-ID                                      PRESENCE mandatory}|
//		{ ID id-MCEname                                 CRITICALITY ignore      TYPE MCEname                                            PRESENCE optional}|
//		{ ID id-MBMSServiceAreaList             CRITICALITY reject      TYPE MBMSServiceAreaListItem            PRESENCE mandatory}, --> optional ??????????
//		...
//	}


  /* M3AP_MBMSServiceAreaListItem_t */
  ie = (M3AP_M3SetupRequestIEs_t *)calloc(1, sizeof(M3AP_M3SetupRequestIEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MBMSServiceAreaList;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_M3SetupRequestIEs__value_PR_MBMSServiceAreaListItem;

  //M3AP_MBMSServiceAreaListItem_t * m3ap_mbmsservicearealistitem = (M3AP_MBMSServiceAreaListItem_t*)calloc(1,sizeof(M3AP_MBMSServiceAreaListItem_t)); 

  M3AP_MBMSServiceArea1_t * m3ap_mbmsservicearea1 = (M3AP_MBMSServiceArea1_t*)calloc(1,sizeof(M3AP_MBMSServiceArea1_t));
  
  OCTET_STRING_fromBuf(m3ap_mbmsservicearea1,"02",2);

  //ASN_SEQUENCE_ADD(&m3ap_mbmsservicearealistitem->list,m3ap_mbmsservicearea1);

  //ASN_SEQUENCE_ADD(&ie->value.choice.MBMSServiceAreaListItem,m3ap_mbmsservicearealistitem);
  //ASN_SEQUENCE_ADD(&ie->value.choice.MBMSServiceAreaListItem.list,m3ap_mbmsservicearealistitem);
  ASN_SEQUENCE_ADD(&ie->value.choice.MBMSServiceAreaListItem.list,m3ap_mbmsservicearea1);
  
  //ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

//
//  /* mandatory */
//  /* c4. serverd cells list */
//  ie = (M3AP_M3SetupRequest_Ies_t *)calloc(1, sizeof(M3AP_M3SetupRequest_Ies_t));
//  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_Configuration_data_List;
//  ie->criticality               = M3AP_Criticality_reject;
//  ie->value.present             = M3AP_M3SetupRequest_Ies__value_PR_MCE_MBMS_Configuration_data_List;
//
//  int num_mbms_available = 1;//m3ap_du_data->num_mbms_available;
//  LOG_D(M3AP, "num_mbms_available = %d \n", num_mbms_available);
//
// for (i=0;
//       i<num_mbms_available;
//       i++) {
//        /* mandatory */
//        /* 4.1 serverd cells item */
//
//        M3AP_MCE_MBMS_Configuration_data_ItemIEs_t *mbms_configuration_data_list_item_ies;
//        mbms_configuration_data_list_item_ies = (M3AP_MCE_MBMS_Configuration_data_ItemIEs_t *)calloc(1, sizeof(M3AP_MCE_MBMS_Configuration_data_ItemIEs_t));
//        mbms_configuration_data_list_item_ies->id = M3AP_ProtocolIE_ID_id_MCE_MBMS_Configuration_data_Item;
//        mbms_configuration_data_list_item_ies->criticality = M3AP_Criticality_reject;
//        mbms_configuration_data_list_item_ies->value.present = M3AP_MCE_MBMS_Configuration_data_ItemIEs__value_PR_MCE_MBMS_Configuration_data_Item;
//
//        M3AP_MCE_MBMS_Configuration_data_Item_t *mbms_configuration_data_item;
//        mbms_configuration_data_item = &mbms_configuration_data_list_item_ies->value.choice.MCE_MBMS_Configuration_data_Item;
//        {
//		/* M3AP_ECGI_t eCGI */
//                MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//                  &mbms_configuration_data_item->eCGI.pLMN_Identity);
//                MACRO_MCE_ID_TO_CELL_IDENTITY(instance_p->MCE_id,0,
//                                   &mbms_configuration_data_item->eCGI.eUTRANcellIdentifier);
//		/* M3AP_MBSFN_SynchronisationArea_ID_t mbsfnSynchronisationArea */ 
//		mbms_configuration_data_item->mbsfnSynchronisationArea=10000; //? long
//		/* M3AP_MBMS_Service_Area_ID_List_t mbmsServiceAreaList */
//                M3AP_MBMS_Service_Area_t * mbms_service_area,*mbms_service_area2;
//                mbms_service_area = (M3AP_MBMS_Service_Area_t*)calloc(1,sizeof(M3AP_MBMS_Service_Area_t));
//                mbms_service_area2 = (M3AP_MBMS_Service_Area_t*)calloc(1,sizeof(M3AP_MBMS_Service_Area_t));
//		//memset(mbms_service_area,0,sizeof(OCTET_STRING_t));
//		OCTET_STRING_fromBuf(mbms_service_area,"01",2);
//                ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area);
//		OCTET_STRING_fromBuf(mbms_service_area2,"02",2);
//                ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area2);
//
//
//        }
//
//
//        //M3AP_MCE_MBMS_Configuration_data_Item_t mbms_configuration_data_item;
//        //memset((void *)&mbms_configuration_data_item, 0, sizeof(M3AP_MCE_MBMS_Configuration_data_Item_t));
//
//        //M3AP_ECGI_t      eCGI;
//                //M3AP_PLMN_Identity_t     pLMN_Identity;
//                //M3AP_EUTRANCellIdentifier_t      eUTRANcellIdentifier
//        //M3AP_MBSFN_SynchronisationArea_ID_t      mbsfnSynchronisationArea;
//        //M3AP_MBMS_Service_Area_ID_List_t         mbmsServiceAreaList;
//
//
//        ASN_SEQUENCE_ADD(&ie->value.choice.MCE_MBMS_Configuration_data_List.list,mbms_configuration_data_list_item_ies);
//
// }
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//  
// LOG_W(M3AP,"m3ap_MCE_data_p->assoc_id %d\n",m3ap_MCE_data_p->assoc_id);
//  /* encode */

//    xer_fprint(stdout, &asn_DEF_M3AP_M3AP_PDU, &pdu);
//    xer_fprint(stdout, &asn_DEF_M3AP_M3SetupRequest, &pdu.choice.initiatingMessage.value.choice.M3SetupRequest);
//    xer_fprint(stdout, &asn_DEF_M3AP_M3SetupRequest, out);

 if (m3ap_encode_pdu(&pdu, &buffer, &len) < 0) {
   LOG_E(M3AP, "Failed to encode M3 setup request\n");
   return -1;
 }

 /* buffer = &m3ap_start_message[0];
  len=8*9+7; 

  buffer = &m3ap_m3setup_message[0];
  len=8*4+7; */
//
//
//  LOG_W(M3AP,"pdu.present %d\n",pdu.present);
// // MSC_LOG_TX_MESSAGE(
// // MSC_M3AP_MCE,
// // MSC_M3AP_MCE,
// // (const char *)buffer,
// // len,
// // MSC_AS_TIME_FMT" M3_SETUP_REQUEST initiatingMessage MCEname %s",
// // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
// // m3ap_MCE_data_p->MCEname);
//
//
////  buffer = &bytes[0];
////  len = 40;
////
////  for(int i=0; i < len; i++ )
////	printf("%02X",buffer[i]);
////  printf("\n");
////

// M3AP_M3AP_PDU_t pdu2;
//
//  memset(&pdu2, 0, sizeof(pdu2));
//
//  for( i=0; i < len; i++)
//        printf("%02X",buffer[i]);
//  printf("\n");
//
//  if (m3ap_decode_pdu(&pdu2, buffer, len) < 0) {
//    LOG_E(M3AP, "Failed to decode PDU\n");
//    //return -1;
//  }
//

  //printf("m3ap_MCE_data_p->assoc_id %d\n",m3ap_MCE_data_p->assoc_id);
  m3ap_MCE_itti_send_sctp_data_req(instance_p->instance, m3ap_MCE_data_p->assoc_id, buffer, len, 0);

  return 0;
}



int MCE_handle_M3_SETUP_RESPONSE(instance_t instance,
				uint32_t               assoc_id,
				uint32_t               stream,
				M3AP_M3AP_PDU_t       *pdu)
{

   LOG_D(M3AP, "MCE_handle_M3_SETUP_RESPONSE\n");

   AssertFatal(pdu->present == M3AP_M3AP_PDU_PR_successfulOutcome,
	       "pdu->present != M3AP_M3AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M3AP_ProcedureCode_id_m3Setup,
	       "pdu->choice.successfulOutcome.procedureCode != M3AP_ProcedureCode_id_M3Setup\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M3AP_Criticality_reject,
	       "pdu->choice.successfulOutcome.criticality != M3AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M3AP_SuccessfulOutcome__value_PR_M3SetupResponse,
	       "pdu->choice.successfulOutcome.value.present != M3AP_SuccessfulOutcome__value_PR_M3SetupResponse\n");

   //M3AP_M3SetupResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.M3SetupResponse;


   //M3AP_M3SetupResponseIEs_t *ie;
   //int GlobalMCE_ID = -1;
   //int num_cells_to_activate = 0;
   //M3AP_Cells_to_be_Activated_List_Item_t *cell;

   MessageDef *msg_p = itti_alloc_new_message (TASK_M3AP_MCE, 0, M3AP_SETUP_RESP);

  // LOG_D(M3AP, "M3AP: M3Setup-Resp: protocolIEs.list.count %d\n",
  //       in->protocolIEs.list.count);
  // for (int i=0;i < in->protocolIEs.list.count; i++) {
  //   ie = in->protocolIEs.list.array[i];
  // }
   //AssertFatal(GlobalMCE_ID!=-1,"GlobalMCE_ID was not sent\n");
   //AssertFatal(num_cells_to_activate>0,"No cells activated\n");
   //M3AP_SETUP_RESP (msg_p).num_cells_to_activate = num_cells_to_activate;

   //for (int i=0;i<num_cells_to_activate;i++)  
   //  AssertFatal(M3AP_SETUP_RESP (msg_p).num_SI[i] > 0, "System Information %d is missing",i);

   //MSC_LOG_RX_MESSAGE(
   // MSC_M3AP_MCE,
   // MSC_M3AP_CU,
   // 0,
   // 0,
   // MSC_AS_TIME_FMT" MCE_handle_M3_SETUP_RESPONSE successfulOutcome assoc_id %d",
   // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
   // assoc_id);
 
   //LOG_D(M3AP, "Sending M3AP_SETUP_RESP ITTI message to MCE_APP with assoc_id (%d->%d)\n",
   //      assoc_id,MCE_MOMCELE_ID_TO_INSTANCE(assoc_id));
   itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);

   return 0;
}

// SETUP FAILURE
int MCE_handle_M3_SETUP_FAILURE(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M3AP_M3AP_PDU_t *pdu) {
  LOG_E(M3AP, "MCE_handle_M3_SETUP_FAILURE\n");
  return 0;
}





