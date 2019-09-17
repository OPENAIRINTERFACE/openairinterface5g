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

/*! \file m2ap_MCE_interface_management.c
 * \brief m2ap interface management for MCE
 * \author Javier Morgade
 * \date 2019
 * \version 0.1
 * \company Vicomtech
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#include "m2ap_common.h"
#include "m2ap_encoder.h"
#include "m2ap_decoder.h"
#include "m2ap_itti_messaging.h"
#include "m2ap_MCE_interface_management.h"

#include "conversions.h"

extern m2ap_setup_req_t *m2ap_mce_data_from_enb;

/*
 * MBMS Session start
 */
int MCE_send_MBMS_SESSION_START_REQUEST(instance_t instance/*, uint32_t assoc_id*/,m2ap_session_start_req_t* m2ap_session_start_req){

  //AssertFatal(1==0,"Not implemented yet\n");
    
  //module_id_t enb_mod_idP=0;
  //module_id_t du_mod_idP=0;

  M2AP_M2AP_PDU_t          pdu; 
  M2AP_SessionStartRequest_t       *out;
  M2AP_SessionStartRequest_Ies_t   *ie;

  uint8_t *buffer;
  uint32_t len;
  //int	   i=0; 
  //int 	   j=0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M2AP_InitiatingMessage_t *)calloc(1, sizeof(M2AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_sessionStart;
  pdu.choice.initiatingMessage.criticality   = M2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_SessionStartRequest;
  out = &pdu.choice.initiatingMessage.value.choice.SessionStartRequest;  

  /* mandatory */
  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStartRequest_Ies__value_PR_MCE_MBMS_M2AP_ID;
  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. TMGI (integrer value) */
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_TMGI;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStartRequest_Ies__value_PR_TMGI;
  MCC_MNC_TO_PLMNID(0,0,3,&ie->value.choice.TMGI.pLMNidentity);/*instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,*/
  uint8_t TMGI[5] = {4,3,2,1,0};
  OCTET_STRING_fromBuf(&ie->value.choice.TMGI.serviceID,(const char*)&TMGI[2],3);
 
                       //&ie->choice.TMGI.pLMN_Identity);
  //INT16_TO_OCTET_STRING(0,&ie->choice.TMGI.serviceId);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. MBMS_Session_ID (integrer value) */ //OCTET_STRING
  if(0){
	  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
	  ie->id                        = M2AP_ProtocolIE_ID_id_MBMS_Session_ID;
	  ie->criticality               = M2AP_Criticality_reject;
	  ie->value.present             = M2AP_SessionStartRequest_Ies__value_PR_MBMS_Session_ID;
  	  //INT16_TO_OCTET_STRING(0,&ie->choice.MBMS_Session_ID);
	  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c5. MBMS_Service_Area (integrer value) */ //OCTET_STRING
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MBMS_Service_Area; 
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStartRequest_Ies__value_PR_MBMS_Service_Area;
  //OCTET_STRING_fromBuf(&ie->value.choice.MBMS_Service_Area, m2ap_setup_resp->MCEname,
                         //strlen(m2ap_setup_resp->MCEname));
  //INT16_TO_OCTET_STRING(0,&ie->choice.TMGI.serviceId);
  ie->value.choice.MBMS_Service_Area.buf = calloc(3,sizeof(uint8_t));
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c6. TNL_Information (integrer value) */
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_TNL_Information; 
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStartRequest_Ies__value_PR_TNL_Information;
  //ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(4,sizeof(uint8_t));
  //ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(4,sizeof(uint8_t));
  //TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234,&ie->value.choice.TNL_Information.iPMCAddress);
  //TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234,&ie->value.choice.TNL_Information.iPSourceAddress);
  
  OCTET_STRING_fromBuf(&ie->value.choice.TNL_Information.gTP_TEID, "1204",strlen("1234"));
  OCTET_STRING_fromBuf(&ie->value.choice.TNL_Information.iPMCAddress, "1204",strlen("1234"));
  OCTET_STRING_fromBuf(&ie->value.choice.TNL_Information.iPSourceAddress, "1204",strlen("1234"));
  
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if(0){
	 // /* optional */
	 // /* c7. TNL_Information_1 (integrer value) */
	 // ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
	 // ie->id                        = M2AP_ProtocolIE_ID_id_TNL_Information_1; 
	 // ie->criticality               = M2AP_Criticality_reject;
	 // ie->value.present             = M2AP_SessionStartRequest_Ies__value_PR_TNL_Information_1;
	 // //asn_int642INTEGER(&ie->value.choice.MBMS_Session_ID, f1ap_du_data->MBMS_Session_ID); //?
	 // ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }
   

  /* encode */
  if (m2ap_encode_pdu(&pdu,&buffer,&len) < 0){
	return -1;
  }

  //MCE_m2ap_itti_send_sctp_data_req(instance, m2ap_mce_data_from_mce->assoid,buffer,len,0); 
  m2ap_MCE_itti_send_sctp_data_req(instance,m2ap_mce_data_from_enb->assoc_id,buffer,len,0);
  return 0;

}

int MCE_handle_MBMS_SESSION_START_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu){
  //AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M2AP, "MCE_handle_MBMS_SESSION_START_RESPONSE\n");

   AssertFatal(pdu->present == M2AP_M2AP_PDU_PR_successfulOutcome,
	       "pdu->present != M2AP_M2AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M2AP_ProcedureCode_id_sessionStart,
	       "pdu->choice.successfulOutcome->procedureCode != M2AP_ProcedureCode_id_sessionStart\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M2AP_Criticality_reject,
	       "pdu->choice.successfulOutcome->criticality != M2AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M2AP_SuccessfulOutcome__value_PR_SessionStartResponse,
	       "pdu->choice.successfulOutcome.value.present != M2AP_SuccessfulOutcome__value_PR_SessionStartResponse\n");


  M2AP_SessionStartResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.SessionStartResponse;
  M2AP_SessionStartResponse_Ies_t  *ie;
  int MCE_MBMS_M2AP_ID=-1;
  int ENB_MBMS_M2AP_ID=-1;


  MessageDef *msg_g = itti_alloc_new_message(TASK_M2AP_MCE,M2AP_MBMS_SESSION_START_RESP); //TODO

  LOG_D(M2AP, "M2AP: SessionStart-Resp: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
  for (int i=0;i < in->protocolIEs.list.count; i++) {
     ie = in->protocolIEs.list.array[i];
     switch (ie->id) {
     case M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID:
       AssertFatal(ie->criticality == M2AP_Criticality_reject,
                   "ie->criticality != M2AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M2AP_SessionStartResponse_Ies__value_PR_MCE_MBMS_M2AP_ID,
                   "ie->value.present != M2AP_sessionStartResponse_IEs__value_PR_MCE_MBMS_M2AP_ID\n");
       MCE_MBMS_M2AP_ID=ie->value.choice.MCE_MBMS_M2AP_ID;
       LOG_D(M2AP, "M2AP: SessionStart-Resp: MCE_MBMS_M2AP_ID %d\n",
             MCE_MBMS_M2AP_ID);
       break;
      case M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID:
       AssertFatal(ie->criticality == M2AP_Criticality_reject,
                   "ie->criticality != M2AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M2AP_SessionStartResponse_Ies__value_PR_ENB_MBMS_M2AP_ID,
                   "ie->value.present != M2AP_sessionStartResponse_Ies__value_PR_ENB_MBMS_M2AP_ID\n");
       ENB_MBMS_M2AP_ID=ie->value.choice.ENB_MBMS_M2AP_ID;
       LOG_D(M2AP, "M2AP: SessionStart-Resp: ENB_MBMS_M2AP_ID %d\n",
             ENB_MBMS_M2AP_ID);
       break;
     }
  }

  AssertFatal(MCE_MBMS_M2AP_ID!=-1,"MCE_MBMS_M2AP_ID was not sent\n");
  AssertFatal(ENB_MBMS_M2AP_ID!=-1,"ENB_MBMS_M2AP_ID was not sent\n");
  //M2AP_SESSION_START_RESP(msg_p).
//  MSC_LOG_RX_MESSAGE(
//    MSC_M2AP_MCE,
//    MSC_M2AP_ENB,
	//return 0;
//    0,
//    0,
//    MSC_AS_TIME_FMT" MCE_handle_M2_SESSION_START_RESPONSE successfulOutcome assoc_id %d",
//    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
//    assoc_id);
//
   LOG_D(M2AP, "Sending M2AP_SESSION_START_RESP ITTI message to ENB_APP with assoc_id (%d->%d)\n",
         assoc_id,ENB_MODULE_ID_TO_INSTANCE(assoc_id));
   itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_g);
   return 0;

}
int MCE_handle_MBMS_SESSION_START_FAILURE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
 * MBMS Session stop
 */
int MCE_send_MBMS_SESSION_STOP_REQUEST(instance_t instance, m2ap_session_stop_req_t* m2ap_session_stop_req){

    
 // module_id_t enb_mod_idP=0;
  //module_id_t du_mod_idP=0;

  M2AP_M2AP_PDU_t          pdu; 
  M2AP_SessionStopRequest_t        *out;
  M2AP_SessionStopRequest_Ies_t    *ie;

  uint8_t *buffer;
  uint32_t len;
  //int	   i=0; 
  //int 	   j=0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M2AP_InitiatingMessage_t *)calloc(1, sizeof(M2AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_sessionStop;
  pdu.choice.initiatingMessage.criticality   = M2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_SessionStopRequest;
  out = &pdu.choice.initiatingMessage.value.choice.SessionStopRequest; 

  /* mandatory */
  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStopRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStopRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStopRequest_Ies__value_PR_MCE_MBMS_M2AP_ID;
  ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. ENB_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStopRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStopRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStopRequest_Ies__value_PR_ENB_MBMS_M2AP_ID;
  ie->value.choice.ENB_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* encode */
  if (m2ap_encode_pdu(&pdu,&buffer,&len) < 0){
	return -1;
  }

  //MCE_m2ap_itti_send_sctp_data_req(instance, m2ap_mce_data_from_mce->assoid,buffer,len,0); 
  m2ap_MCE_itti_send_sctp_data_req(instance, m2ap_mce_data_from_enb->assoc_id,buffer,len,0);
  return 0;


                        
}

int MCE_handle_MBMS_SESSION_STOP_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu){
  //AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M2AP, "MCE_handle_MBMS_SESSION_STOP_RESPONSE\n");

   AssertFatal(pdu->present == M2AP_M2AP_PDU_PR_successfulOutcome,
	       "pdu->present != M2AP_M2AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M2AP_ProcedureCode_id_sessionStop,
	       "pdu->choice.successfulOutcome->procedureCode != M2AP_ProcedureCode_id_sessionStop\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M2AP_Criticality_reject,
	       "pdu->choice.successfulOutcome->criticality != M2AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M2AP_SuccessfulOutcome__value_PR_SessionStopResponse,
	       "pdu->choice.successfulOutcome->value.present != M2AP_SuccessfulOutcome__value_PR_SessionStopResponse\n");


  M2AP_SessionStopResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.SessionStopResponse;
  M2AP_SessionStopResponse_Ies_t  *ie;
  int MCE_MBMS_M2AP_ID=-1;
  int ENB_MBMS_M2AP_ID=-1;


  MessageDef *msg_g = itti_alloc_new_message(TASK_M2AP_MCE,M2AP_MBMS_SESSION_STOP_RESP); //TODO

  LOG_D(M2AP, "M2AP: SessionStop-Resp: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
  for (int i=0;i < in->protocolIEs.list.count; i++) {
     ie = in->protocolIEs.list.array[i];
     switch (ie->id) {
     case M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID:
       AssertFatal(ie->criticality == M2AP_Criticality_reject,
                   "ie->criticality != M2AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M2AP_SessionStopResponse_Ies__value_PR_MCE_MBMS_M2AP_ID,
                   "ie->value.present != M2AP_SessionStopResponse_Ies__value_PR_MCE_MBMS_M2AP_ID\n");
       MCE_MBMS_M2AP_ID=ie->value.choice.MCE_MBMS_M2AP_ID;
       LOG_D(M2AP, "M2AP: SessionStop-Resp: MCE_MBMS_M2AP_ID %d\n",
             MCE_MBMS_M2AP_ID);
       break;
      case M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID:
       AssertFatal(ie->criticality == M2AP_Criticality_reject,
                   "ie->criticality != M2AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M2AP_SessionStopResponse_Ies__value_PR_ENB_MBMS_M2AP_ID,
                   "ie->value.present != M2AP_SessionStopResponse_Ies__value_PR_ENB_MBMS_M2AP_ID\n");
       ENB_MBMS_M2AP_ID=ie->value.choice.ENB_MBMS_M2AP_ID;
       LOG_D(M2AP, "M2AP: SessionStop-Resp: ENB_MBMS_M2AP_ID %d\n",
             ENB_MBMS_M2AP_ID);
       break;
     }
  }

  AssertFatal(MCE_MBMS_M2AP_ID!=-1,"MCE_MBMS_M2AP_ID was not sent\n");
  AssertFatal(ENB_MBMS_M2AP_ID!=-1,"ENB_MBMS_M2AP_ID was not sent\n");
 // M2AP_SESSION_STOP_RESP(msg_p).
 // MSC_LOG_RX_MESSAGE(
 //   MSC_M2AP_MCE,
 //   MSC_M2AP_ENB,
 //   0,
 //   0,
 //   MSC_AS_TIME_FMT" MCE_handle_M2_SESSION_STOP_RESPONSE successfulOutcome assoc_id %d",
 //   0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 //   assoc_id);

 //  LOG_D(M2AP, "Sending M2AP_SESSION_START_RESP ITTI message to ENB_APP with assoc_id (%d->%d)\n",
 //        assoc_id,ENB_MODULE_ID_TO_INSTANCE(assoc_id));
   itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_g);
 //                      
	return 0;
}



