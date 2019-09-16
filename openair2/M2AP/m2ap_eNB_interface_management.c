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
 * \company Vicomtech, Spain
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#include "intertask_interface.h"

#include "m2ap_common.h"
#include "m2ap_eNB.h"
#include "m2ap_eNB_generate_messages.h"
#include "m2ap_encoder.h"
#include "m2ap_decoder.h"
#include "m2ap_ids.h"

#include "m2ap_eNB_interface_management.h"


#include "m2ap_itti_messaging.h"

#include "msc.h"
#include "assertions.h"
#include "conversions.h"

#include "M2AP_MBSFN-Area-Configuration-List.h" 

//#include "m2ap_common.h"
//#include "m2ap_encoder.h"
//#include "m2ap_decoder.h"
//#include "m2ap_itti_messaging.h"
//#include "m2ap_eNB_interface_management.h"
//#include "assertions.h"

extern m2ap_setup_req_t *m2ap_enb_data_g;


//extern m2ap_setup_req_t *m2ap_mce_data_from_enb;
int eNB_handle_MBMS_SCHEDULING_INFORMATION(instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        M2AP_M2AP_PDU_t *pdu){
  LOG_D(M2AP, "eNB_handle_MBMS_SCHEDULING_INFORMATION assoc_id %d\n",assoc_id);

  MessageDef                         *message_p/*,*message_p2*/;
  M2AP_MbmsSchedulingInformation_t *container;
  M2AP_MbmsSchedulingInformation_Ies_t   *ie;
  int i = 0;
  int j = 0;
  int k = 0;
  //int m = 0;

  DevAssert(pdu != NULL);

  container = &pdu->choice.initiatingMessage.value.choice.MbmsSchedulingInformation;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS scheduling information on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_MBMS_SCHEDULING_INFORMATION);
  //message_p2  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_MBMS_SCHEDULING_INFORMATION);





  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_MbmsSchedulingInformation_Ies_t, ie, container,M2AP_ProtocolIE_ID_id_MCCH_Update_Time ,true);
  //printf("id %d\n",ie->id);

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_MbmsSchedulingInformation_Ies_t, ie, container,M2AP_ProtocolIE_ID_id_MBSFN_Area_Configuration_List ,true);

  if(ie){

	  //printf("id %d\n",ie->id);
	  //printf("MBSFN_Area_Configuration_List  %p\n",ie->value.choice.MBSFN_Area_Configuration_List.list.array);
	  /*M2AP_MBSFN_Area_Configuration_Item_t  * kk  = &ie->value.choice.MBSFN_Area_Configuration_List.list.array[0];
	  printf("M2AP_MBSFN_Area_Configuration_Item %p\n",kk);
	  printf("M2AP_MBSFN_Area_Configuration_Item %d\n",kk->id);*/

	  const asn_anonymous_sequence_ *list = _A_CSEQUENCE_FROM_VOID((void*)&ie->value.choice.MBSFN_Area_Configuration_List);
	  if(list->count > 0 ){
		M2AP_MBMS_SCHEDULING_INFORMATION(message_p).num_mbms_area_config_list = list->count;
	  }
	  for(i=0; i < list->count; i++ ){
		  void * memb_ptr = list->array[i];
		  //printf("%p %d\n", memb_ptr,list->count);
		  const asn_anonymous_sequence_ *list1 = _A_CSEQUENCE_FROM_VOID((void*)memb_ptr);
		  void * memb_ptr1 = list1->array[0];
		  //printf("%p %d\n", memb_ptr1,list1->count);
		  void * memb_ptr2 = list1->array[1];
		  void * memb_ptr3 = list1->array[2];
		  void * memb_ptr4 = list1->array[3];

		  //printf("%lu\n", ((M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr1)->id);
		  M2AP_MBSFN_Area_Configuration_Item_t * m2ap_mbsfn_area_configuration_item = (M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr1;
		  //printf("count %d\n",m2ap_mbsfn_area_configuration_item->value.choice.PMCH_Configuration_List.list.count);
		  if(m2ap_mbsfn_area_configuration_item->value.choice.PMCH_Configuration_List.list.count > 0){
			M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].num_pmch_config_list = m2ap_mbsfn_area_configuration_item->value.choice.PMCH_Configuration_List.list.count;
		  }
		  for(j=0; j < m2ap_mbsfn_area_configuration_item->value.choice.PMCH_Configuration_List.list.count; j++){
			  M2AP_PMCH_Configuration_Item_t * m2ap_pmchconfiguration_item =&(((M2AP_PMCH_Configuration_ItemIEs_t*)m2ap_mbsfn_area_configuration_item->value.choice.PMCH_Configuration_List.list.array[j])->value.choice.PMCH_Configuration_Item);
			  M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].data_mcs = m2ap_pmchconfiguration_item->pmch_Configuration.dataMCS;
			  M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mch_scheduling_period = m2ap_pmchconfiguration_item->pmch_Configuration.mchSchedulingPeriod;
			  M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].allocated_sf_end = m2ap_pmchconfiguration_item->pmch_Configuration.allocatedSubframesEnd;
			  //printf("dataMCS %lu\n",m2ap_pmchconfiguration_item->pmch_Configuration.dataMCS);
			  //printf("allocatedSubframesEnd %lu\n",m2ap_pmchconfiguration_item->pmch_Configuration.allocatedSubframesEnd);
			  if(m2ap_pmchconfiguration_item->mbms_Session_List.list.count > 0){
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].num_mbms_session_list = m2ap_pmchconfiguration_item->mbms_Session_List.list.count;
			  }
			  for(k=0; k < m2ap_pmchconfiguration_item->mbms_Session_List.list.count; k++){
				//long mnc,mcc,mnc_length;
				PLMNID_TO_MCC_MNC(&m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.pLMNidentity,
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mcc,
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mnc,
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mnc_length);
				//char buf[4];
				
				//BUFFER_TO_INT32(buf,);
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id = ((m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.serviceID.buf[0]<<16) | (m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.serviceID.buf[1]<<8) | (m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.serviceID.buf[2])); //
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid = m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->lcid; //*/
				//LOG_E(M2AP,"buf[0]:%d buf[1]:%d buf[2]:%d\n",m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.serviceID.buf[0],m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.serviceID.buf[1],m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.serviceID.buf[2]);
			  }
		  }

		  //printf("%lu\n", ((M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr2)->id);
		  M2AP_MBSFN_Area_Configuration_Item_t * m2ap_mbsfn_area_configuration_item2 = (M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr2;
		  //printf("count %d\n",m2ap_mbsfn_area_configuration_item2->value.choice.MBSFN_Subframe_ConfigurationList.list.count);
		  if(m2ap_mbsfn_area_configuration_item2->value.choice.MBSFN_Subframe_ConfigurationList.list.count > 0){
			M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].num_mbms_sf_config_list = m2ap_mbsfn_area_configuration_item2->value.choice.MBSFN_Subframe_ConfigurationList.list.count;
		  }
		  for(j=0; j < m2ap_mbsfn_area_configuration_item2->value.choice.MBSFN_Subframe_ConfigurationList.list.count; j++){
			  M2AP_MBSFN_Subframe_Configuration_t * m2ap_mbsfn_sf_configuration = &(((M2AP_MBSFN_Subframe_ConfigurationItem_t*)m2ap_mbsfn_area_configuration_item2->value.choice.MBSFN_Subframe_ConfigurationList.list.array[j])->value.choice.MBSFN_Subframe_Configuration);
			  //printf("radioframe_allocation_period %lu\n",m2ap_mbsfn_sf_configuration->radioframeAllocationPeriod);
			  //printf("radioframe_allocation_offset %lu\n",m2ap_mbsfn_sf_configuration->radioframeAllocationOffset);
			  M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].mbms_sf_config_list[j].radioframe_allocation_period = m2ap_mbsfn_sf_configuration->radioframeAllocationPeriod;
			  M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].mbms_sf_config_list[j].radioframe_allocation_offset = m2ap_mbsfn_sf_configuration->radioframeAllocationOffset;
			  if( m2ap_mbsfn_sf_configuration->subframeAllocation.present == M2AP_MBSFN_Subframe_Configuration__subframeAllocation_PR_fourFrames ) {
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation = m2ap_mbsfn_sf_configuration->subframeAllocation.choice.oneFrame.buf[0] | (m2ap_mbsfn_sf_configuration->subframeAllocation.choice.oneFrame.buf[1]<<8) | (m2ap_mbsfn_sf_configuration->subframeAllocation.choice.oneFrame.buf[0]<<16);
			  }else{
				M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation = (m2ap_mbsfn_sf_configuration->subframeAllocation.choice.oneFrame.buf[0] >> 2) & 0x3F;
			  }
                  }
		

		  //printf("%lu\n", ((M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr3)->id);
		  M2AP_MBSFN_Area_Configuration_Item_t * m2ap_mbsfn_area_configuration_item3 = (M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr3;
		  //printf("count %d\n",m2ap_mbsfn_area_configuration_item2->value.choice.MBSFN_Subframe_ConfigurationList.list.count);
		  //printf("Common_Subframe_Allocation_Period %lu\n",m2ap_mbsfn_area_configuration_item3->value.choice.Common_Subframe_Allocation_Period);
		  M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].common_sf_allocation_period = m2ap_mbsfn_area_configuration_item3->value.choice.Common_Subframe_Allocation_Period;

		  //printf("%lu\n", ((M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr4)->id);
		  M2AP_MBSFN_Area_Configuration_Item_t * m2ap_mbsfn_area_configuration_item4 = (M2AP_MBSFN_Area_Configuration_Item_t*)memb_ptr4;
		  //printf("MBMS_Area_ID %lu\n",m2ap_mbsfn_area_configuration_item4->value.choice.MBSFN_Area_ID);
		  M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].mbms_area_id = m2ap_mbsfn_area_configuration_item4->value.choice.MBSFN_Area_ID;
	  } 
	

	  //const asn_anonymous_sequence_ *list3 = _A_CSEQUENCE_FROM_VOID((void*)memb_ptr2);
	  //void * memb_ptr3 = list3->array[0];
	  //printf("%p\n", memb_ptr3);




	  //xer_fprint(stdout, &asn_DEF_M2AP_MBSFN_Area_Configuration_List,  &ie->value.choice.MBSFN_Area_Configuration_List);
	
  }
  
  itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);
  return 0;
  
}
int eNB_send_MBMS_SCHEDULING_INFORMATION_RESPONSE(instance_t instance, m2ap_mbms_scheduling_information_resp_t * m2ap_mbms_scheduling_information_resp){
//	module_id_t mce_mod_idP;
  //module_id_t enb_mod_idP;

  // This should be fixed
 // enb_mod_idP = (module_id_t)0;
 // mce_mod_idP  = (module_id_t)0;

  M2AP_M2AP_PDU_t           pdu;
  //M2AP_MbmsSchedulingInformationResponse_t    *out;
  //M2AP_MbmsSchedulingInformationResponse_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
  //pdu.choice.successfulOutcome = (M2AP_SuccessfulOutcome_t *)calloc(1, sizeof(M2AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_mbmsSchedulingInformation;
  pdu.choice.successfulOutcome.criticality   = M2AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_MbmsSchedulingInformationResponse;
  //out = &pdu.choice.successfulOutcome.value.choice.MbmsSchedulingInformationResponse;
  

//  /* mandatory */
//  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
//  ie = (M2AP_MbmsSchedulingInformationResponse_Ies_t *)calloc(1, sizeof(M2AP_MbmsSchedulingInformationResponse_Ies_t));
//  ie->id                        = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
//  ie->criticality               = M2AP_Criticality_reject;
//  ie->value.present             = M2AP_MbmsSchedulingInformationResponse_Ies__value_PR_MCE_MBMS_M2AP_ID;
//  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
// /* mandatory */
//  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
//  ie = (M2AP_SessionStartResponse_Ies_t *)calloc(1, sizeof(M2AP_SessionStartResponse_Ies_t));
//  ie->id                        = M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID;
//  ie->criticality               = M2AP_Criticality_reject;
//  ie->value.present             = M2AP_MbmsSchedulingInformationResponse_Ies__value_PR_ENB_MBMS_M2AP_ID;
//  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//

  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M2AP, "Failed to encode M2 SessionStart Response\n");
    return -1;
  }


  LOG_D(M2AP,"pdu.present %d\n",pdu.present);
 // MSC_LOG_TX_MESSAGE(
 // MSC_M2AP_eNB,
 // MSC_M2AP_MCE,
 // (const char *)buffer,
 // len,
 // MSC_AS_TIME_FMT" M2_SETUP_REQUEST initiatingMessage MCEname %s",
 // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 // m2ap_eNB_data_p->ENBname);

  m2ap_eNB_itti_send_sctp_data_req(instance, m2ap_enb_data_g->assoc_id, buffer, len, 0);
  return 0;
}




