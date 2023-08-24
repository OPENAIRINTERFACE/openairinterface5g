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

/*! \file m3ap_MME_interface_management.c
 * \brief m3ap interface management for MME
 * \author Javier Morgade
 * \date 2019
 * \version 0.1
 * \company Vicomtech
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#include "m3ap_common.h"
#include "m3ap_encoder.h"
#include "m3ap_decoder.h"
#include "m3ap_itti_messaging.h"
#include "m3ap_MME_interface_management.h"

#include "conversions.h"

#include "M3AP_ECGI.h"

extern m3ap_setup_req_t *m3ap_mme_data_from_mce;



uint8_t m3ap_start_message[] = {
  0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x07, 0x00,
  0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x02, 0x00,
  0x07, 0x00, 0x55, 0xf5, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x04, 0x00, 0x0d, 0x60, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x40, 0x01,
  0x7a, 0x00, 0x05, 0x00, 0x03, 0x07, 0x08, 0x00,
  0x00, 0x06, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00,
  0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x07, 0x00,
  0x0e, 0x00, 0xe8, 0x0a, 0x0a, 0x0a, 0x00, 0x0a,
  0xc8, 0x0a, 0xfe, 0x00, 0x00, 0x00, 0x01
};


/*
 * MBMS Session start
 */