int MCE_handle_MBMS_SESSION_STOP_FAILURE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");
                      
}
 typedef struct M2AP_MBSFN_Area_Configuration{
	M2AP_PMCH_Configuration_List_t   PMCH_Configuration_List;
        M2AP_MBSFN_Subframe_ConfigurationList_t  MBSFN_Subframe_ConfigurationList;
        M2AP_Common_Subframe_Allocation_Period_t         Common_Subframe_Allocation_Period;
        M2AP_MBSFN_Area_ID_t     MBSFN_Area_ID;
        M2AP_MBMS_Suspension_Notification_List_t         MBMS_Suspension_Notification_List;
  }M2AP_MBSFN_Area_Configuration_t;


/*
 * MBMS Scheduling Information
 */
 uint8_t m2ap_message[]    = {0x00, 0x02, 0x00, 0x3a, 0x00, 0x00, 0x02, 0x00, 0x19, 0x00, 0x01, 0x00, 0x00,
                            0x0a, 0x00, 0x2e, 0x00, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x12, 0x10, 0x00, 0x0c,
                            0x00, 0x0d, 0x00, 0x00, 0x3f, 0x13, 0x00, 0x00, 0x00, 0xf1, 0x10, 0x00, 0x00,
                            0x01, 0x08, 0x00, 0x16, 0x00, 0x07, 0x00, 0x00, 0x17, 0x00, 0x02, 0x00, 0x40,
                            0x00, 0x18, 0x00, 0x01, 0x80, 0x00, 0x1d, 0x00, 0x01, 0x01};