int eNB_handle_MBMS_SESSION_START_REQUEST(instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        M2AP_M2AP_PDU_t *pdu){
  LOG_D(M2AP, "eNB_handle_MBMS_SESSION_START_REQUEST assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_SessionStartRequest_t              *container;
  //M2AP_SessionStartRequest_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.SessionStartRequest;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_MBMS_SESSION_START_REQ);


  itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);


//  if(1){
//	eNB_send_MBMS_SESSION_START_RESPONSE(instance,NULL);
//  }else
//	eNB_send_MBMS_SESSION_START_FAILURE(instance,NULL);
  return 0;
  
}

int eNB_send_MBMS_SESSION_START_RESPONSE(instance_t instance, m2ap_session_start_resp_t * m2ap_session_start_resp){
//	module_id_t mce_mod_idP;
//  module_id_t enb_mod_idP;

  // This should be fixed
//  enb_mod_idP = (module_id_t)0;
//  mce_mod_idP  = (module_id_t)0;

  M2AP_M2AP_PDU_t           pdu;
  M2AP_SessionStartResponse_t    *out;
  M2AP_SessionStartResponse_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
  //pdu.choice.successfulOutcome = (M2AP_SuccessfulOutcome_t *)calloc(1, sizeof(M2AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_sessionStart;
  pdu.choice.successfulOutcome.criticality   = M2AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_SessionStartResponse;
  out = &pdu.choice.successfulOutcome.value.choice.SessionStartResponse;
  

  /* mandatory */
  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStartResponse_Ies_t *)calloc(1, sizeof(M2AP_SessionStartResponse_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStartResponse_Ies__value_PR_MCE_MBMS_M2AP_ID;
  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

 /* mandatory */
  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStartResponse_Ies_t *)calloc(1, sizeof(M2AP_SessionStartResponse_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStartResponse_Ies__value_PR_ENB_MBMS_M2AP_ID;
  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M2AP, "Failed to encode M2 SessionStart Response\n");
    return -1;
  }


  LOG_D(M2AP,"pdu.present %d\n",pdu.present);
 // MSC_LOG_TX_MESSAGE(
 // MSC_M2AP_eNB,
 // MSC_M2AP_MCE,
 // (const char *)buffer,
 // len,
 // MSC_AS_TIME_FMT" M2_SETUP_REQUEST initiatingMessage MCEname %s",
 // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 // m2ap_eNB_data_p->ENBname);

  m2ap_eNB_itti_send_sctp_data_req(instance, m2ap_enb_data_g->assoc_id, buffer, len, 0);
  return 0;
}