int MME_send_MBMS_SESSION_START_REQUEST(instance_t instance /*, sctp_assoc_t assoc_id*/,
                                        m3ap_session_start_req_t *m3ap_session_start_req)
{
  //AssertFatal(1==0,"Not implemented yet\n");
    
  //module_id_t enb_mod_idP=0;
  //module_id_t du_mod_idP=0;

  M3AP_M3AP_PDU_t          pdu; 
  M3AP_MBMSSessionStartRequest_t       *out;
  M3AP_MBMSSessionStartRequest_IEs_t   *ie;

  uint8_t *buffer;
  uint32_t len;
  //int	   i=0; 
  //int 	   j=0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M3AP_InitiatingMessage_t *)calloc(1, sizeof(M3AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M3AP_ProcedureCode_id_mBMSsessionStart;
  pdu.choice.initiatingMessage.criticality   = M3AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M3AP_InitiatingMessage__value_PR_MBMSSessionStartRequest;
  out = &pdu.choice.initiatingMessage.value.choice.MBMSSessionStartRequest; 

  /* mandatory */
  /* c1. MME_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_MME_MBMS_M3AP_ID;
  //ie->value.choice.MME_MBMS_M3AP_ID = 0;
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. TMGI */ 
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_TMGI; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_TMGI;
  M3AP_TMGI_t	*tmgi 		= &ie->value.choice.TMGI;
  {
	  MCC_MNC_TO_PLMNID(0,0,3,&tmgi->pLMNidentity);
	  uint8_t TMGI[5] = {4,3,2,1,0};
	  OCTET_STRING_fromBuf(&tmgi->serviceID,(const char*)&TMGI[2],3);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* optional */
  /* c2. MBMS_Session_ID */
  if(0){
	  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Session_ID;
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_MBMS_Session_ID;
	  //M3AP_MBMS_Session_ID_t * mbms_session_id = &ie->value.choice.MBMS_Session_ID;

	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }
  /* mandatory */
  /* c2. MBMS_E_RAB_QoS_Parameters */ 
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_E_RAB_QoS_Parameters; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_MBMS_E_RAB_QoS_Parameters;
  M3AP_MBMS_E_RAB_QoS_Parameters_t * mbms_e_rab_qos_parameters = &ie->value.choice.MBMS_E_RAB_QoS_Parameters;
  {
	//M3AP_QCI_t * qci = &mbms_e_rab_qos_parameters->qCI; //long
	mbms_e_rab_qos_parameters->qCI = 1;
	//struct M3AP_GBR_QosInformation  *gbrQosInformation;     /* OPTIONAL */
  }

  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c2. MBMS_Session_Duration */ 
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Session_Duration; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_MBMS_Session_Duration; 
  M3AP_MBMS_Session_Duration_t * mbms_session_duration = &ie->value.choice.MBMS_Session_Duration;
  {
	uint8_t duration[5] = {4,3,2,1,0};
	OCTET_STRING_fromBuf(mbms_session_duration,(const char*)&duration[2],3);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c2 MBMS_Service_Area */
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Service_Area;
  ie->criticality               = M3AP_Criticality_ignore;
  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_MBMS_Service_Area;
  M3AP_MBMS_Service_Area_t * mbms_service_area = &ie->value.choice.MBMS_Service_Area;
  {
	uint8_t duration[5] = {4,3,2,1,0};
	OCTET_STRING_fromBuf(mbms_service_area,(const char*)&duration[2],1);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c2 MinimumTimeToMBMSDataTransfer */
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MinimumTimeToMBMSDataTransfer; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_MinimumTimeToMBMSDataTransfer; 
  M3AP_MinimumTimeToMBMSDataTransfer_t * minimumtimetombmsdatatransfer = &ie->value.choice.MinimumTimeToMBMSDataTransfer;
  {
	uint8_t duration[5] = {4,3,2,1,0};
	OCTET_STRING_fromBuf(minimumtimetombmsdatatransfer,(const char*)&duration[2],1);
  }

  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c2 TNL_Information */
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_TNL_Information; 
  ie->criticality               = M3AP_Criticality_ignore;
  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_TNL_Information; 
  M3AP_TNL_Information_t * tnl_information = &ie->value.choice.TNL_Information;
  {
        //OCTET_STRING_fromBuf(&tnl_information->iPMCAddress,"1204",strlen("1234"));
        //OCTET_STRING_fromBuf(&tnl_information->iPSourceAddress,"1204",strlen("1234"));
        //OCTET_STRING_fromBuf(&tnl_information->gTP_DLTEID,"1204",strlen("1234"));
	uint32_t gtp_dlteid = 1;
	GTP_TEID_TO_ASN1(gtp_dlteid,&tnl_information->gTP_DLTEID);
	//tnl_information->iPMCAddress.buf = calloc(4,sizeof(uint8_t));
	//tnl_information->iPMCAddress.buf[0]
	//tnl_information->iPMCAddress.buf[1]
	//tnl_information->iPMCAddress.buf[2]
	//tnl_information->iPMCAddress.buf[3]
	//tnl_information->iPMCAddress.buf.size = 4;
  uint32_t ip = (224U << 24) | (0) << 16 | (0) << 8 | (2);
  INT32_TO_OCTET_STRING(ip, &tnl_information->iPMCAddress);
  ip = (224U << 24) | (0) << 16 | (0) << 8 | (2);
  INT32_TO_OCTET_STRING(ip, &tnl_information->iPSourceAddress);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* optional */
  /* c2 Time_ofMBMS_DataTransfer */
  if(0){
	  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_Time_ofMBMS_DataTransfer; 
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_Absolute_Time_ofMBMS_Data; 
	  //M3AP_Absolute_Time_ofMBMS_Data_t * absolute_time_ofmbms_data	= &ie->value.choice.Absolute_Time_ofMBMS_Data;
	  //{
		//char duration[5] = {4,3,2,1,0};
		//OCTET_STRING_fromBuf(absolute_time_ofmbms_data,(const char*)&duration[2],3);
	  //}
	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }
  /* optional */
  /* c2 MBMS_Cell_List */
  if(0){
	  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Cell_List; 
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionStartRequest_IEs__value_PR_MBMS_Cell_List; 
	  M3AP_MBMS_Cell_List_t * mbms_cell_list = &ie->value.choice.MBMS_Cell_List;
	  M3AP_ECGI_t * ecgi = (M3AP_ECGI_t*)calloc(1,sizeof(M3AP_ECGI_t));	
	  {
		//MCC_MNC_TO_PLMNID(0,0,3,ecgi->pLMN_Identity);
		//MACRO_ENB_ID_TO_CELL_IDENTITY(1,0,ecgi->eUTRANcellIdentifier);
		asn1cSeqAdd(&mbms_cell_list->list,ecgi);
	  }
	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (m3ap_encode_pdu(&pdu,&buffer,&len) < 0){
        return -1;
  }

  
//  buffer = &m3ap_start_message[0];
//  len=8*9+7;
//
  m3ap_MME_itti_send_sctp_data_req(instance, m3ap_mme_data_from_mce->assoc_id, buffer, len, 0);
  
  return 0;
}

int MME_handle_MBMS_SESSION_START_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
//  AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M3AP, "MME_handle_MBMS_SESSION_START_RESPONSE\n");

   AssertFatal(pdu->present == M3AP_M3AP_PDU_PR_successfulOutcome,
               "pdu->present != M3AP_M3AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M3AP_ProcedureCode_id_mBMSsessionStart,
               "pdu->choice.successfulOutcome->procedureCode != M3AP_ProcedureCode_id_sessionStart\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M3AP_Criticality_reject,
               "pdu->choice.successfulOutcome->criticality != M3AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M3AP_SuccessfulOutcome__value_PR_MBMSSessionStartResponse,
               "pdu->choice.successfulOutcome.value.present != M3AP_SuccessfulOutcome__value_PR_MBMSSessionStartResponse\n");


  M3AP_MBMSSessionStartResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.MBMSSessionStartResponse;
  M3AP_MBMSSessionStartResponse_IEs_t  *ie;
  int MME_MBMS_M3AP_ID=-1;
  int MCE_MBMS_M3AP_ID=-1;


  MessageDef *msg_g = itti_alloc_new_message(TASK_M3AP_MME, 0,M3AP_MBMS_SESSION_START_RESP); //TODO

  LOG_D(M3AP, "M3AP: SessionStart-Resp: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
  for (int i=0;i < in->protocolIEs.list.count; i++) {
     ie = in->protocolIEs.list.array[i];
     switch (ie->id) {
     case M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID:
       AssertFatal(ie->criticality == M3AP_Criticality_reject,
                   "ie->criticality != M3AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M3AP_MBMSSessionStartResponse_IEs__value_PR_MME_MBMS_M3AP_ID,
                   "ie->value.present != M3AP_sessionStartResponse_IEs__value_PR_MME_MBMS_M3AP_ID\n");
       MME_MBMS_M3AP_ID=ie->value.choice.MME_MBMS_M3AP_ID;
       LOG_D(M3AP, "M3AP: SessionStart-Resp: MME_MBMS_M3AP_ID %d\n",
             MME_MBMS_M3AP_ID);
       break;
      case M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID:
       AssertFatal(ie->criticality == M3AP_Criticality_reject,
                   "ie->criticality != M3AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M3AP_MBMSSessionStartResponse_IEs__value_PR_MCE_MBMS_M3AP_ID,
                   "ie->value.present != M3AP_sessionStartResponse_IEs__value_PR_MCE_MBMS_M3AP_ID\n");
       MCE_MBMS_M3AP_ID=ie->value.choice.MCE_MBMS_M3AP_ID;
       LOG_D(M3AP, "M3AP: SessionStart-Resp: MCE_MBMS_M3AP_ID %d\n",
             MCE_MBMS_M3AP_ID);
       break;
     }
  }

  //AssertFatal(MME_MBMS_M3AP_ID!=-1,"MME_MBMS_M3AP_ID was not sent\n");
  //AssertFatal(MCE_MBMS_M3AP_ID!=-1,"MCE_MBMS_M3AP_ID was not sent\n");
  ////M3AP_SESSION_START_RESP(msg_p).
  // LOG_W(M3AP, "Sending M3AP_SESSION_START_RESP ITTI message to MCE_APP with assoc_id (%d->%d)\n",
  //       assoc_id,ENB_MODULE_ID_TO_INSTANCE(assoc_id));
  itti_send_msg_to_task(TASK_MME_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_g);
  return 0;
}
int MME_handle_MBMS_SESSION_START_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
 * MBMS Session stop
 */
int MME_send_MBMS_SESSION_STOP_REQUEST(instance_t instance, m3ap_session_stop_req_t* m3ap_session_stop_req){

    
 // module_id_t enb_mod_idP=0;
 // module_id_t du_mod_idP=0;

  M3AP_M3AP_PDU_t          pdu; 
  M3AP_MBMSSessionStopRequest_t        *out;
  M3AP_MBMSSessionStopRequest_IEs_t    *ie;

  uint8_t *buffer;
  uint32_t len;
  //int	   i=0; 
  //int 	   j=0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M3AP_InitiatingMessage_t *)calloc(1, sizeof(M3AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M3AP_ProcedureCode_id_mBMSsessionStop;
  pdu.choice.initiatingMessage.criticality   = M3AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M3AP_InitiatingMessage__value_PR_MBMSSessionStopRequest;
  out = &pdu.choice.initiatingMessage.value.choice.MBMSSessionStopRequest; 

  /* mandatory */
  /* c1. MME_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionStopRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStopRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStopRequest_IEs__value_PR_MME_MBMS_M3AP_ID;
  //ie->value.choice.MME_MBMS_M3AP_ID = 0;
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. MCE_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionStopRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStopRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionStopRequest_IEs__value_PR_MCE_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = 0;

  asn1cSeqAdd(&out->protocolIEs.list, ie);


  /* encode */
  if (m3ap_encode_pdu(&pdu,&buffer,&len) < 0){
        return -1;
  }

  //MME_m3ap_itti_send_sctp_data_req(instance, m3ap_mce_data_from_mce->assoid,buffer,len,0); 
  m3ap_MME_itti_send_sctp_data_req(instance, m3ap_mme_data_from_mce->assoc_id,buffer,len,0);
  return 0;


                        
}

int MME_handle_MBMS_SESSION_STOP_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  //AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M3AP, "eNB_handle_MBMS_SESSION_STOP_RESPONSE\n");

   AssertFatal(pdu->present == M3AP_M3AP_PDU_PR_successfulOutcome,
	       "pdu->present != M3AP_M3AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M3AP_ProcedureCode_id_mBMSsessionStop,
	       "pdu->choice.successfulOutcome->procedureCode != M3AP_ProcedureCode_id_sessionStop\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M3AP_Criticality_reject,
	       "pdu->choice.successfulOutcome->criticality != M3AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M3AP_SuccessfulOutcome__value_PR_MBMSSessionStopResponse,
	       "pdu->choice.successfulOutcome->value.present != M3AP_SuccessfulOutcome__value_PR_MBMSSessionStopResponse\n");


  M3AP_MBMSSessionStopResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.MBMSSessionStopResponse;
  M3AP_MBMSSessionStopResponse_IEs_t  *ie;
  int MME_MBMS_M3AP_ID=-1;
  int MCE_MBMS_M3AP_ID=-1;


  MessageDef *msg_g = itti_alloc_new_message(TASK_M3AP_MME, 0,M3AP_MBMS_SESSION_STOP_RESP); //TODO

  LOG_D(M3AP, "M3AP: MBMSSessionStop-Resp: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
  for (int i=0;i < in->protocolIEs.list.count; i++) {
     ie = in->protocolIEs.list.array[i];
     switch (ie->id) {
     case M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID:
       AssertFatal(ie->criticality == M3AP_Criticality_reject,
                   "ie->criticality != M3AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M3AP_MBMSSessionStopResponse_IEs__value_PR_MME_MBMS_M3AP_ID,
                   "ie->value.present != M3AP_MBMSStopResponse_IEs__value_PR_MME_MBMS_M3AP_ID\n");
       MME_MBMS_M3AP_ID=ie->value.choice.MME_MBMS_M3AP_ID;
       LOG_D(M3AP, "M3AP: MBMSSessionStop-Resp: MME_MBMS_M3AP_ID %d\n",
             MME_MBMS_M3AP_ID);
       break;
      case M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID:
       AssertFatal(ie->criticality == M3AP_Criticality_reject,
                   "ie->criticality != M3AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M3AP_MBMSSessionStopResponse_IEs__value_PR_MCE_MBMS_M3AP_ID,
                   "ie->value.present != M3AP_MBMSStopResponse_IEs__value_PR_MCE_MBMS_M3AP_ID\n");
       MCE_MBMS_M3AP_ID=ie->value.choice.MCE_MBMS_M3AP_ID;
       LOG_D(M3AP, "M3AP: MBMSSessionStop-Resp: MCE_MBMS_M3AP_ID %d\n",
             MCE_MBMS_M3AP_ID);
       break;
     }
  }

  //AssertFatal(MME_MBMS_M3AP_ID!=-1,"MME_MBMS_M3AP_ID was not sent\n");
  //AssertFatal(MCE_MBMS_M3AP_ID!=-1,"MCE_MBMS_M3AP_ID was not sent\n");
 // M3AP_SESSION_STOP_RESP(msg_p).

 //  LOG_D(M3AP, "Sending M3AP_SESSION_START_RESP ITTI message to MCE_APP with assoc_id (%d->%d)\n",
 //        assoc_id,MCE_MODULE_ID_TO_INSTANCE(assoc_id));
   itti_send_msg_to_task(TASK_MME_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_g);
// //                      
	return 0;
}

int MME_handle_MBMS_SESSION_STOP_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
                      
}





/*
 * Reset
 */

int MME_send_RESET(instance_t instance, m3ap_reset_t * m3ap_reset) {
  AssertFatal(1==0,"Not implemented yet\n");
  //M3AP_Reset_t     Reset;
                        
}

int MME_handle_RESET_ACKKNOWLEDGE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");

}

int MME_handle_RESET(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int MME_send_RESET_ACKNOWLEDGE(instance_t instance, M3AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
 *    M3 Setup
 */
int MME_handle_M3_SETUP_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  LOG_D(M2AP, "MCE_handle_M3_SETUP_REQUEST assoc_id %d\n",assoc_id);

  //AssertFatal(1==0,"Not implemented yet\n");
//  LOG_W(M3AP, "MME_handle_M3_SETUP_REQUEST assoc_id %d\n",assoc_id);
//
    MessageDef                         *message_p;
//  M3AP_M3SetupRequest_t              *container;
//  M3AP_M3SetupRequestIEs_t           *ie;
//  int i = 0,j=0;
//
//  DevAssert(pdu != NULL);
//
//  container = &pdu->choice.initiatingMessage.value.choice.M3SetupRequest;
//
//  /* M3 Setup Request == Non UE-related procedure -> stream 0 */
//  if (stream != 0) {
//    LOG_W(M3AP, "[SCTP %d] Received m3 setup request on stream != 0 (%d)\n",
//              assoc_id, stream);
//  }
//
    message_p = itti_alloc_new_message(TASK_M3AP_MME, 0, M3AP_SETUP_REQ); 
    //printf("M3AP_SETUP_REQ(message_p).assoc_id %d\n",M3AP_SETUP_REQ(message_p).assoc_id);
//  
//  /* assoc_id */
    M3AP_SETUP_REQ(message_p).assoc_id = assoc_id;
//
// /* GlobalMCE_id */
// // this function exits if the ie is mandatory
//  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_M3SetupRequestIEs_t, ie, container,
//                             M3AP_ProtocolIE_ID_id_Global_MCE_ID, true);
//  asn_INTEGER2ulong(&ie->value.choice.GlobalMCE_ID, &M3AP_SETUP_REQ(message_p).GlobalMCE_ID);
//  LOG_W(M3AP, "M3AP_SETUP_REQ(message_p).GlobalMCE_ID %lu \n", M3AP_SETUP_REQ(message_p).GlobalMCE_ID);
//
//  /* MCE_name */
//  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_M3SetupRequestIEs_t, ie, container,
//                              M3AP_ProtocolIE_ID_id_MCEname, true);
//  M3AP_SETUP_REQ(message_p).MCEname = calloc(ie->value.choice.MCEname.size + 1, sizeof(char));
//  memcpy(M3AP_SETUP_REQ(message_p).MCEname, ie->value.choice.MCEname.buf,
//         ie->value.choice.MCEname.size);
//
//  /* Convert the mme name to a printable string */
//  M3AP_SETUP_REQ(message_p).MCEname[ie->value.choice.MCEname.size] = '\0';
//  LOG_W(M3AP, "M3AP_SETUP_REQ(message_p).gNB_DU_name %s \n", M3AP_SETUP_REQ(message_p).MCEname);
//   /* MCE_MBMS_Configuration_data_List */
//  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_M3SetupRequestIEs_t, ie, container,
//                              M3AP_ProtocolIE_ID_id_MCE_MBMS_Configuration_data_List, true);
//  M3AP_SETUP_REQ(message_p).num_mbms_available = ie->value.choice.MCE_MBMS_Configuration_data_List.list.count;
//  LOG_W(M3AP, "M3AP_SETUP_REQ(message_p).num_mbms_available %d \n",
//        M3AP_SETUP_REQ(message_p).num_mbms_available);
//  int num_mbms_available = M3AP_SETUP_REQ(message_p).num_mbms_available;
//  for (i=0; i<num_mbms_available; i++) {
//	 M3AP_MCE_MBMS_Configuration_data_Item_t *mbms_configuration_item_p;
//         mbms_configuration_item_p = &(((M3AP_MCE_MBMS_Configuration_data_ItemIEs_t *)ie->value.choice.MCE_MBMS_Configuration_data_List.list.array[i])->value.choice.MCE_MBMS_Configuration_data_Item);
//    /* eCGI */
//       //mbms_configuration_item_p->eCGI ... (M3AP_ECGI_t)
//
//    OCTET_STRING_TO_INT16(&(mbms_configuration_item_p->eCGI.pLMN_Identity),M3AP_SETUP_REQ(message_p).plmn_identity[i]);
//    //OCTET_STRING_TO_INT16(&(mbms_configuration_item_p->eCGI.eUTRANcellIdentifier),M3AP_SETUP_REQ(message_p).eutran_cell_identifier[i]);
//    /* mbsfnSynchronisationArea */
//       //mbms_configuration_item_p->mbsfnSynchronisationArea ... (M3AP_MBSFN_SynchronisationArea_ID_t)
//
//    M3AP_SETUP_REQ(message_p).mbsfn_synchronization_area[i]=mbms_configuration_item_p->mbsfnSynchronisationArea;
//    /* mbmsServiceAreaList */
//       //mbms_configuration_item_p->mbmsServiceAreaList ... (M3AP_MBMS_Service_Area_ID_List_t)
//   for(j=0;j<1;j++){
//   	//OCTET_STRING_TO_INT16(&(mbms_configuration_item_p->mbmsServiceAreaList.list.array[j]), M3AP_SETUP_REQ(message_p).service_area_id[i][j]);
//   }
//
// }
//    
////    /* tac */
////    OCTET_STRING_TO_INT16(&(served_celles_item_p->served_Cell_Information.fiveGS_TAC), M3AP_SETUP_REQ(message_p).tac[i]);
////    LOG_D(M3AP, "M3AP_SETUP_REQ(message_p).tac[%d] %d \n",
////          i, M3AP_SETUP_REQ(message_p).tac[i]);
////
////    /* - nRCGI */
////    TBCD_TO_MCC_MNC(&(served_celles_item_p->served_Cell_Information.nRCGI.pLMN_Identity), M3AP_SETUP_REQ(message_p).mcc[i],
////                    M3AP_SETUP_REQ(message_p).mnc[i],
////                    M3AP_SETUP_REQ(message_p).mnc_digit_length[i]);
////    
////    
////    // NR cellID
////    BIT_STRING_TO_NR_CELL_IDENTITY(&served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity,
////				   M3AP_SETUP_REQ(message_p).nr_cellid[i]);
////    LOG_D(M3AP, "[SCTP %d] Received nRCGI: MCC %d, MNC %d, CELL_ID %llu\n", assoc_id,
////          M3AP_SETUP_REQ(message_p).mcc[i],
////          M3AP_SETUP_REQ(message_p).mnc[i],
////          (long long unsigned int)M3AP_SETUP_REQ(message_p).nr_cellid[i]);
////    LOG_D(M3AP, "nr_cellId : %x %x %x %x %x\n",
////          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[0],
////          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[1],
////          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[2],
////          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[3],
////          served_celles_item_p->served_Cell_Information.nRCGI.nRCellIdentity.buf[4]);
////    /* - nRPCI */
////    M3AP_SETUP_REQ(message_p).nr_pci[i] = served_celles_item_p->served_Cell_Information.nRPCI;
////    LOG_D(M3AP, "M3AP_SETUP_REQ(message_p).nr_pci[%d] %d \n",
////          i, M3AP_SETUP_REQ(message_p).nr_pci[i]);
////  
////    // System Information
////    /* mib */
////    M3AP_SETUP_REQ(message_p).mib[i] = calloc(served_celles_item_p->gNB_DU_System_Information->mIB_message.size + 1, sizeof(char));
////    memcpy(M3AP_SETUP_REQ(message_p).mib[i], served_celles_item_p->gNB_DU_System_Information->mIB_message.buf,
////           served_celles_item_p->gNB_DU_System_Information->mIB_message.size);
////    /* Convert the mme name to a printable string */
////    M3AP_SETUP_REQ(message_p).mib[i][served_celles_item_p->gNB_DU_System_Information->mIB_message.size] = '\0';
////    M3AP_SETUP_REQ(message_p).mib_length[i] = served_celles_item_p->gNB_DU_System_Information->mIB_message.size;
////    LOG_D(M3AP, "M3AP_SETUP_REQ(message_p).mib[%d] %s , len = %d \n",
////          i, M3AP_SETUP_REQ(message_p).mib[i], M3AP_SETUP_REQ(message_p).mib_length[i]);
////
////    /* sib1 */
////    M3AP_SETUP_REQ(message_p).sib1[i] = calloc(served_celles_item_p->gNB_DU_System_Information->sIB1_message.size + 1, sizeof(char));
////    memcpy(M3AP_SETUP_REQ(message_p).sib1[i], served_celles_item_p->gNB_DU_System_Information->sIB1_message.buf,
////           served_celles_item_p->gNB_DU_System_Information->sIB1_message.size);
////    /* Convert the mme name to a printable string */
////    M3AP_SETUP_REQ(message_p).sib1[i][served_celles_item_p->gNB_DU_System_Information->sIB1_message.size] = '\0';
////    M3AP_SETUP_REQ(message_p).sib1_length[i] = served_celles_item_p->gNB_DU_System_Information->sIB1_message.size;
////    LOG_D(M3AP, "M3AP_SETUP_REQ(message_p).sib1[%d] %s , len = %d \n",
////          i, M3AP_SETUP_REQ(message_p).sib1[i], M3AP_SETUP_REQ(message_p).sib1_length[i]);
////  }
//
//
    //printf("m3ap_mme_data_from_mce->assoc_id %d %d\n",m3ap_mme_data_from_mce->assoc_id,assoc_id);
    *m3ap_mme_data_from_mce = M3AP_SETUP_REQ(message_p);
    //printf("m3ap_mme_data_from_mce->assoc_id %d %d\n",m3ap_mme_data_from_mce->assoc_id,assoc_id);
//
    itti_send_msg_to_task(TASK_MME_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);
//  if (num_mbms_available > 0) {
//    itti_send_msg_to_task(TASK_MME_APP, MCE_MODULE_ID_TO_INSTANCE(instance), message_p);
//  } else {
//       //MME_send_M3_SETUP_FAILURE(instance);
//       return -1;
//  }
////  return 0;
//    //TEST POINT MME -> eNB
//    if(1){
//	printf("instance %d\n",instance);
//	//MME_send_M3_SETUP_RESPONSE(instance,assoc_id,m3ap_mce_data_from_enb->assoc_id);
//	//MME_send_MBMS_SESSION_START_REQUEST(instance,assoc_id);
//	//MME_send_MBMS_SESSION_STOP_REQUEST(instance,assoc_id);
//	//MME_send_MBMS_SCHEDULING_INFORMATION(instance,assoc_id,NULL); //TODO
//    }
//    else 
//	MME_send_M3_SETUP_FAILURE(instance,assoc_id);
//
    return 0;
}

int MME_send_M3_SETUP_RESPONSE(instance_t instance, /*sctp_assoc_t assoc_id,*/
                               m3ap_setup_resp_t *m3ap_setup_resp)
{
 M3AP_M3AP_PDU_t           pdu;
 M3AP_M3SetupResponse_t    *out;
 M3AP_M3SetupResponseIEs_t *ie;

 uint8_t  *buffer;
 uint32_t  len;
 //int       i = 0;

 /* Create */
 /* 0. Message Type */
 memset(&pdu, 0, sizeof(pdu));
 pdu.present = M3AP_M3AP_PDU_PR_successfulOutcome;
 //pdu.choice.successfulOutcome = (M3AP_SuccessfulOutcome_t *)calloc(1, sizeof(M3AP_SuccessfulOutcome_t));
 pdu.choice.successfulOutcome.procedureCode = M3AP_ProcedureCode_id_m3Setup;
 pdu.choice.successfulOutcome.criticality   = M3AP_Criticality_reject;
 pdu.choice.successfulOutcome.value.present = M3AP_SuccessfulOutcome__value_PR_M3SetupResponse;
 out = &pdu.choice.successfulOutcome.value.choice.M3SetupResponse;

 /* mandatory */
  /* c4. CriticalityDiagnostics*/
  if (0) {
    ie = (M3AP_M3SetupResponseIEs_t *)calloc(1, sizeof(M3AP_M3SetupResponseIEs_t));
    ie->id                        = M3AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality               = M3AP_Criticality_ignore;
    ie->value.present             = M3AP_M3SetupResponseIEs__value_PR_CriticalityDiagnostics;
    ie->value.choice.CriticalityDiagnostics.procedureCode = (M3AP_ProcedureCode_t *)calloc(1, sizeof(M3AP_ProcedureCode_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCode = M3AP_ProcedureCode_id_m3Setup;
    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (M3AP_TriggeringMessage_t *)calloc(1, sizeof(M3AP_TriggeringMessage_t));
    *ie->value.choice.CriticalityDiagnostics.triggeringMessage = M3AP_TriggeringMessage_initiating_message;
    ie->value.choice.CriticalityDiagnostics.procedureCriticality = (M3AP_Criticality_t *)calloc(1, sizeof(M3AP_Criticality_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCriticality = M3AP_Criticality_reject;
    //ie->value.choice.CriticalityDiagnostics.transactionID = (M2AP_TransactionID_t *)calloc(1, sizeof(M3AP_TransactionID_t));
    //*ie->value.choice.CriticalityDiagnostics.transactionID = 0;
    asn1cSeqAdd(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (m3ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M3AP, "Failed to encode M3 response\n");
    return -1;
  }

  //printf(",m3ap_mme_data_from_mce->assoc_id %d\n",m3ap_mme_data_from_mce->assoc_id);

  m3ap_MME_itti_send_sctp_data_req(instance,m3ap_mme_data_from_mce->assoc_id,buffer,len,0);

  return 0;
}

int MME_send_M3_SETUP_FAILURE(instance_t instance,m3ap_setup_failure_t* m3ap_setup_failure) {
  AssertFatal(1==0,"Not implemented yet\n");
  
//  module_id_t enb_mod_idP;
//  module_id_t mce_mod_idP;
//
//  // This should be fixed
//  enb_mod_idP = (module_id_t)0;
//  mce_mod_idP  = (module_id_t)0;
//
//  M3AP_M3AP_PDU_t           pdu;
//  M3AP_M3SetupFailure_t    *out;
//  M3AP_M3SetupFailureIEs_t *ie;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//
//  /* Create */
//  /* 0. Message Type */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M3AP_M3AP_PDU_PR_unsuccessfulOutcome;
//  //pdu.choice.unsuccessfulOutcome = (M3AP_UnsuccessfulOutcome_t *)calloc(1, sizeof(M3AP_UnsuccessfulOutcome_t));
//  pdu.choice.unsuccessfulOutcome.procedureCode = M3AP_ProcedureCode_id_m3Setup;
//  pdu.choice.unsuccessfulOutcome.criticality   = M3AP_Criticality_reject;
//  pdu.choice.unsuccessfulOutcome.value.present = M3AP_UnsuccessfulOutcome__value_PR_M2SetupFailure;
//  out = &pdu.choice.unsuccessfulOutcome.value.choice.M2SetupFailure;
//
//  /* mandatory */
//  /* c1. Transaction ID (integer value)*/
// // ie = (M3AP_M3SetupFailure_Ies_t *)calloc(1, sizeof(M3AP_M3SetupFailure_Ies_t));
// // ie->id                        = M3AP_ProtocolIE_ID_id_GlobalMCE_ID;
// // ie->criticality               = M3AP_Criticality_reject;
// // ie->value.present             = M3AP_M3SetupFailure_Ies__value_PR_GlobalMCE_ID;
// // ie->value.choice.GlobalMCE_ID = M3AP_get_next_transaction_identifier(enb_mod_idP, mce_mod_idP);
// // asn1cSeqAdd(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  /* c2. Cause */
//  ie = (M3AP_M3SetupFailureIEs_t *)calloc(1, sizeof(M3AP_M3SetupFailureIEs_t));
//  ie->id                        = M3AP_ProtocolIE_ID_id_Cause;
//  ie->criticality               = M3AP_Criticality_ignore;
//  ie->value.present             = M3AP_M3SetupFailure_Ies__value_PR_Cause;
//  ie->value.choice.Cause.present = M3AP_Cause_PR_radioNetwork;
//  ie->value.choice.Cause.choice.radioNetwork = M3AP_CauseRadioNetwork_unspecified;
//  asn1cSeqAdd(&out->protocolIEs.list, ie);
//
//  /* optional */
//  /* c3. TimeToWait */
//  if (0) {
//    ie = (M3AP_M3SetupFailureIEs_t *)calloc(1, sizeof(M3AP_M3SetupFailureIEs_t));
//    ie->id                        = M3AP_ProtocolIE_ID_id_TimeToWait;
//    ie->criticality               = M3AP_Criticality_ignore;
//    ie->value.present             = M3AP_M3SetupFailure_Ies__value_PR_TimeToWait;
//    ie->value.choice.TimeToWait = M3AP_TimeToWait_v10s;
//    asn1cSeqAdd(&out->protocolIEs.list, ie);
//  }
//
//  /* optional */
//  /* c4. CriticalityDiagnostics*/
//  if (0) {
//    ie = (M3AP_M3SetupFailureIEs_t *)calloc(1, sizeof(M3AP_M3SetupFailureIEs_t));
//    ie->id                        = M3AP_ProtocolIE_ID_id_CriticalityDiagnostics;
//    ie->criticality               = M3AP_Criticality_ignore;
//    ie->value.present             = M3AP_M3SetupFailure_Ies__value_PR_CriticalityDiagnostics;
//    ie->value.choice.CriticalityDiagnostics.procedureCode = (M3AP_ProcedureCode_t *)calloc(1, sizeof(M3AP_ProcedureCode_t));
//    *ie->value.choice.CriticalityDiagnostics.procedureCode = M3AP_ProcedureCode_id_m3Setup;
//    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (M3AP_TriggeringMessage_t *)calloc(1, sizeof(M3AP_TriggeringMessage_t));
//    *ie->value.choice.CriticalityDiagnostics.triggeringMessage = M3AP_TriggeringMessage_initiating_message;
//    ie->value.choice.CriticalityDiagnostics.procedureCriticality = (M3AP_Criticality_t *)calloc(1, sizeof(M3AP_Criticality_t));
//    *ie->value.choice.CriticalityDiagnostics.procedureCriticality = M3AP_Criticality_reject;
//    //ie->value.choice.CriticalityDiagnostics.transactionID = (M3AP_TransactionID_t *)calloc(1, sizeof(M3AP_TransactionID_t));
//    //*ie->value.choice.CriticalityDiagnostics.transactionID = 0;
//    asn1cSeqAdd(&out->protocolIEs.list, ie);
//  }
//
//  /* encode */
//  if (m3ap_encode_pdu(&pdu, &buffer, &len) < 0) {
//    LOG_E(M3AP, "Failed to encode M2 setup request\n");
//    return -1;
//  }
//
//  //mce_m3ap_itti_send_sctp_data_req(instance, m3ap_mce_data_from_enb->assoc_id, buffer, len, 0);
//  m3ap_MME_itti_send_sctp_data_req(instance,m3ap_mme_data_from_mce->assoc_id,buffer,len,0);

  return 0;
}

/*
 * MME Configuration Update
 */

int MME_handle_MME_CONFIGURATION_UPDATE_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int MME_handle_MME_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                                    sctp_assoc_t assoc_id,
                                                    uint32_t stream,
                                                    M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
 * MCE Configuration Update
 */

int MME_handle_MCE_CONFIGURATION_UPDATE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}






/*
 * Error Indication
 */

int MME_handle_ERROR_INDICATION(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");


}

int MME_send_ERROR_INDICATION(instance_t instance, M3AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}




/*
 * Session Update Request
 */
int MME_send_MBMS_SESSION_UPDATE_REQUEST(instance_t instance, m3ap_mbms_session_update_req_t * m3ap_mbms_session_update_req){
  M3AP_M3AP_PDU_t          pdu; 
  M3AP_MBMSSessionUpdateRequest_t        *out;
  M3AP_MBMSSessionUpdateRequest_IEs_t    *ie;

  uint8_t *buffer;
  uint32_t len;
  //int	   i=0; 
  //int 	   j=0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M3AP_InitiatingMessage_t *)calloc(1, sizeof(M3AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M3AP_ProcedureCode_id_mBMSsessionUpdate;
  pdu.choice.initiatingMessage.criticality   = M3AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M3AP_InitiatingMessage__value_PR_MBMSSessionUpdateRequest;
  out = &pdu.choice.initiatingMessage.value.choice.MBMSSessionUpdateRequest; 

  /* mandatory */
  /* c1. MME_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MME_MBMS_M3AP_ID;
  //ie->value.choice.MME_MBMS_M3AP_ID = 0;
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. MCE_MBMS_M3AP_ID (integer value) */ //long
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MCE_MBMS_M3AP_ID;
  //ie->value.choice.MCE_MBMS_M3AP_ID = 0;

  asn1cSeqAdd(&out->protocolIEs.list, ie);

   /* mandatory */
  /* c2. TMGI */ 
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_TMGI; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_TMGI;
  M3AP_TMGI_t	*tmgi 		= &ie->value.choice.TMGI;
  {
	  MCC_MNC_TO_PLMNID(0,0,3,&tmgi->pLMNidentity);
	  uint8_t TMGI[5] = {4,3,2,1,0};
	  OCTET_STRING_fromBuf(&tmgi->serviceID,(const char*)&TMGI[2],3);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* optional */
  /* c2. MBMS_Session_ID */
  if(0){
	  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Session_ID;
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MBMS_Session_ID;
	  //M3AP_MBMS_Session_ID_t * mbms_session_id = &ie->value.choice.MBMS_Session_ID;

	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }
  /* mandatory */
  /* c2. MBMS_E_RAB_QoS_Parameters */ 
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_E_RAB_QoS_Parameters; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MBMS_E_RAB_QoS_Parameters;
  M3AP_MBMS_E_RAB_QoS_Parameters_t * mbms_e_rab_qos_parameters = &ie->value.choice.MBMS_E_RAB_QoS_Parameters;
  {
	//M3AP_QCI_t * qci = &mbms_e_rab_qos_parameters->qCI; //long
	mbms_e_rab_qos_parameters->qCI=1;
	//struct M3AP_GBR_QosInformation  *gbrQosInformation;     /* OPTIONAL */
  }

  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c2. MBMS_Session_Duration */ 
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Session_Duration; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MBMS_Session_Duration; 
  M3AP_MBMS_Session_Duration_t * mbms_session_duration = &ie->value.choice.MBMS_Session_Duration;
  {
	uint8_t duration[5] = {4,3,2,1,0};
	OCTET_STRING_fromBuf(mbms_session_duration,(const char*)&duration[2],3);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* optional */
  /* c2 MBMS_Service_Area */
  if(0){
	  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Service_Area;
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MBMS_Service_Area;
	  //M3AP_MBMS_Service_Area_t * mbms_service_area = &ie->value.choice.MBMS_Service_Area;

	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }
  /* mandatory */
  /* c2 MinimumTimeToMBMSDataTransfer */
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id                        = M3AP_ProtocolIE_ID_id_MinimumTimeToMBMSDataTransfer; 
  ie->criticality               = M3AP_Criticality_reject;
  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MinimumTimeToMBMSDataTransfer; 
  M3AP_MinimumTimeToMBMSDataTransfer_t * minimumtimetombmsdatatransfer = &ie->value.choice.MinimumTimeToMBMSDataTransfer;
  {
	uint8_t duration[5] = {4,3,2,1,0};
	OCTET_STRING_fromBuf(minimumtimetombmsdatatransfer,(const char*)&duration[2],1);
  }

  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* optional */
  /* c2 TNL_Information */
  if(0){
	  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_TNL_Information; 
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_TNL_Information; 
	  M3AP_TNL_Information_t * tnl_information = &ie->value.choice.TNL_Information;
	  {
		OCTET_STRING_fromBuf(&tnl_information->iPMCAddress,"1204",strlen("1234"));
		OCTET_STRING_fromBuf(&tnl_information->iPSourceAddress,"1204",strlen("1234"));
		OCTET_STRING_fromBuf(&tnl_information->gTP_DLTEID,"1204",strlen("1234"));
	  }
	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }
  /* optional */
  /* c2 Time_ofMBMS_DataTransfer */
  if(0){
	  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_Time_ofMBMS_DataTransfer; 
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_Absolute_Time_ofMBMS_Data; 
	  //M3AP_Absolute_Time_ofMBMS_Data_t * absolute_time_ofmbms_data	= &ie->value.choice.Absolute_Time_ofMBMS_Data;
	  //{
		//char duration[5] = {4,3,2,1,0};
		//OCTET_STRING_fromBuf(absolute_time_ofmbms_data,(const char*)&duration[2],3);
	  //}
	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }
  /* optional */
  /* c2 MBMS_Cell_List */
  if(0){
	  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
	  ie->id                        = M3AP_ProtocolIE_ID_id_MBMS_Cell_List; 
	  ie->criticality               = M3AP_Criticality_ignore;
	  ie->value.present             = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MBMS_Cell_List; 
	  M3AP_MBMS_Cell_List_t * mbms_cell_list = &ie->value.choice.MBMS_Cell_List;
	  M3AP_ECGI_t * ecgi = (M3AP_ECGI_t*)calloc(1,sizeof(M3AP_ECGI_t));	
	  {
		//MCC_MNC_TO_PLMNID(0,0,3,ecgi->pLMN_Identity);
		//MACRO_ENB_ID_TO_CELL_IDENTITY(1,0,ecgi->eUTRANcellIdentifier);
		asn1cSeqAdd(&mbms_cell_list->list,ecgi);
	  }
	  asn1cSeqAdd(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (m3ap_encode_pdu(&pdu,&buffer,&len) < 0){
        return -1;
  }

  //MME_m3ap_itti_send_sctp_data_req(instance, m3ap_mce_data_from_mce->assoid,buffer,len,0); 
  m3ap_MME_itti_send_sctp_data_req(instance, m3ap_mme_data_from_mce->assoc_id,buffer,len,0);
  return 0;



}

int MME_handle_MBMS_SESSION_UPDATE_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  //AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M3AP, "MME_handle_MBMS_SESSION_UPDATE_RESPONSE\n");

   AssertFatal(pdu->present == M3AP_M3AP_PDU_PR_successfulOutcome,
               "pdu->present != M3AP_M3AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M3AP_ProcedureCode_id_mBMSsessionUpdate,
               "pdu->choice.successfulOutcome->procedureCode != M3AP_ProcedureCode_id_sessionUpdate\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M3AP_Criticality_reject,
               "pdu->choice.successfulOutcome->criticality != M3AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M3AP_SuccessfulOutcome__value_PR_MBMSSessionUpdateResponse,
               "pdu->choice.successfulOutcome.value.present != M3AP_SuccessfulOutcome__value_PR_MBMSSessionUpdateResponse\n");


  M3AP_MBMSSessionUpdateResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.MBMSSessionUpdateResponse;
  M3AP_MBMSSessionUpdateResponse_IEs_t  *ie;
  int MME_MBMS_M3AP_ID=-1;
  int MCE_MBMS_M3AP_ID=-1;


  MessageDef *msg_g = itti_alloc_new_message(TASK_M3AP_MME, 0,M3AP_MBMS_SESSION_UPDATE_RESP); //TODO

  LOG_D(M3AP, "M3AP: SessionUpdate-Resp: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
  for (int i=0;i < in->protocolIEs.list.count; i++) {
     ie = in->protocolIEs.list.array[i];
     switch (ie->id) {
     case M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID:
       AssertFatal(ie->criticality == M3AP_Criticality_reject,
                   "ie->criticality != M3AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M3AP_MBMSSessionUpdateResponse_IEs__value_PR_MME_MBMS_M3AP_ID,
                   "ie->value.present != M3AP_sessionUpdateResponse_IEs__value_PR_MME_MBMS_M3AP_ID\n");
       MME_MBMS_M3AP_ID=ie->value.choice.MME_MBMS_M3AP_ID;
       LOG_D(M3AP, "M3AP: SessionUpdate-Resp: MME_MBMS_M3AP_ID %d\n",
             MME_MBMS_M3AP_ID);
       break;
      case M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID:
       AssertFatal(ie->criticality == M3AP_Criticality_reject,
                   "ie->criticality != M3AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M3AP_MBMSSessionUpdateResponse_IEs__value_PR_MCE_MBMS_M3AP_ID,
                   "ie->value.present != M3AP_sessionUpdateResponse_IEs__value_PR_MCE_MBMS_M3AP_ID\n");
       MCE_MBMS_M3AP_ID=ie->value.choice.MCE_MBMS_M3AP_ID;
       LOG_D(M3AP, "M3AP: SessionUpdate-Resp: MCE_MBMS_M3AP_ID %d\n",
             MCE_MBMS_M3AP_ID);
       break;
     }
  }

  //AssertFatal(MME_MBMS_M3AP_ID!=-1,"MME_MBMS_M3AP_ID was not sent\n");
  //AssertFatal(MCE_MBMS_M3AP_ID!=-1,"MCE_MBMS_M3AP_ID was not sent\n");
  ////M3AP_SESSION_START_RESP(msg_p).
  // LOG_W(M3AP, "Sending M3AP_SESSION_START_RESP ITTI message to MCE_APP with assoc_id (%d->%d)\n",
  //       assoc_id,ENB_MODULE_ID_TO_INSTANCE(assoc_id));
  itti_send_msg_to_task(TASK_MME_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_g);

  return 0;

}

int MME_handle_MBMS_SESSION_UPDATE_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int MME_handle_MBMS_SERVICE_COUNTING_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int MME_handle_MBMS_SESSION_COUNTING_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");

}


/*
 * Service Counting Results Report
 */

int MME_handle_MBMS_SESSION_COUNTING_RESULTS_REPORT(instance_t instance,
                                                    sctp_assoc_t assoc_id,
                                                    uint32_t stream,
                                                    M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");

}


/*
 * Overload Notification
 */
int MME_handle_MBMS_OVERLOAD_NOTIFICATION(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
 
}