int MCE_send_MBMS_SCHEDULING_INFORMATION(instance_t instance, /*uint32_t assoc_id,*/m2ap_mbms_scheduling_information_t * m2ap_mbms_scheduling_information){
   
  //module_id_t enb_mod_idP=0;
  //module_id_t du_mod_idP=0;

  M2AP_M2AP_PDU_t          pdu; 
  M2AP_MbmsSchedulingInformation_t       *out;
  M2AP_MbmsSchedulingInformation_Ies_t   *ie;

  uint8_t *buffer/*,*buffer2*/;
  uint32_t len/*,len2*/;
  int	   i=0; 
  int 	   j=0;
  int 	   k=0;
  //int 	   l=0;
  //int 	   l=0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M2AP_InitiatingMessage_t *)calloc(1, sizeof(M2AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_mbmsSchedulingInformation;
  pdu.choice.initiatingMessage.criticality   = M2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_MbmsSchedulingInformation;
  out = &pdu.choice.initiatingMessage.value.choice.MbmsSchedulingInformation; 

  /* mandatory */
  /* c1. MCCH_Update_Time */ //long
  ie=(M2AP_MbmsSchedulingInformation_Ies_t*)calloc(1,sizeof(M2AP_MbmsSchedulingInformation_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCCH_Update_Time;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_MbmsSchedulingInformation_Ies__value_PR_MCCH_Update_Time;
  //ie->value.choice.MCCH_Update_Time =  ; 
  ie->value.choice.MCCH_Update_Time = m2ap_mbms_scheduling_information->mcch_update_time;

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);  

  /* mandatory */
  /* c2. MBSFN_Area_Configuration_List */ //long
  ie=(M2AP_MbmsSchedulingInformation_Ies_t*)calloc(1,sizeof(M2AP_MbmsSchedulingInformation_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MBSFN_Area_Configuration_List;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_MbmsSchedulingInformation_Ies__value_PR_MBSFN_Area_Configuration_List;

  for(i=0; i < m2ap_mbms_scheduling_information->num_mbms_area_config_list; i++){ 
      M2AP_MBSFN_Area_Configuration_List_t * m2ap_mbsfn_area_configuration_list = (M2AP_MBSFN_Area_Configuration_List_t*)calloc(1,sizeof(M2AP_MBSFN_Area_Configuration_List_t));
      /*M2AP_MBSFN_Area_Configuration_Item_t*/
      M2AP_MBSFN_Area_Configuration_Item_t *mbsfn_area_configuration_item_ie;
      mbsfn_area_configuration_item_ie =(M2AP_MBSFN_Area_Configuration_Item_t*)calloc(1,sizeof(M2AP_MBSFN_Area_Configuration_Item_t)); 
      mbsfn_area_configuration_item_ie->id = M2AP_ProtocolIE_ID_id_PMCH_Configuration_List;
      mbsfn_area_configuration_item_ie->criticality = M2AP_Criticality_reject;
      mbsfn_area_configuration_item_ie->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_PMCH_Configuration_List;

      for(j=0; j < m2ap_mbms_scheduling_information->mbms_area_config_list[i].num_pmch_config_list; j++){
      
      	M2AP_PMCH_Configuration_ItemIEs_t * pmch_configuration_item_ies = (M2AP_PMCH_Configuration_ItemIEs_t*)calloc(1,sizeof(M2AP_PMCH_Configuration_ItemIEs_t));
      	pmch_configuration_item_ies->id = M2AP_ProtocolIE_ID_id_PMCH_Configuration_Item;
      	pmch_configuration_item_ies->criticality = M2AP_Criticality_reject;
      	pmch_configuration_item_ies->value.present = M2AP_PMCH_Configuration_ItemIEs__value_PR_PMCH_Configuration_Item;

      	M2AP_PMCH_Configuration_Item_t * pmch_configuration_item;/* = (M2AP_PMCH_Configuration_Item_t*)calloc(1,sizeof(M2AP_PMCH_Configuration_Item_t));*/
      	pmch_configuration_item = &pmch_configuration_item_ies->value.choice.PMCH_Configuration_Item;
      	{
            pmch_configuration_item->pmch_Configuration.dataMCS = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].data_mcs;
            pmch_configuration_item->pmch_Configuration.mchSchedulingPeriod = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mch_scheduling_period;
	    pmch_configuration_item->pmch_Configuration.allocatedSubframesEnd = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].allocated_sf_end;
            for(k=0; k < m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].num_mbms_session_list; k++){
            	MBMSsessionListPerPMCH_Item__Member *member;
            	member = (MBMSsessionListPerPMCH_Item__Member*)calloc(1,sizeof(MBMSsessionListPerPMCH_Item__Member));
            	MCC_MNC_TO_PLMNID(
            			  m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mcc,
            			  m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mnc,
            			  m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mnc_length
            			,&member->tmgi.pLMNidentity);

	  	//INT16_TO_OCTET_STRING(0,&imember->tmgi);
		char buf[4];
	        INT32_TO_BUFFER(m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id,buf);
            	//uint8_t TMGI[5] = {4,3,2,1,0};
            	OCTET_STRING_fromBuf(&member->tmgi.serviceID,(const char*)&buf[1],3);

            		//LOG_D(M2AP,"%p,%p,%p,%li,%ld,%ld\n",pmch_configuration_item,&pmch_configuration_item->mbms_Session_List.list,member, m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid,sizeof(member->lcid),sizeof(m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid)); 
		//long kk = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid;
		//uint64_t kk=(m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid);
		//memcpy(&member->lcid,&m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid,sizeof(member->lcid));
	//	member->lcid = 1;
	//	for( l=2; l < 28;l++)
	//		LOG_E(M2AP,"%di\n",l);
	//		if(l ==  m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid){
        //    	member->lcid++;//m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id;
	//		break;
	//	}
                member->lcid = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid;
		//member->lcid = (M2AP_LCID_t*)calloc(1,sizeof(M2AP_LCID_t));
		
            	ASN_SEQUENCE_ADD(&pmch_configuration_item->mbms_Session_List.list,member);
            }
      
      	}
      /*M2AP_MBSFN_Subframe_ConfigurationList_t*/
      // M2AP_MBSFN_Area_Configuration_Item_t *mbsfn_area_configuration_item_ie2;
      //(mbsfn_area_configuration_item_ie+1) =(M2AP_MBSFN_Area_Configuration_Item_t*)calloc(1,sizeof(M2AP_MBSFN_Area_Configuration_Item_t)); 

      	ASN_SEQUENCE_ADD(&mbsfn_area_configuration_item_ie->value.choice.PMCH_Configuration_List.list,pmch_configuration_item_ies);

      }

      ASN_SEQUENCE_ADD(m2ap_mbsfn_area_configuration_list,mbsfn_area_configuration_item_ie);

      /*M2AP_MBSFN_Area_Configuration_Item_t*/
      M2AP_MBSFN_Area_Configuration_Item_t *mbsfn_area_configuration_item_ie_1;
      mbsfn_area_configuration_item_ie_1 =(M2AP_MBSFN_Area_Configuration_Item_t*)calloc(1,sizeof(M2AP_MBSFN_Area_Configuration_Item_t)); 
      (mbsfn_area_configuration_item_ie_1)->id = M2AP_ProtocolIE_ID_id_MBSFN_Subframe_Configuration_List;
      (mbsfn_area_configuration_item_ie_1)->criticality = M2AP_Criticality_reject;
      (mbsfn_area_configuration_item_ie_1)->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_MBSFN_Subframe_ConfigurationList;

      for(j=0; j < m2ap_mbms_scheduling_information->mbms_area_config_list[i].num_mbms_sf_config_list; j++){
	   M2AP_MBSFN_Subframe_ConfigurationItem_t * mbsfn_subframe_configuration_item_ies = (M2AP_MBSFN_Subframe_ConfigurationItem_t*)calloc(1,sizeof(M2AP_MBSFN_Subframe_ConfigurationItem_t));
	   mbsfn_subframe_configuration_item_ies->id = M2AP_ProtocolIE_ID_id_MBSFN_Subframe_Configuration_Item;
	   mbsfn_subframe_configuration_item_ies->criticality = M2AP_Criticality_reject;
	   mbsfn_subframe_configuration_item_ies->value.present = M2AP_MBSFN_Subframe_ConfigurationItem__value_PR_MBSFN_Subframe_Configuration;

	   M2AP_MBSFN_Subframe_Configuration_t * mbsfn_subframe_configuration; /* = (M2AP_MBSFN_Subframe_Configuration_t*)calloc(1,sizeof(M2AP_MBSFN_Subframe_Configuration_t));*/
	   mbsfn_subframe_configuration = &mbsfn_subframe_configuration_item_ies->value.choice.MBSFN_Subframe_Configuration;
	   {
		   mbsfn_subframe_configuration->radioframeAllocationPeriod = m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].radioframe_allocation_period;
		   mbsfn_subframe_configuration->radioframeAllocationOffset = m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].radioframe_allocation_offset;
		   if(m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].is_four_sf){
			   mbsfn_subframe_configuration->subframeAllocation.present = M2AP_MBSFN_Subframe_Configuration__subframeAllocation_PR_fourFrames;
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.buf = MALLOC(3);
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.buf[2] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation) & 0xFF);
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.buf[1] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation>>8) & 0xFF);
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.buf[0] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation>>16) & 0xFF);
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.size =3;
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.bits_unused = 0;

		   }else{
			   mbsfn_subframe_configuration->subframeAllocation.present = M2AP_MBSFN_Subframe_Configuration__subframeAllocation_PR_oneFrame;
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.buf = MALLOC(1);
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.size = 1;
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.bits_unused = 2;
			   //mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.buf[0] = 0x38<<2;
			   mbsfn_subframe_configuration->subframeAllocation.choice.oneFrame.buf[0] = (m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation & 0x3F)<<2;
		   }

		   ASN_SEQUENCE_ADD(&(mbsfn_area_configuration_item_ie_1)->value.choice.MBSFN_Subframe_ConfigurationList.list,mbsfn_subframe_configuration_item_ies);
	   }
       }

       ASN_SEQUENCE_ADD(m2ap_mbsfn_area_configuration_list,mbsfn_area_configuration_item_ie_1);


      /*M2AP_MBSFN_Area_Configuration_Item_t*/
      M2AP_MBSFN_Area_Configuration_Item_t *mbsfn_area_configuration_item_ie_2;
      mbsfn_area_configuration_item_ie_2 =(M2AP_MBSFN_Area_Configuration_Item_t*)calloc(1,sizeof(M2AP_MBSFN_Area_Configuration_Item_t)); 
      (mbsfn_area_configuration_item_ie_2)->id = M2AP_ProtocolIE_ID_id_Common_Subframe_Allocation_Period;
      (mbsfn_area_configuration_item_ie_2)->criticality = M2AP_Criticality_reject;
      (mbsfn_area_configuration_item_ie_2)->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_Common_Subframe_Allocation_Period;
      (mbsfn_area_configuration_item_ie_2)->value.choice.Common_Subframe_Allocation_Period=m2ap_mbms_scheduling_information->mbms_area_config_list[i].common_sf_allocation_period;

       ASN_SEQUENCE_ADD(m2ap_mbsfn_area_configuration_list,mbsfn_area_configuration_item_ie_2);


      /*M2AP_MBSFN_Area_Configuration_Item_t*/
      M2AP_MBSFN_Area_Configuration_Item_t *mbsfn_area_configuration_item_ie_3;
      mbsfn_area_configuration_item_ie_3 =(M2AP_MBSFN_Area_Configuration_Item_t*)calloc(1,sizeof(M2AP_MBSFN_Area_Configuration_Item_t)); 
      (mbsfn_area_configuration_item_ie_3)->id = M2AP_ProtocolIE_ID_id_MBSFN_Area_ID;
      (mbsfn_area_configuration_item_ie_3)->criticality = M2AP_Criticality_reject;
      (mbsfn_area_configuration_item_ie_3)->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_MBSFN_Area_ID;
      (mbsfn_area_configuration_item_ie_3)->value.choice.MBSFN_Area_ID = m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_area_id;

      ASN_SEQUENCE_ADD(m2ap_mbsfn_area_configuration_list,mbsfn_area_configuration_item_ie_3);