int eNB_send_MBMS_SESSION_START_FAILURE(instance_t instance, m2ap_session_start_failure_t * m2ap_session_start_failure){
  //module_id_t enb_mod_idP;
  //module_id_t mce_mod_idP;

  // This should be fixed
  //enb_mod_idP = (module_id_t)0;
 // mce_mod_idP  = (module_id_t)0;

  M2AP_M2AP_PDU_t           pdu;
  M2AP_SessionStartFailure_t    *out;
  M2AP_SessionStartFailure_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_unsuccessfulOutcome;
  //pdu.choice.unsuccessfulOutcome = (M2AP_UnsuccessfulOutcome_t *)calloc(1, sizeof(M2AP_UnsuccessfulOutcome_t));
  pdu.choice.unsuccessfulOutcome.procedureCode = M2AP_ProcedureCode_id_sessionStart;
  pdu.choice.unsuccessfulOutcome.criticality   = M2AP_Criticality_reject;
  pdu.choice.unsuccessfulOutcome.value.present = M2AP_UnsuccessfulOutcome__value_PR_SessionStartFailure;
  out = &pdu.choice.unsuccessfulOutcome.value.choice.SessionStartFailure;

  /* mandatory */
  /* c1. Transaction ID (integer value)*/
 // ie = (M2AP_M2SetupFailure_Ies_t *)calloc(1, sizeof(M2AP_M2SetupFailure_Ies_t));
 // ie->id                        = M2AP_ProtocolIE_ID_id_GlobalENB_ID;
 // ie->criticality               = M2AP_Criticality_reject;
 // ie->value.present             = M2AP_M2SetupFailure_Ies__value_PR_GlobalENB_ID;
 // ie->value.choice.GlobalENB_ID = M2AP_get_next_transaction_identifier(enb_mod_idP, mce_mod_idP);
 // ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

 /* mandatory */
  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStartFailure_Ies_t *)calloc(1, sizeof(M2AP_SessionStartFailure_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStartFailure_Ies__value_PR_MCE_MBMS_M2AP_ID;
  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);



  /* mandatory */
  /* c2. Cause */
  ie = (M2AP_SessionStartFailure_Ies_t *)calloc(1, sizeof(M2AP_SessionStartFailure_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_Cause;
  ie->criticality               = M2AP_Criticality_ignore;
  ie->value.present             = M2AP_SessionStartFailure_Ies__value_PR_Cause;
  ie->value.choice.Cause.present = M2AP_Cause_PR_radioNetwork;
  ie->value.choice.Cause.choice.radioNetwork = M2AP_CauseRadioNetwork_unspecified;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


     /* encode */
  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M2AP, "Failed to encode M2 setup request\n");
    return -1;
  }

  //mce_m2ap_itti_send_sctp_data_req(instance, m2ap_mce_data_from_enb->assoc_id, buffer, len, 0);
  m2ap_eNB_itti_send_sctp_data_req(instance,m2ap_enb_data_g->assoc_id,buffer,len,0);


  return 0;
}

int eNB_handle_MBMS_SESSION_STOP_REQUEST(instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        M2AP_M2AP_PDU_t *pdu){
  LOG_D(M2AP, "eNB_handle_MBMS_SESSION_STOP_REQUEST assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_SessionStopRequest_t              *container;
  //M2AP_SessionStopRequest_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.SessionStopRequest;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_MBMS_SESSION_STOP_REQ);


  itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);




//  if(1){
//	eNB_send_MBMS_SESSION_STOP_RESPONSE(instance,NULL);
//  }else
//	eNB_send_MBMS_SESSION_STOP_FAILURE(instance,NULL);
  return 0;
  
}
int eNB_send_MBMS_SESSION_STOP_RESPONSE(instance_t instance, m2ap_session_stop_resp_t * m2ap_session_stop_resp){
	//module_id_t mce_mod_idP;
  //module_id_t enb_mod_idP;

  // This should be fixed
  //enb_mod_idP = (module_id_t)0;
  //mce_mod_idP  = (module_id_t)0;

  M2AP_M2AP_PDU_t           pdu;
  M2AP_SessionStopResponse_t    *out;
  M2AP_SessionStopResponse_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
  //pdu.choice.successfulOutcome = (M2AP_SuccessfulOutcome_t *)calloc(1, sizeof(M2AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_sessionStop;
  pdu.choice.successfulOutcome.criticality   = M2AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_SessionStopResponse;
  out = &pdu.choice.successfulOutcome.value.choice.SessionStopResponse;
  

  /* mandatory */
  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStopResponse_Ies_t *)calloc(1, sizeof(M2AP_SessionStopResponse_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStopResponse_Ies__value_PR_MCE_MBMS_M2AP_ID;
  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

 /* mandatory */
  /* c1. MCE_MBMS_M2AP_ID (integer value) */ //long
  ie = (M2AP_SessionStopResponse_Ies_t *)calloc(1, sizeof(M2AP_SessionStopResponse_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_SessionStopResponse_Ies__value_PR_ENB_MBMS_M2AP_ID;
  //ie->value.choice.MCE_MBMS_M2AP_ID = /*F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);*/ //?
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(M2AP, "Failed to encode M2 SessionStop Response\n");
    return -1;
  }


  LOG_D(M2AP,"pdu.present %d\n",pdu.present);
 // MSC_LOG_TX_MESSAGE(
 // MSC_M2AP_eNB,
 // MSC_M2AP_MCE,
 // (const char *)buffer,
 // len,
 // MSC_AS_TIME_FMT" M2_SETUP_REQUEST initiatingMessage MCEname %s",
 // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 // m2ap_eNB_data_p->ENBname);

  m2ap_eNB_itti_send_sctp_data_req(instance, m2ap_enb_data_g->assoc_id, buffer, len, 0);
  return 0;
}

 uint8_t bytes [] = {0x00, 0x05, /* .....+.. */
0x00, 0x24, 0x00, 0x00, 0x02, 0x00, 0x0d, 0x00, /* .$...... */
0x08, 0x00, 0x02, 0xf8, 0x39, 0x00, 0x00, 0xe0, /* ....9... */
0x00, 0x00, 0x0f, 0x00, 0x11, 0x00, 0x00, 0x10, /* ........ */
0x00, 0x0c, 0x00, 0x02, 0xf8, 0x39, 0x00, 0xe0, /* .....9.. */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 

/*
    M2 Setup
*/

// SETUP REQUEST
int eNB_send_M2_SETUP_REQUEST(m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t* m2ap_eNB_data_p) {
 // module_id_t enb_mod_idP=0;
 // module_id_t du_mod_idP=0;

  M2AP_M2AP_PDU_t          pdu;
  M2AP_M2SetupRequest_t    *out;
  M2AP_M2SetupRequest_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;
  int       j = 0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M2AP_InitiatingMessage_t *)calloc(1, sizeof(M2AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_m2Setup;
  pdu.choice.initiatingMessage.criticality   = M2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_M2SetupRequest;
  out = &pdu.choice.initiatingMessage.value.choice.M2SetupRequest;

  /* mandatory */
  /* c1. GlobalENB_ID (integer value) */
  ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_GlobalENB_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_M2SetupRequest_Ies__value_PR_GlobalENB_ID;
  //ie->value.choice.GlobalENB_ID.eNB_ID = 1;//M2AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);
  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                  &ie->value.choice.GlobalENB_ID.pLMN_Identity);
  ie->value.choice.GlobalENB_ID.eNB_ID.present = M2AP_ENB_ID_PR_macro_eNB_ID;
  MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
                           &ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID);
  M2AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id,
          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[0],
          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[1],
          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[2]);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ///* mandatory */
  ///* c2. GNB_eNB_ID (integrer value) */
  //ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
  //ie->id                        = M2AP_ProtocolIE_ID_id_gNB_eNB_ID;
  //ie->criticality               = M2AP_Criticality_reject;
  //ie->value.present             = M2AP_M2SetupRequestIEs__value_PR_GNB_eNB_ID;
  //asn_int642INTEGER(&ie->value.choice.GNB_eNB_ID, f1ap_du_data->gNB_eNB_id);
  //ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

     /* optional */
  /* c3. ENBname */
  if (m2ap_eNB_data_p->eNB_name != NULL) {
    ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
    ie->id                        = M2AP_ProtocolIE_ID_id_ENBname;
    ie->criticality               = M2AP_Criticality_ignore;
    ie->value.present             = M2AP_M2SetupRequest_Ies__value_PR_ENBname;
    OCTET_STRING_fromBuf(&ie->value.choice.ENBname, m2ap_eNB_data_p->eNB_name,
                         strlen(m2ap_eNB_data_p->eNB_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. serverd cells list */
  ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_List;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_M2SetupRequest_Ies__value_PR_ENB_MBMS_Configuration_data_List;

  int num_mbms_available = instance_p->num_mbms_configuration_data_list;
  LOG_D(M2AP, "num_mbms_available = %d \n", num_mbms_available);

 for (i=0;
       i<num_mbms_available;
       i++) {
        /* mandatory */
        /* 4.1 serverd cells item */

        M2AP_ENB_MBMS_Configuration_data_ItemIEs_t *mbms_configuration_data_list_item_ies;
        mbms_configuration_data_list_item_ies = (M2AP_ENB_MBMS_Configuration_data_ItemIEs_t *)calloc(1, sizeof(M2AP_ENB_MBMS_Configuration_data_ItemIEs_t));
        mbms_configuration_data_list_item_ies->id = M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_Item;
        mbms_configuration_data_list_item_ies->criticality = M2AP_Criticality_reject;
        mbms_configuration_data_list_item_ies->value.present = M2AP_ENB_MBMS_Configuration_data_ItemIEs__value_PR_ENB_MBMS_Configuration_data_Item;

        M2AP_ENB_MBMS_Configuration_data_Item_t *mbms_configuration_data_item;
        mbms_configuration_data_item = &mbms_configuration_data_list_item_ies->value.choice.ENB_MBMS_Configuration_data_Item;
        {
		/* M2AP_ECGI_t eCGI */
                MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                  &mbms_configuration_data_item->eCGI.pLMN_Identity);
                MACRO_ENB_ID_TO_CELL_IDENTITY(instance_p->eNB_id,0,
                                   &mbms_configuration_data_item->eCGI.eUTRANcellIdentifier);
		/* M2AP_MBSFN_SynchronisationArea_ID_t mbsfnSynchronisationArea */ 
		mbms_configuration_data_item->mbsfnSynchronisationArea=instance_p->mbms_configuration_data_list[i].mbsfn_sync_area; //? long
		/* M2AP_MBMS_Service_Area_ID_List_t mbmsServiceAreaList */
		for(j=0;j<instance_p->mbms_configuration_data_list[i].num_mbms_service_area_list;j++){
			M2AP_MBMS_Service_Area_t * mbms_service_area = (M2AP_MBMS_Service_Area_t*)calloc(1,sizeof(M2AP_MBMS_Service_Area_t));
			
			char buf[2];
			INT16_TO_BUFFER(instance_p->mbms_configuration_data_list[i].mbms_service_area_list[j],buf);
			OCTET_STRING_fromBuf(mbms_service_area,buf,2);
			//LOG_D(M2AP,"%s\n",instance_p->mbms_configuration_data_list[i].mbms_service_area_list[j]);
			//OCTET_STRING_fromBuf(mbms_service_area,"03",2);
                	ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area);
		}
                /*M2AP_MBMS_Service_Area_t * mbms_service_area,*mbms_service_area2;
                mbms_service_area = (M2AP_MBMS_Service_Area_t*)calloc(1,sizeof(M2AP_MBMS_Service_Area_t));
                mbms_service_area2 = (M2AP_MBMS_Service_Area_t*)calloc(1,sizeof(M2AP_MBMS_Service_Area_t));
		//memset(mbms_service_area,0,sizeof(OCTET_STRING_t));
		OCTET_STRING_fromBuf(mbms_service_area,"01",2);
                ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area);
		OCTET_STRING_fromBuf(mbms_service_area2,"02",2);
                ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area2);*/


        }


        //M2AP_ENB_MBMS_Configuration_data_Item_t mbms_configuration_data_item;
        //memset((void *)&mbms_configuration_data_item, 0, sizeof(M2AP_ENB_MBMS_Configuration_data_Item_t));

        //M2AP_ECGI_t      eCGI;
                //M2AP_PLMN_Identity_t     pLMN_Identity;
                //M2AP_EUTRANCellIdentifier_t      eUTRANcellIdentifier
        //M2AP_MBSFN_SynchronisationArea_ID_t      mbsfnSynchronisationArea;
        //M2AP_MBMS_Service_Area_ID_List_t         mbmsServiceAreaList;


        ASN_SEQUENCE_ADD(&ie->value.choice.ENB_MBMS_Configuration_data_List.list,mbms_configuration_data_list_item_ies);

 }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  
 LOG_D(M2AP,"m2ap_eNB_data_p->assoc_id %d\n",m2ap_eNB_data_p->assoc_id);
  /* encode */
 if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
   LOG_E(M2AP, "Failed to encode M2 setup request\n");
   return -1;
 }


  LOG_D(M2AP,"pdu.present %d\n",pdu.present);
 // MSC_LOG_TX_MESSAGE(
 // MSC_M2AP_eNB,
 // MSC_M2AP_MCE,
 // (const char *)buffer,
 // len,
 // MSC_AS_TIME_FMT" M2_SETUP_REQUEST initiatingMessage MCEname %s",
 // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 // m2ap_eNB_data_p->ENBname);


//  buffer = &bytes[0];
//  len = 40;
//
//  for(int i=0; i < len; i++ )
//	printf("%02X",buffer[i]);
//  printf("\n");
//
  m2ap_eNB_itti_send_sctp_data_req(instance_p->instance, m2ap_eNB_data_p->assoc_id, buffer, len, 0);

  return 0;
}


int eNB_handle_M2_SETUP_RESPONSE(instance_t instance,
				uint32_t               assoc_id,
				uint32_t               stream,
				M2AP_M2AP_PDU_t       *pdu)
{

   LOG_D(M2AP, "eNB_handle_M2_SETUP_RESPONSE\n");

   AssertFatal(pdu->present == M2AP_M2AP_PDU_PR_successfulOutcome,
	       "pdu->present != M2AP_M2AP_PDU_PR_successfulOutcome\n");
   AssertFatal(pdu->choice.successfulOutcome.procedureCode  == M2AP_ProcedureCode_id_m2Setup,
	       "pdu->choice.successfulOutcome.procedureCode != M2AP_ProcedureCode_id_M2Setup\n");
   AssertFatal(pdu->choice.successfulOutcome.criticality  == M2AP_Criticality_reject,
	       "pdu->choice.successfulOutcome.criticality != M2AP_Criticality_reject\n");
   AssertFatal(pdu->choice.successfulOutcome.value.present  == M2AP_SuccessfulOutcome__value_PR_M2SetupResponse,
	       "pdu->choice.successfulOutcome.value.present != M2AP_SuccessfulOutcome__value_PR_M2SetupResponse\n");

   M2AP_M2SetupResponse_t    *in = &pdu->choice.successfulOutcome.value.choice.M2SetupResponse;


   M2AP_M2SetupResponse_Ies_t *ie;
   //int GlobalMCE_ID = -1;
   int num_cells_to_activate = 0;
   //M2AP_Cells_to_be_Activated_List_Item_t *cell;
   int i,j;

   MessageDef *msg_p = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_SETUP_RESP);
   //MessageDef *msg_p2 = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_SETUP_RESP);

   LOG_D(M2AP, "M2AP: M2Setup-Resp: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
   for (j=0;j < in->protocolIEs.list.count; j++) {
     ie = in->protocolIEs.list.array[j];
     switch (ie->id) {
     case M2AP_ProtocolIE_ID_id_GlobalMCE_ID:
       AssertFatal(ie->criticality == M2AP_Criticality_reject,
		   "ie->criticality != M2AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M2AP_M2SetupResponse_Ies__value_PR_GlobalMCE_ID,
		   "ie->value.present != M2AP_M2SetupResponse_Ies__value_PR_GlobalMCE_ID\n");
       LOG_D(M2AP, "M2AP: M2Setup-Resp: GlobalMCE_ID \n");/*,
             GlobalMCE_ID);*/
       /*PLMNID_TO_MCC_MNC(&m2ap_pmchconfiguration_item->mbms_Session_List.list.array[k]->tmgi.pLMNidentity,
				&M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mcc,
				&M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mnc,
				&M2AP_MBMS_SCHEDULING_INFORMATION(message_p).mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].mnc_length);*/

       
       break;
     case M2AP_ProtocolIE_ID_id_MCEname:
       AssertFatal(ie->criticality == M2AP_Criticality_ignore,
		   "ie->criticality != M2AP_Criticality_ignore\n");
       AssertFatal(ie->value.present == M2AP_M2SetupResponse_Ies__value_PR_MCEname,
		   "ie->value.present != M2AP_M2SetupResponse_Ies__value_PR_MCEname\n");
       //M2AP_SETUP_RESP (msg_p).MCE_name = malloc(ie->value.choice.size+1);
       //memcpy(M2AP_SETUP_RESP (msg_p).gNB_CU_name,ie->value.choice.GNB_CU_Name.buf,ie->value.choice.GNB_CU_Name.size);
       //M2AP_SETUP_RESP (msg_p).gNB_CU_name[ie->value.choice.GNB_CU_Name.size]='\0';
       //LOG_D(M2AP, "M2AP: M2Setup-Resp: gNB_CU_name %s\n",
             //M2AP_SETUP_RESP (msg_p).gNB_CU_name);
       break;
     case M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea:
       AssertFatal(ie->criticality == M2AP_Criticality_reject,
		   "ie->criticality != M2AP_Criticality_reject\n");
       AssertFatal(ie->value.present == M2AP_M2SetupResponse_Ies__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea,
		   "ie->value.present != M2AP_M2SetupResponse_Ies__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea\n");
       num_cells_to_activate = ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list.count;
       if(ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list.count > 0 ){
        	M2AP_SETUP_RESP(msg_p).num_mcch_config_per_mbsfn = ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list.count;
       }
       //LOG_D(M2AP, "M2AP: Activating %d cells\n",num_cells_to_activate);
       for (i=0;i<num_cells_to_activate;i++) {
	M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t * mcch_related_bcch_config_per_mbms_area_ies = (M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_ItemIEs_t*) ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list.array[i];
	AssertFatal(mcch_related_bcch_config_per_mbms_area_ies->id == M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item, 
			" mcch_related_bcch_config_per_mbms_area_ies->id != M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item ");
	M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item_t * config_per_mbsfn_area_item = &mcch_related_bcch_config_per_mbms_area_ies->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea_Item;
	M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[i].pdcch_length = (uint8_t)config_per_mbsfn_area_item->pdcchLength; 
	M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[i].offset 	     = (uint8_t)config_per_mbsfn_area_item->offset;
	M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[i].modification_period = (uint8_t)config_per_mbsfn_area_item->modificationPeriod;
	M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[i].mcs 	     = (uint8_t)config_per_mbsfn_area_item->modulationAndCodingScheme;
	M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[i].repetition_period  = (uint8_t)config_per_mbsfn_area_item->repetitionPeriod;

	//LOG_E(M2AP,"mcs %lu\n",config_per_mbsfn_area_item->modulationAndCodingScheme);
	//LOG_E(M2AP,"pdcch_length %lu\n",config_per_mbsfn_area_item->pdcchLength);
	//LOG_E(M2AP,"modification_period %lu\n",config_per_mbsfn_area_item->modificationPeriod);
	//LOG_E(M2AP,"repetition_period %lu\n",config_per_mbsfn_area_item->repetitionPeriod);
	//LOG_E(M2AP,"offset %lu\n",config_per_mbsfn_area_item->offset);
	//LOG_E(M2AP,"subframe_allocation_info %lu\n", config_per_mbsfn_area_item->subframeAllocationInfo.size);
	

	if(config_per_mbsfn_area_item->subframeAllocationInfo.size == 1){
		M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[i].subframe_allocation_info = config_per_mbsfn_area_item->subframeAllocationInfo.buf[0]>>2;
		LOG_D(M2AP,"subframe_allocation_info %d\n", config_per_mbsfn_area_item->subframeAllocationInfo.buf[0]);
	}		
       }
       break;
     }
   }
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
   //itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p2);
   itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);

   return 0;
}