ASN_SET_ADD(&ie->value.choice.MBSFN_Area_Configuration_List,m2ap_mbsfn_area_configuration_list);

/* xer_fprint(stdout,&asn_DEF_M2AP_MBSFN_Area_Configuration_Item,mbsfn_area_configuration_item_ie);
 xer_fprint(stdout,&asn_DEF_M2AP_MBSFN_Area_Configuration_Item,mbsfn_area_configuration_item_ie_1);
 xer_fprint(stdout,&asn_DEF_M2AP_MBSFN_Area_Configuration_Item,mbsfn_area_configuration_item_ie_2);
 xer_fprint(stdout,&asn_DEF_M2AP_MBSFN_Area_Configuration_Item,mbsfn_area_configuration_item_ie_3);
 xer_fprint(stdout,&asn_DEF_M2AP_MBSFN_Area_Configuration_List, &ie->value.choice.MBSFN_Area_Configuration_List);*/

  }


 ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);  

 
 
  /* encode */
  if (m2ap_encode_pdu(&pdu,&buffer,&len) < 0){
	return -1;
 }
  m2ap_MCE_itti_send_sctp_data_req(instance,m2ap_mce_data_from_enb->assoc_id,buffer,len,0);
  return 0;
                        
}

int MCE_handle_MBMS_SCHEDULING_INFORMATION_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu){

  LOG_D(M2AP, "MCE_handle_MBMS_SCHEDULING_INFORMATION_RESPONSE\n");

   AssertFatal(pdu->present == M2AP_M2AP_PDU_PR_successfulOutcome,
	       "pdu->present != M2AP_M2AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M2AP_ProcedureCode_id_mbmsSchedulingInformation,
	       "pdu->choice.successfulOutcome->procedureCode != M2AP_ProcedureCode_id_mbmsSchedulingInformation\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M2AP_Criticality_reject,
	       "pdu->choice.successfulOutcome->criticality != M2AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M2AP_SuccessfulOutcome__value_PR_MbmsSchedulingInformationResponse,
	       "pdu->choice.successfulOutcome->value.present != M2AP_SuccessfulOutcome__value_PR_MbmsSchedulingInformationResponse\n");


  //M2AP_MbmsSchedulingInformationResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.MbmsSchedulingInformationResponse;
  //M2AP_MbmsSchedulingInformationResponse_Ies_t  *ie;
  //int MCE_MBMS_M2AP_ID=-1;
  //int ENB_MBMS_M2AP_ID=-1;


  MessageDef *msg_g = itti_alloc_new_message(TASK_M2AP_MCE,M2AP_MBMS_SCHEDULING_INFORMATION_RESP); //TODO

//  LOG_D(M2AP, "M2AP: SessionStop-Resp: protocolIEs.list.count %d\n",
//         in->protocolIEs.list.count);
//  for (int i=0;i < in->protocolIEs.list.count; i++) {
//     ie = in->protocolIEs.list.array[i];
//     //switch (ie->id) {
//     //case M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID:
//     //  AssertFatal(ie->criticality == M2AP_Criticality_reject,
//     //              "ie->criticality != M2AP_Criticality_reject\n");
//     //  AssertFatal(ie->value.present == M2AP_sessionStopIEs__value_PR_MCE_MBMS_M2AP_ID,
//     //              "ie->value.present != M2AP_sessionStopIEs__value_PR_MCE_MBMS_M2AP_ID\n");
//     //  TransactionId=ie->value.choice.MCE_MBMS_M2AP_ID;
//     //  LOG_D(M2AP, "M2AP: SessionStop-Resp: MCE_MBMS_M2AP_ID %d\n",
//     //        MCE_MBMS_M2AP_ID);
//     //  break;
//     // case M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID:
//     //  AssertFatal(ie->criticality == M2AP_Criticality_reject,
//     //              "ie->criticality != M2AP_Criticality_reject\n");
//     //  AssertFatal(ie->value.present == M2AP_sessionStopIEs__value_PR_ENB_MBMS_M2AP_ID,
//     //              "ie->value.present != M2AP_sessionStopIEs__value_PR_ENB_MBMS_M2AP_ID\n");
//     //  TransactionId=ie->value.choice.ENB_MBMS_M2AP_ID;
//     //  LOG_D(M2AP, "M2AP: SessionStop-Resp: ENB_MBMS_M2AP_ID %d\n",
//     //        ENB_MBMS_M2AP_ID);
//     //  break;
//     //}
//  }
//
//  M2AP_SESSION_STOP_RESP(msg_p).
//  MSC_LOG_RX_MESSAGE(
//    MSC_M2AP_MCE,
//    MSC_M2AP_ENB,
//    0,
//    0,
//    MSC_AS_TIME_FMT" MCE_handle_M2_SCHEDULING_INFORMATION_RESPONSE successfulOutcome assoc_id %d",
//    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
//    assoc_id);
//
//   LOG_D(M2AP, "Sending M2AP_SCHEDULING_INFO_RESP ITTI message to ENB_APP with assoc_id (%d->%d)\n",
//         assoc_id,ENB_MODULE_ID_TO_INSTANCE(assoc_id));
//   itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);

    itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(instance), msg_g);