// SETUP FAILURE
int eNB_handle_M2_SETUP_FAILURE(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M2AP_M2AP_PDU_t *pdu) {
  LOG_D(M2AP, "eNB_handle_M2_SETUP_FAILURE\n");

  M2AP_M2SetupFailure_t    *in = &pdu->choice.unsuccessfulOutcome.value.choice.M2SetupFailure;


   M2AP_M2SetupFailure_Ies_t *ie;


  MessageDef *msg_p = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_SETUP_FAILURE);

   LOG_D(M2AP, "M2AP: M2Setup-Failure: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
   for (int i=0;i < in->protocolIEs.list.count; i++) {
     ie = in->protocolIEs.list.array[i];
     switch (ie->id) {
     case M2AP_ProtocolIE_ID_id_TimeToWait:
       AssertFatal(ie->criticality == M2AP_Criticality_ignore,
		   "ie->criticality != M2AP_Criticality_ignore\n");
       AssertFatal(ie->value.present == M2AP_M2SetupFailure_Ies__value_PR_TimeToWait,
		   "ie->value.present != M2AP_M2SetupFailure_Ies__value_PR_TimeToWait\n");
       LOG_D(M2AP, "M2AP: M2Setup-Failure: TimeToWait \n");/*,
             GlobalMCE_ID);*/
       break;
     }
   }
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

   itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);
  
  return 0;
}

/*
 * eNB Configuration Update
 */
int eNB_send_eNB_CONFIGURATION_UPDATE(instance_t instance, m2ap_enb_configuration_update_t * m2ap_enb_configuration_update) 
{

  AssertFatal(1==0,"Not implemented yet\n");

  M2AP_M2AP_PDU_t          pdu;
  M2AP_ENBConfigurationUpdate_t    *out;
  M2AP_ENBConfigurationUpdate_Ies_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;
  //int       j = 0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  //pdu.choice.initiatingMessage = (M2AP_InitiatingMessage_t *)calloc(1, sizeof(M2AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_eNBConfigurationUpdate;
  pdu.choice.initiatingMessage.criticality   = M2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_ENBConfigurationUpdate;
  out = &pdu.choice.initiatingMessage.value.choice.ENBConfigurationUpdate;

  /* mandatory */
  /* c1. GlobalENB_ID (integer value) */
  ie = (M2AP_ENBConfigurationUpdate_Ies_t *)calloc(1, sizeof(M2AP_ENBConfigurationUpdate_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_GlobalENB_ID;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_ENBConfigurationUpdate_Ies__value_PR_GlobalENB_ID;
  //ie->value.choice.GlobalENB_ID.eNB_ID = 1;//M2AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);
  MCC_MNC_TO_PLMNID(0, 0, 3,
                  &ie->value.choice.GlobalENB_ID.pLMN_Identity);
  ie->value.choice.GlobalENB_ID.eNB_ID.present = M2AP_ENB_ID_PR_macro_eNB_ID;
  MACRO_ENB_ID_TO_BIT_STRING(10,
                           &ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID);
  M2AP_INFO("%d -> %02x%02x%02x\n", 10,
          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[0],
          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[1],
          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[2]);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. ENBname */
  if (0) {
    ie = (M2AP_ENBConfigurationUpdate_Ies_t *)calloc(1, sizeof(M2AP_ENBConfigurationUpdate_Ies_t));
    ie->id                        = M2AP_ProtocolIE_ID_id_ENBname;
    ie->criticality               = M2AP_Criticality_ignore;
    ie->value.present             = M2AP_ENBConfigurationUpdate_Ies__value_PR_ENBname;
    //OCTET_STRING_fromBuf(&ie->value.choice.ENBname, m2ap_eNB_data_p->eNB_name,
                         //strlen(m2ap_eNB_data_p->eNB_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. serverd cells list */
  ie = (M2AP_ENBConfigurationUpdate_Ies_t *)calloc(1, sizeof(M2AP_ENBConfigurationUpdate_Ies_t));
  ie->id                        = M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_List_ConfigUpdate;
  ie->criticality               = M2AP_Criticality_reject;
  ie->value.present             = M2AP_ENBConfigurationUpdate_Ies__value_PR_ENB_MBMS_Configuration_data_List_ConfigUpdate;

  int num_mbms_available = 1;//m2ap_du_data->num_mbms_available;
  LOG_D(M2AP, "num_mbms_available = %d \n", num_mbms_available);

 for (i=0;
       i<num_mbms_available;
       i++) {
        /* mandatory */
        /* 4.1 serverd cells item */

        M2AP_ENB_MBMS_Configuration_data_ItemIEs_t *mbms_configuration_data_list_item_ies;
        mbms_configuration_data_list_item_ies = (M2AP_ENB_MBMS_Configuration_data_ItemIEs_t *)calloc(1, sizeof(M2AP_ENB_MBMS_Configuration_data_ItemIEs_t));
        mbms_configuration_data_list_item_ies->id = M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_Item;
        mbms_configuration_data_list_item_ies->criticality = M2AP_Criticality_reject;
        mbms_configuration_data_list_item_ies->value.present = M2AP_ENB_MBMS_Configuration_data_ItemIEs__value_PR_ENB_MBMS_Configuration_data_Item;

        M2AP_ENB_MBMS_Configuration_data_Item_t *mbms_configuration_data_item;
        mbms_configuration_data_item = &mbms_configuration_data_list_item_ies->value.choice.ENB_MBMS_Configuration_data_Item;
        {
		/* M2AP_ECGI_t eCGI */
                MCC_MNC_TO_PLMNID(0, 0, 3,
                  &mbms_configuration_data_item->eCGI.pLMN_Identity);
                MACRO_ENB_ID_TO_CELL_IDENTITY(10,0,
                                   &mbms_configuration_data_item->eCGI.eUTRANcellIdentifier);
		/* M2AP_MBSFN_SynchronisationArea_ID_t mbsfnSynchronisationArea */ 
		mbms_configuration_data_item->mbsfnSynchronisationArea=10000; //? long
		/* M2AP_MBMS_Service_Area_ID_List_t mbmsServiceAreaList */
                M2AP_MBMS_Service_Area_t * mbms_service_area,*mbms_service_area2;
                mbms_service_area = (M2AP_MBMS_Service_Area_t*)calloc(1,sizeof(M2AP_MBMS_Service_Area_t));
                mbms_service_area2 = (M2AP_MBMS_Service_Area_t*)calloc(1,sizeof(M2AP_MBMS_Service_Area_t));
		//memset(mbms_service_area,0,sizeof(OCTET_STRING_t));
		OCTET_STRING_fromBuf(mbms_service_area,"01",2);
                ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area);
		OCTET_STRING_fromBuf(mbms_service_area2,"02",2);
                ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area2);


        }


        //M2AP_ENB_MBMS_Configuration_data_Item_t mbms_configuration_data_item;
        //memset((void *)&mbms_configuration_data_item, 0, sizeof(M2AP_ENB_MBMS_Configuration_data_Item_t));

        //M2AP_ECGI_t      eCGI;
                //M2AP_PLMN_Identity_t     pLMN_Identity;
                //M2AP_EUTRANCellIdentifier_t      eUTRANcellIdentifier
        //M2AP_MBSFN_SynchronisationArea_ID_t      mbsfnSynchronisationArea;
        //M2AP_MBMS_Service_Area_ID_List_t         mbmsServiceAreaList;


        ASN_SEQUENCE_ADD(&ie->value.choice.ENB_MBMS_Configuration_data_List_ConfigUpdate.list,mbms_configuration_data_list_item_ies);

 }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  
 //LOG_D(M2AP,"m2ap_eNB_data_p->assoc_id %d\n",m2ap_eNB_data_p->assoc_id);
  /* encode */
 if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
   LOG_E(M2AP, "Failed to encode M2 setup request\n");
   return -1;
 }


  LOG_D(M2AP,"pdu.present %d\n",pdu.present);
 // MSC_LOG_TX_MESSAGE(
 // MSC_M2AP_eNB,
 // MSC_M2AP_MCE,
 // (const char *)buffer,
 // len,
 // MSC_AS_TIME_FMT" M2_SETUP_REQUEST initiatingMessage MCEname %s",
 // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
 // m2ap_eNB_data_p->ENBname);


//  buffer = &bytes[0];
//  len = 40;
//
//  for(int i=0; i < len; i++ )
//	printf("%02X",buffer[i]);
//  printf("\n");
//
  m2ap_eNB_itti_send_sctp_data_req(instance, m2ap_enb_data_g->assoc_id, buffer, len, 0);

   return 0;
}