//
   return 0;
                       
}

/*
 * Reset
 */

int MCE_send_RESET(instance_t instance, m2ap_reset_t * m2ap_reset) {
  AssertFatal(1==0,"Not implemented yet\n");
  //M2AP_Reset_t     Reset;
                        
}


int MCE_handle_RESET_ACKKNOWLEDGE(instance_t instance,
                                  uint32_t assoc_id,
                                  uint32_t stream,
                                  M2AP_M2AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");

}

int MCE_handle_RESET(instance_t instance,
                     uint32_t assoc_id,
                     uint32_t stream,
                     M2AP_M2AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int MCE_send_RESET_ACKNOWLEDGE(instance_t instance, M2AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
 *    M2 Setup
 */
int MCE_handle_M2_SETUP_REQUEST(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M2AP_M2AP_PDU_t *pdu)
{
  LOG_D(M2AP, "MCE_handle_M2_SETUP_REQUEST assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  M2AP_M2SetupRequest_t              *container;
  M2AP_M2SetupRequest_Ies_t           *ie;
  int i = 0/*,j=0*/;
  int num_mbms_available=0;

  DevAssert(pdu != NULL);

  container = &pdu->choice.initiatingMessage.value.choice.M2SetupRequest;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received m2 setup request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p = itti_alloc_new_message(TASK_MCE_APP, M2AP_SETUP_REQ); 
  
  /* assoc_id */
  M2AP_SETUP_REQ(message_p).assoc_id = assoc_id;

 /* GlobalENB_id */
 // this function exits if the ie is mandatory
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_Ies_t, ie, container,
                             M2AP_ProtocolIE_ID_id_GlobalENB_ID, true);
  //asn_INTEGER2ulong(&ie->value.choice.GlobalENB_ID.eNB_ID, &M2AP_SETUP_REQ(message_p).GlobalENB_ID);
  if(ie!=NULL){
  if(ie->value.choice.GlobalENB_ID.eNB_ID.present == M2AP_ENB_ID_PR_macro_eNB_ID){
  }else if(ie->value.choice.GlobalENB_ID.eNB_ID.present == M2AP_ENB_ID_PR_short_Macro_eNB_ID){
  }else if(ie->value.choice.GlobalENB_ID.eNB_ID.present == M2AP_ENB_ID_PR_long_Macro_eNB_ID){
  }
  }

  LOG_D(M2AP, "M2AP_SETUP_REQ(message_p).GlobalENB_ID %lu \n", M2AP_SETUP_REQ(message_p).GlobalENB_ID);

  /* ENB_name */
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_Ies_t, ie, container,
                              M2AP_ProtocolIE_ID_id_ENBname, false);
  if(ie!=NULL){
	  M2AP_SETUP_REQ(message_p).ENBname = calloc(ie->value.choice.ENBname.size + 1, sizeof(char));
	  memcpy(M2AP_SETUP_REQ(message_p).ENBname, ie->value.choice.ENBname.buf,
		 ie->value.choice.ENBname.size);

	  /* Convert the mme name to a printable string */
	  M2AP_SETUP_REQ(message_p).ENBname[ie->value.choice.ENBname.size] = '\0';
	  LOG_D(M2AP, "M2AP_SETUP_REQ(message_p).gNB_DU_name %s \n", M2AP_SETUP_REQ(message_p).ENBname);
  }
   /* ENB_MBMS_Configuration_data_List */


  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_Ies_t, ie, container,
                              M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_List, true);

  if(ie!=NULL){
	  M2AP_SETUP_REQ(message_p).num_mbms_available = ie->value.choice.ENB_MBMS_Configuration_data_List.list.count;
	  LOG_D(M2AP, "M2AP_SETUP_REQ(message_p).num_mbms_available %d \n",
		M2AP_SETUP_REQ(message_p).num_mbms_available);
	  num_mbms_available = M2AP_SETUP_REQ(message_p).num_mbms_available;
	  for (i=0; i<num_mbms_available; i++) {
		 M2AP_ENB_MBMS_Configuration_data_Item_t *mbms_configuration_item_p;
		 mbms_configuration_item_p = &(((M2AP_ENB_MBMS_Configuration_data_ItemIEs_t *)ie->value.choice.ENB_MBMS_Configuration_data_List.list.array[i])->value.choice.ENB_MBMS_Configuration_data_Item);
	    /* eCGI */
	       //mbms_configuration_item_p->eCGI ... (M2AP_ECGI_t)

	    OCTET_STRING_TO_INT16(&(mbms_configuration_item_p->eCGI.pLMN_Identity),M2AP_SETUP_REQ(message_p).plmn_identity[i]);
	    //OCTET_STRING_TO_INT16(&(mbms_configuration_item_p->eCGI.eUTRANcellIdentifier),M2AP_SETUP_REQ(message_p).eutran_cell_identifier[i]);
	    /* mbsfnSynchronisationArea */
	       //mbms_configuration_item_p->mbsfnSynchronisationArea ... (M2AP_MBSFN_SynchronisationArea_ID_t)

	    M2AP_SETUP_REQ(message_p).mbsfn_synchronization_area[i]=mbms_configuration_item_p->mbsfnSynchronisationArea;
	    /* mbmsServiceAreaList */
	       //mbms_configuration_item_p->mbmsServiceAreaList ... (M2AP_MBMS_Service_Area_ID_List_t)
	  }
  }
    
//    /* tac */
//    OCTET_STRING_TO_INT16(&(served_celles_item_p->served_Cell_Information.fiveGS_TAC), M2AP_SETUP_REQ(message_p).tac[i]);
//    LOG_D(M2AP, "M2AP_SETUP_REQ(message_p).tac[%d] %d \n",
//          i, M2AP_SETUP_REQ(message_p).tac[i]);
//
//    /* - nRCGI */
//    TBCD_TO_MCC_MNC(&(served_celles_item_p->served_Cell_Information.nRCGI.pLMN_Identity), M2AP_SETUP_REQ(message_p).mcc[i],
//                    M2AP_SETUP_REQ(message_p).mnc[i],
//                    M2AP_SETUP_REQ(message_p).mnc_digit_length[i]);
//    
//    
//    // NR cellID
//    BIT_STRING_TO_NR_CELL_IDENTITY(&served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity,
//				   M2AP_SETUP_REQ(message_p).nr_cellid[i]);
//    LOG_D(M2AP, "[SCTP %d] Received nRCGI: MCC %d, MNC %d, CELL_ID %llu\n", assoc_id,
//          M2AP_SETUP_REQ(message_p).mcc[i],
//          M2AP_SETUP_REQ(message_p).mnc[i],
//          (long long unsigned int)M2AP_SETUP_REQ(message_p).nr_cellid[i]);
//    LOG_D(M2AP, "nr_cellId : %x %x %x %x %x\n",
//          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[0],
//          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[1],
//          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[2],
//          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[3],
//          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[4]);
//    /* - nRPCI */
//    M2AP_SETUP_REQ(message_p).nr_pci[i] = served_celles_item_p->served_Cell_Information.nRPCI;
//    LOG_D(M2AP, "M2AP_SETUP_REQ(message_p).nr_pci[%d] %d \n",
//          i, M2AP_SETUP_REQ(message_p).nr_pci[i]);
//  
//    // System Information
//    /* mib */
//    M2AP_SETUP_REQ(message_p).mib[i] = calloc(served_celles_item_p->gNB_DU_System_Information->mIB_message.size + 1, sizeof(char));
//    memcpy(M2AP_SETUP_REQ(message_p).mib[i], served_celles_item_p->gNB_DU_System_Information->mIB_message.buf,
//           served_celles_item_p->gNB_DU_System_Information->mIB_message.size);
//    /* Convert the mme name to a printable string */
//    M2AP_SETUP_REQ(message_p).mib[i][served_celles_item_p->gNB_DU_System_Information->mIB_message.size] = '\0';
//    M2AP_SETUP_REQ(message_p).mib_length[i] = served_celles_item_p->gNB_DU_System_Information->mIB_message.size;
//    LOG_D(M2AP, "M2AP_SETUP_REQ(message_p).mib[%d] %s , len = %d \n",
//          i, M2AP_SETUP_REQ(message_p).mib[i], M2AP_SETUP_REQ(message_p).mib_length[i]);
//
//    /* sib1 */
//    M2AP_SETUP_REQ(message_p).sib1[i] = calloc(served_celles_item_p->gNB_DU_System_Information->sIB1_message.size + 1, sizeof(char));
//    memcpy(M2AP_SETUP_REQ(message_p).sib1[i], served_celles_item_p->gNB_DU_System_Information->sIB1_message.buf,
//           served_celles_item_p->gNB_DU_System_Information->sIB1_message.size);
//    /* Convert the mme name to a printable string */
//    M2AP_SETUP_REQ(message_p).sib1[i][served_celles_item_p->gNB_DU_System_Information->sIB1_message.size] = '\0';
//    M2AP_SETUP_REQ(message_p).sib1_length[i] = served_celles_item_p->gNB_DU_System_Information->sIB1_message.size;
//    LOG_D(M2AP, "M2AP_SETUP_REQ(message_p).sib1[%d] %s , len = %d \n",
//          i, M2AP_SETUP_REQ(message_p).sib1[i], M2AP_SETUP_REQ(message_p).sib1_length[i]);
//  }


  //printf("m2ap_mce_data_from_enb->assoc_id %d %d\n",m2ap_mce_data_from_enb->assoc_id,assoc_id);

  *m2ap_mce_data_from_enb = M2AP_SETUP_REQ(message_p);
  //printf("m2ap_mce_data_from_enb->assoc_id %d %d\n",m2ap_mce_data_from_enb->assoc_id,assoc_id);

//  MSC_LOG_TX_MESSAGE(
//  MSC_M2AP_MCE,
//  MSC_RRC_ENB,
//  0,
//  0,
//  MSC_AS_TIME_FMT" MCE_handle_M2_SETUP_REQUEST",
//  0,0//MSC_AS_TIME_ARGS(ctxt_pP),
//  );
//
  if (num_mbms_available > 0) {
    itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);
  } else {
       //MCE_send_M2_SETUP_FAILURE(instance);
       return -1;
  }