int eNB_handle_eNB_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                                         uint32_t assoc_id,
                                         uint32_t stream,
                                         M2AP_M2AP_PDU_t *pdu)
{

  AssertFatal(1==0,"Not implemented yet\n");
  LOG_D(M2AP, "eNB_handle_eNB_CONFIGURATION_UPDATE_FAILURE\n");

  M2AP_ENBConfigurationUpdateFailure_t    *in = &pdu->choice.unsuccessfulOutcome.value.choice.ENBConfigurationUpdateFailure;


   //M2AP_ENBConfigurationUpdateFailure_Ies_t *ie;


  MessageDef *msg_p = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_ENB_CONFIGURATION_UPDATE_FAILURE);

   LOG_D(M2AP, "M2AP: ENBConfigurationUpdate-Failure: protocolIEs.list.count %d\n",
         in->protocolIEs.list.count);
   //for (int i=0;i < in->protocolIEs.list.count; i++) {
   //  ie = in->protocolIEs.list.array[i];
   // // switch (ie->id) {
   // // case M2AP_ProtocolIE_ID_id_TimeToWait:
   // //   AssertFatal(ie->criticality == M2AP_Criticality_ignore,
   // //    	   "ie->criticality != M2AP_Criticality_ignore\n");
   // //   AssertFatal(ie->value.present == M2AP_M2SetupFailure_Ies__value_PR_TimeToWait,
   // //    	   "ie->value.present != M2AP_M2SetupFailure_Ies__value_PR_TimeToWait\n");
   // //   LOG_D(M2AP, "M2AP: M2Setup-Failure: TimeToWait %d\n");/*,
   // //         GlobalMCE_ID);*/
   // //   break;
   // // }
   //}
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

   itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);

   return 0;
}



int eNB_handle_eNB_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                         uint32_t assoc_id,
                                         uint32_t stream,
                                         M2AP_M2AP_PDU_t *pdu)
{
  LOG_D(M2AP, "eNB_handle_eNB_CONFIGURATION_UPDATE_ACKNOWLEDGE assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_ENBConfigurationUpdateAcknowledge_t              *container;
  //M2AP_ENBConfigurationUpdateAcknowledge_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.ENBConfigurationUpdate;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_ENB_CONFIGURATION_UPDATE_ACK);


  itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);


   return 0;
}


/*
 * MCE Configuration Update
 */
int eNB_handle_MCE_CONFIGURATION_UPDATE(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          M2AP_M2AP_PDU_t *pdu)
{
 LOG_D(M2AP, "eNB_handle_MCE_CONFIGURATION_UPDATE assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_MCEConfigurationUpdate_t              *container;
  //M2AP_MCEConfigurationUpdate_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.MCEConfigurationUpdate;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_MCE_CONFIGURATION_UPDATE);


  itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);


   return 0;
}

int eNB_send_MCE_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                    M2AP_MCEConfigurationUpdateFailure_t *MCEConfigurationUpdateFailure)
{

  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}


int eNB_send_MCE_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                    M2AP_MCEConfigurationUpdateAcknowledge_t *MCEConfigurationUpdateAcknowledge)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}


/*
 * Error Indication
 */
int eNB_send_ERROR_INDICATION(instance_t instance, m2ap_error_indication_t * m2ap_error_indication)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}

int eNB_handle_ERROR_INDICATION(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               M2AP_M2AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}



/*
 * Session Update Request
 */
int eNB_handle_MBMS_SESSION_UPDATE_REQUEST(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu)
{
  LOG_D(M2AP, "eNB_handle_MBMS_SESSION_STOP_REQUEST assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_SessionUpdateRequest_t              *container;
  //M2AP_SessionUpdateRequest_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

  //container = &pdu->choice.initiatingMessage.value.choice.SessionUpdateRequest;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_MBMS_SESSION_UPDATE_REQ);


  itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);



   return 0;
}


int eNB_send_MBMS_SESSION_UPDATE_RESPONSE(instance_t instance, m2ap_mbms_session_update_resp_t * m2ap_mbms_session_update_resp)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}


int eNB_send_MBMS_SESSION_UPDATE_FAILURE(instance_t instance, m2ap_mbms_session_update_failure_t * m2ap_mbms_session_update_failure)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}

/*
 * Service Counting
 */

int eNB_handle_MBMS_SERVICE_COUNTING_REQ(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  M2AP_M2AP_PDU_t *pdu)
{
  LOG_D(M2AP, "eNB_handle_MBMS_SERVICE_COUNTING_REQUEST assoc_id %d\n",assoc_id);

  MessageDef                         *message_p;
  //M2AP_MbmsServiceCountingRequest_t              *container;
  //M2AP_MbmsServiceCountingRequest_Ies_t           *ie;
  //int i = 0;

  DevAssert(pdu != NULL);

 // container = &pdu->choice.initiatingMessage.value.choice.MbmsServiceCountingRequest;

  /* M2 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_D(M2AP, "[SCTP %d] Received MMBS session start request on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  message_p  = itti_alloc_new_message (TASK_M2AP_ENB, M2AP_MBMS_SERVICE_COUNTING_REQ);


  itti_send_msg_to_task(TASK_ENB_APP, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);




   return 0;
}
int eNB_send_MBMS_SERVICE_COUNTING_REPORT(instance_t instance, m2ap_mbms_service_counting_report_t * m2ap_mbms_service_counting_report)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}
int eNB_send_MBMS_SERVICE_COUNTING_RESP(instance_t instance, m2ap_mbms_service_counting_resp_t * m2ap_mbms_service_counting_resp)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}
int eNB_send_MBMS_SERVICE_COUNTING_FAILURE(instance_t instance, m2ap_mbms_service_counting_failure_t * m2ap_mbms_service_counting_failure)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}

/*
 * Overload Notification
 */
int eNB_send_MBMS_OVERLOAD_NOTIFICATION(instance_t instance, m2ap_mbms_overload_notification_t * m2ap_mbms_overload_notification)
{
  AssertFatal(1==0,"Not implemented yet\n");
   return 0;
}