//  return 0;
    //TEST POINT MCE -> eNB
//    if(1){
//	printf("instance %d\n",instance);
//	//MCE_send_M2_SETUP_RESPONSE(instance,assoc_id,m2ap_mce_data_from_enb->assoc_id);
//	//MCE_send_MBMS_SESSION_START_REQUEST(instance,assoc_id);
//	//MCE_send_MBMS_SESSION_STOP_REQUEST(instance,assoc_id);
//	//MCE_send_MBMS_SCHEDULING_INFORMATION(instance,assoc_id,NULL); //TODO
//    }
//    else 
//	MCE_send_M2_SETUP_FAILURE(instance,assoc_id);

    return 0;
}

int MCE_send_M2_SETUP_RESPONSE(instance_t instance, /*uint32_t assoc_id,*/
                               m2ap_setup_resp_t *m2ap_setup_resp) {
  
  //module_id_t mce_mod_idP;
  //module_id_t enb_mod_idP;

  // This should be fixed
  //enb_mod_idP = (module_id_t)0;
  //mce_mod_idP  = (module_id_t)0;

  M2AP_M2AP_PDU_t           pdu;
  M2AP_M2SetupResponse_t    *out;
  M2AP_M2SetupResponse_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;
  //int       j = 0;

  AssertFatal(m2ap_setup_resp!=NULL,"m2ap_setup_resp = NULL\n");

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
  //pdu.choice.successfulOutcome = (M2AP_SuccessfulOutcome_t *)calloc(1, sizeof(M2AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_m2Setup;
  pdu.choice.successfulOutcome.criticality   = M2AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_M2SetupResponse;
  out = &pdu.choice.successfulOutcome.value.choice.M2SetupResponse;
  
  /* mandatory */
  /* c1. GlobalMCE ID (integer value)*/
  ie = (M2AP_M2SetupResponse_Ies_t *)calloc(1, sizeof(M2AP_M2SetupResponse_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_GlobalMCE_ID;
  ie->criticality               = M2AP_Criticality_reject; //?
  ie->value.present             = M2AP_M2SetupResponse_Ies__value_PR_GlobalMCE_ID;
  //ie->value.choice.TransactionID = M2AP_get_next_transaction_identifier(enb_mod_idP, cu_mod_idP);
  MCC_MNC_TO_PLMNID(m2ap_setup_resp->mcc, m2ap_setup_resp->mnc, m2ap_setup_resp->mnc_digit_length,
  		&ie->value.choice.GlobalMCE_ID.pLMN_Identity);

  ie->value.choice.GlobalMCE_ID.mCE_ID.buf =calloc(2, sizeof(uint8_t));
  ie->value.choice.GlobalMCE_ID.mCE_ID.buf[0] = (m2ap_setup_resp->MCE_id) >> 8;
  ie->value.choice.GlobalMCE_ID.mCE_ID.buf[1] = ((m2ap_setup_resp->MCE_id) & 0x0ff); 
  ie->value.choice.GlobalMCE_ID.mCE_ID.size=2;
  //ie->value.choice.GlobalMCE_ID.mCE_ID.bits_unused=0;

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
 
  /* optional */
  /* c2. MCEname */
  if (m2ap_setup_resp->MCE_name != NULL) {
    ie = (M2AP_M2SetupResponse_Ies_t *)calloc(1, sizeof(M2AP_M2SetupResponse_Ies_t));
    ie->id                        = M2AP_ProtocolIE_ID_id_MCEname;
    ie->criticality               = M2AP_Criticality_ignore;
    ie->value.present             = M2AP_M2SetupResponse_Ies__value_PR_MCEname;
    OCTET_STRING_fromBuf(&ie->value.choice.MCEname, m2ap_setup_resp->MCE_name,
                         strlen(m2ap_setup_resp->MCE_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c3. cells to be Activated list */
  ie = (M2AP_M2SetupResponse_Ies_t *)calloc(1, sizeof(M2AP_M2SetupResponse_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_M2SetupResponse_Ies__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea;

  for( i=0; i < m2ap_setup_resp->num_mcch_config_per_mbsfn; i++) 
  {

	  M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t * mcch_related_bcch_config_per_mbsfn_area_item_ies;
	  mcch_related_bcch_config_per_mbsfn_area_item_ies = (M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t*)calloc(1,sizeof(M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t));
	  mcch_related_bcch_config_per_mbsfn_area_item_ies->id=M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item;
	  mcch_related_bcch_config_per_mbsfn_area_item_ies->criticality = M2AP_Criticality_ignore;
	  mcch_related_bcch_config_per_mbsfn_area_item_ies->value.present=M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item;

	  
	  M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item_t * config_per_mbsfn_area_item;
	  config_per_mbsfn_area_item = &mcch_related_bcch_config_per_mbsfn_area_item_ies->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea_Item;
	  {
		config_per_mbsfn_area_item->pdcchLength=m2ap_setup_resp->mcch_config_per_mbsfn[i].pdcch_length;//M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__pdcchLength_s1;
		config_per_mbsfn_area_item->offset=m2ap_setup_resp->mcch_config_per_mbsfn[i].offset;//0;
		config_per_mbsfn_area_item->repetitionPeriod=m2ap_setup_resp->mcch_config_per_mbsfn[i].repetition_period;//M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__repetitionPeriod_rf32;
		config_per_mbsfn_area_item->modificationPeriod=m2ap_setup_resp->mcch_config_per_mbsfn[i].modification_period;//M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__modificationPeriod_rf512;
		config_per_mbsfn_area_item->subframeAllocationInfo.buf = MALLOC(1); 
		config_per_mbsfn_area_item->subframeAllocationInfo.size=1;
		/*char * t;
		int bits=7;
		for( t = m2ap_setup_resp->mcch_config_per_mbsfn[i].subframe_allocation_info; *t != '\0'; t++,bits--){
			if(*t=='1'){
				config_per_mbsfn_area_item->subframeAllocationInfo.buf[0] |= (uint8_t)(0x1<<bits);
			}
                }*/
		config_per_mbsfn_area_item->subframeAllocationInfo.buf[0] = (uint8_t)((m2ap_setup_resp->mcch_config_per_mbsfn[i].subframe_allocation_info & 0x3F)<<2);

		//config_per_mbsfn_area_item->subframeAllocationInfo.buf[0]=0x1<<7;
		config_per_mbsfn_area_item->subframeAllocationInfo.bits_unused=2;
		
		//memset(&config_per_mbsfn_area_item->subframeAllocationInfo,0,sizeof(BIT_STRING_t));
		
		config_per_mbsfn_area_item->modulationAndCodingScheme = m2ap_setup_resp->mcch_config_per_mbsfn[i].mcs;//M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__modulationAndCodingScheme_n2;
		ASN_SEQUENCE_ADD(&ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list,mcch_related_bcch_config_per_mbsfn_area_item_ies);
	  }

  }
 
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  

  /* encode */
  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M2AP, "Failed to encode M2 response\n");
    return -1;
  }

  m2ap_MCE_itti_send_sctp_data_req(instance,m2ap_mce_data_from_enb->assoc_id,buffer,len,0);

  return 0;
}

int MCE_send_M2_SETUP_FAILURE(instance_t instance,m2ap_setup_failure_t* m2ap_setup_failure) {
  
 // module_id_t enb_mod_idP;
  //module_id_t mce_mod_idP;

  // This should be fixed
  //enb_mod_idP = (module_id_t)0;
  //mce_mod_idP  = (module_id_t)0;

  M2AP_M2AP_PDU_t           pdu;
  M2AP_M2SetupFailure_t    *out;
  M2AP_M2SetupFailure_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_unsuccessfulOutcome;
  //pdu.choice.unsuccessfulOutcome = (M2AP_UnsuccessfulOutcome_t *)calloc(1, sizeof(M2AP_UnsuccessfulOutcome_t));
  pdu.choice.unsuccessfulOutcome.procedureCode = M2AP_ProcedureCode_id_m2Setup;
  pdu.choice.unsuccessfulOutcome.criticality   = M2AP_Criticality_reject;
  pdu.choice.unsuccessfulOutcome.value.present = M2AP_UnsuccessfulOutcome__value_PR_M2SetupFailure;
  out = &pdu.choice.unsuccessfulOutcome.value.choice.M2SetupFailure;

  /* mandatory */
  /* c1. Transaction ID (integer value)*/
 // ie = (M2AP_M2SetupFailure_Ies_t *)calloc(1, sizeof(M2AP_M2SetupFailure_Ies_t));
 // ie->id                        = M2AP_ProtocolIE_ID_id_GlobalENB_ID;
 // ie->criticality               = M2AP_Criticality_reject;
 // ie->value.present             = M2AP_M2SetupFailure_Ies__value_PR_GlobalENB_ID;
 // ie->value.choice.GlobalENB_ID = M2AP_get_next_transaction_identifier(enb_mod_idP, mce_mod_idP);
 // ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. Cause */
  ie = (M2AP_M2SetupFailure_Ies_t *)calloc(1, sizeof(M2AP_M2SetupFailure_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_Cause;
  ie->criticality               = M2AP_Criticality_ignore;
  ie->value.present             = M2AP_M2SetupFailure_Ies__value_PR_Cause;
  ie->value.choice.Cause.present = M2AP_Cause_PR_radioNetwork;
  ie->value.choice.Cause.choice.radioNetwork = M2AP_CauseRadioNetwork_unspecified;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. TimeToWait */
  if (0) {
    ie = (M2AP_M2SetupFailure_Ies_t *)calloc(1, sizeof(M2AP_M2SetupFailure_Ies_t));
    ie->id                        = M2AP_ProtocolIE_ID_id_TimeToWait;
    ie->criticality               = M2AP_Criticality_ignore;
    ie->value.present             = M2AP_M2SetupFailure_Ies__value_PR_TimeToWait;
    ie->value.choice.TimeToWait = M2AP_TimeToWait_v10s;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c4. CriticalityDiagnostics*/
  if (0) {
    ie = (M2AP_M2SetupFailure_Ies_t *)calloc(1, sizeof(M2AP_M2SetupFailure_Ies_t));
    ie->id                        = M2AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality               = M2AP_Criticality_ignore;
    ie->value.present             = M2AP_M2SetupFailure_Ies__value_PR_CriticalityDiagnostics;
    ie->value.choice.CriticalityDiagnostics.procedureCode = (M2AP_ProcedureCode_t *)calloc(1, sizeof(M2AP_ProcedureCode_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCode = M2AP_ProcedureCode_id_m2Setup;
    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (M2AP_TriggeringMessage_t *)calloc(1, sizeof(M2AP_TriggeringMessage_t));
    *ie->value.choice.CriticalityDiagnostics.triggeringMessage = M2AP_TriggeringMessage_initiating_message;
    ie->value.choice.CriticalityDiagnostics.procedureCriticality = (M2AP_Criticality_t *)calloc(1, sizeof(M2AP_Criticality_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCriticality = M2AP_Criticality_reject;
    //ie->value.choice.CriticalityDiagnostics.transactionID = (M2AP_TransactionID_t *)calloc(1, sizeof(M2AP_TransactionID_t));
    //*ie->value.choice.CriticalityDiagnostics.transactionID = 0;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M2AP, "Failed to encode M2 setup request\n");
    return -1;
  }

  //mce_m2ap_itti_send_sctp_data_req(instance, m2ap_mce_data_from_enb->assoc_id, buffer, len, 0);
  m2ap_MCE_itti_send_sctp_data_req(instance,m2ap_mce_data_from_enb->assoc_id,buffer,len,0);

  return 0;
}

/*
 * MCE Configuration Update
 */

int MCE_send_MCE_CONFIGURATION_UPDATE(instance_t instance, module_id_t du_mod_idP){
  AssertFatal(1==0,"Not implemented yet\n");

  
  M2AP_M2AP_PDU_t          pdu;
  M2AP_MCEConfigurationUpdate_t    *out;
  M2AP_MCEConfigurationUpdate_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;
  //int       j = 0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M2AP_InitiatingMessage_t *)calloc(1, sizeof(M2AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_mCEConfigurationUpdate;
  pdu.choice.initiatingMessage.criticality   = M2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_MCEConfigurationUpdate;
  out = &pdu.choice.initiatingMessage.value.choice.MCEConfigurationUpdate;
 
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  ie = (M2AP_MCEConfigurationUpdate_Ies_t *)calloc(1, sizeof(M2AP_MCEConfigurationUpdate_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_GlobalMCE_ID;
  ie->criticality               = M2AP_Criticality_reject; //?
  ie->value.present             = M2AP_MCEConfigurationUpdate_Ies__value_PR_GlobalMCE_ID;
  //ie->value.choice.TransactionID = M2AP_get_next_transaction_identifier(enb_mod_idP, cu_mod_idP);
  MCC_MNC_TO_PLMNID(0,0,3/*instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,*/
  ,&ie->value.choice.GlobalMCE_ID.pLMN_Identity);

  ie->value.choice.GlobalMCE_ID.mCE_ID.buf =calloc(2, sizeof(uint8_t));
  ie->value.choice.GlobalMCE_ID.mCE_ID.size=2;
  //ie->value.choice.GlobalMCE_ID.mCE_ID.bits_unused=6;

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
 
  /* optional */
  /* c2. MCEname */
  //if (m2ap_setup_resp->MCEname != NULL) {
  //  ie = (M2AP_MCEConfigurationUpdate_Ies_t *)calloc(1, sizeof(M2AP_MCEConfigurationUpdate_Ies_t));
  //  ie->id                        = M2AP_ProtocolIE_ID_id_MCEname;
  //  ie->criticality               = M2AP_Criticality_ignore;
  //  ie->value.present             = M2AP_MCEConfigurationUpdate_Ies__value_PR_MCEname;
  //  OCTET_STRING_fromBuf(&ie->value.choice.MCEname, m2ap_setup_resp->MCEname,
  //                       strlen(m2ap_setup_resp->MCEname));
  //  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  //}

  /* mandatory */
  /* c3. cells to be Activated list */
  ie = (M2AP_MCEConfigurationUpdate_Ies_t *)calloc(1, sizeof(M2AP_MCEConfigurationUpdate_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_MCEConfigurationUpdate_Ies__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea;

  M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t * mcch_related_bcch_config_per_mbsfn_area_item_ies;
  mcch_related_bcch_config_per_mbsfn_area_item_ies = (M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t*)calloc(1,sizeof(M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t));
  mcch_related_bcch_config_per_mbsfn_area_item_ies->id=M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item;
  mcch_related_bcch_config_per_mbsfn_area_item_ies->criticality = M2AP_Criticality_ignore;
  mcch_related_bcch_config_per_mbsfn_area_item_ies->value.present=M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item;

  M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item_t * config_per_mbsfn_area_item;
  config_per_mbsfn_area_item = &mcch_related_bcch_config_per_mbsfn_area_item_ies->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea_Item;
  {
	config_per_mbsfn_area_item->pdcchLength=M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__pdcchLength_s1;
	config_per_mbsfn_area_item->offset=0;
	config_per_mbsfn_area_item->repetitionPeriod=M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__repetitionPeriod_rf32;
	config_per_mbsfn_area_item->modificationPeriod=M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__modificationPeriod_rf512;
  	config_per_mbsfn_area_item->subframeAllocationInfo.buf = MALLOC(1); 
  	config_per_mbsfn_area_item->subframeAllocationInfo.size=1;
  	//config_per_mbsfn_area_item->subframeAllocationInfo.buf[0]=0x1<<7;
  	config_per_mbsfn_area_item->subframeAllocationInfo.bits_unused=2;
	
	//memset(&config_per_mbsfn_area_item->subframeAllocationInfo,0,sizeof(BIT_STRING_t));
	
        config_per_mbsfn_area_item->modulationAndCodingScheme = M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item__modulationAndCodingScheme_n2;
	ASN_SEQUENCE_ADD(&ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list,mcch_related_bcch_config_per_mbsfn_area_item_ies);
  }
  
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  

  /* encode */
  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M2AP, "Failed to encode M2 response\n");
    return -1;
  }

  //MCE_m2ap_itti_send_sctp_data_req(instance, m2ap_mce_data_from_du->assoc_id, buffer, len, 0);
 //printf(",m2ap_mce_data_from_enb->assoc_id %d\n",m2ap_mce_data_from_enb->assoc_id);

  m2ap_MCE_itti_send_sctp_data_req(instance,m2ap_mce_data_from_enb->assoc_id,buffer,len,0);



  
}


int MCE_handle_MCE_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");

    LOG_D(M2AP, "MCE_handle_MCE_CONFIGURATION_UPDATE_FAILURE\n");

  M2AP_MCEConfigurationUpdateFailure_t    *in = &pdu->choice.unsuccessfulOutcome.value.choice.MCEConfigurationUpdateFailure;


   //M2AP_MCEConfigurationUpdateFailure_Ies_t *ie;


  MessageDef *msg_p = itti_alloc_new_message (TASK_M2AP_MCE, M2AP_MCE_CONFIGURATION_UPDATE_FAILURE);

   LOG_D(M2AP, "M2AP: MCEConfigurationUpdate-Failure: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
  // for (int i=0;i < in->protocolIEs.list.count; i++) {
  //   ie = in->protocolIEs.list.array[i];
  //  // switch (ie->id) {
  //  // case M2AP_ProtocolIE_ID_id_TimeToWait:
  //  //   AssertFatal(ie->criticality == M2AP_Criticality_ignore,
  //  //             "ie->criticality != M2AP_Criticality_ignore\n");
  //  //   AssertFatal(ie->value.present == M2AP_M2SetupFailure_Ies__value_PR_TimeToWait,
  //  //             "ie->value.present != M2AP_M2SetupFailure_Ies__value_PR_TimeToWait\n");
  //  //   LOG_D(M2AP, "M2AP: M2Setup-Failure: TimeToWait %d\n");/*,
  //  //         GlobalMCE_ID);*/
  //  //   break;
  //  // }
  // }
   //AssertFatal(GlobalMCE_ID!=-1,"GlobalMCE_ID was not sent\n");
   //AssertFatal(num_cells_to_activate>0,"No cells activated\n");
   //M2AP_SETUP_RESP (msg_p).num_cells_to_activate = num_cells_to_activate;

   //for (int i=0;i<num_cells_to_activate;i++)
   //  AssertFatal(M2AP_SETUP_RESP (msg_p).num_SI[i] > 0, "System Information %d is missing",i);

   //MSC_LOG_RX_MESSAGE(
   // MSC_M2AP_eNB,
   // MSC_M2AP_CU,
   // 0,
   // 0,
   // MSC_AS_TIME_FMT" eNB_handle_M2_SETUP_RESPONSE successfulOutcome assoc_id %d",
   // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
   // assoc_id);

   //LOG_D(M2AP, "Sending M2AP_SETUP_RESP ITTI message to ENB_APP with assoc_id (%d->%d)\n",
   //      assoc_id,ENB_MOeNBLE_ID_TO_INSTANCE(assoc_id));

   itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);

   return 0;

}


int MCE_handle_MCE_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M2AP, "MCE_handle_MCE_CONFIGURATION_UPDATE_ACKNOWLEDGE assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_MCEConfigurationUpdateAcknowledge_t              *container;
  //M2AP_MCEConfigurationUpdateAcknowledge_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.MCEConfigurationUpdate;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_MCE, M2AP_MCE_CONFIGURATION_UPDATE_ACK);


  itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);

  return 0;

}


/*
 * ENB Configuration Update
 */


int MCE_handle_ENB_CONFIGURATION_UPDATE(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M2AP, "MCE_handle_MCE_CONFIGURATION_UPDATE_ACKNOWLEDGE assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_MCEConfigurationUpdateAcknowledge_t              *container;
  //M2AP_MCEConfigurationUpdateAcknowledge_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.MCEConfigurationUpdate;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_MCE, M2AP_MCE_CONFIGURATION_UPDATE_ACK);


  itti_send_msg_to_task(TASK_MCE_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);

   return 0;

}


int MCE_send_ENB_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                    m2ap_enb_configuration_update_failure_t * m2ap_enb_configuration_update_failure){
  AssertFatal(1==0,"Not implemented yet\n");

}


int MCE_send_ENB_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                    m2ap_enb_configuration_update_ack_t *m2ap_enb_configuration_update_ack){
  AssertFatal(1==0,"Not implemented yet\n");

}


/*
 * Error Indication
 */

int MCE_handle_ERROR_INDICATION(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                M2AP_M2AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");


}

int MCE_send_ERROR_INDICATION(instance_t instance, M2AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}




/*
 * Session Update Request
 */
int MCE_send_MBMS_SESSION_UPDATE_REQUEST(instance_t instance, m2ap_mbms_session_update_req_t * m2ap_mbms_session_update_req){
  AssertFatal(1==0,"Not implemented yet\n");
  //M2AP_SessionUpdateRequest_t      SessionUpdateRequest;
}



int MCE_handle_MBMS_SESSION_UPDATE_RESPONSE(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");

}



int MCE_handle_MBMS_SESSION_UPDATE_FAILURE(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
 * Service Counting Request
 */
int MCE_send_MBMS_SERVICE_COUNTING_REQUEST(instance_t instance, module_id_t du_mod_idP){
  AssertFatal(1==0,"Not implemented yet\n");
  //M2AP_MbmsServiceCountingRequest_t        MbmsServiceCountingRequest;
}



int MCE_handle_MBMS_SERVICE_COUNTING_RESPONSE(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");
}



int MCE_handle_MBMS_SESSION_COUNTING_FAILURE(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");

}


/*
 * Service Counting Results Report
 */

int MCE_handle_MBMS_SESSION_COUNTING_RESULTS_REPORT(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");

}


/*
 * Overload Notification
 */
int MCE_handle_MBMS_OVERLOAD_NOTIFICATION(instance_t instance,
                                                      uint32_t assoc_id,
                                                      uint32_t stream,
                                                      M2AP_M2AP_PDU_t *pdu){
  AssertFatal(1==0,"Not implemented yet\n");
 
}




