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

/*! \file rrc_eNB_M2AP.c
 * \brief rrc M2AP procedures for eNB
 * \author Javier Morgade
 * \version 0.1
 * \company Vicomtech Spain
 * \email: javier.morgade@ieee.org
 */

# include "rrc_defs.h"
# include "rrc_extern.h"
# include "RRC/LTE/MESSAGES/asn1_msg.h"
# include "rrc_eNB_M2AP.h"
//# include "rrc_eNB_UE_context.h"
#   include "oai_asn1.h"
#   include "intertask_interface.h"
# include "common/ran_context.h"
#include "pdcp.h"
#include "uper_encoder.h"

extern RAN_CONTEXT_t RC;

static m2ap_setup_resp_t * m2ap_setup_resp_g=NULL;
static m2ap_mbms_scheduling_information_t *m2ap_mbms_scheduling_information_g=NULL;

 
static void 
rrc_M2AP_openair_rrc_top_init_MBMS(int eMBMS_active){
  module_id_t         module_id;
  int                 CC_id;
  
  (void)CC_id;
  LOG_D(RRC, "[OPENAIR][INIT] Init function start: NB_eNB_INST=%d\n", RC.nb_inst);

  if (RC.nb_inst > 0) {
    LOG_I(RRC,"[eNB] eMBMS active state is %d \n", eMBMS_active);

    for (module_id=0; module_id<NB_eNB_INST; module_id++) {
      for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
        RC.rrc[module_id]->carrier[CC_id].MBMS_flag = (uint8_t)eMBMS_active;
      }
    }
  }
}


static uint8_t rrc_M2AP_do_MBSFNCountingRequest(
    uint8_t Mod_id,
    uint8_t sync_area,
    uint8_t *buffer,
    LTE_MCCH_Message_t *mcch_message,
    LTE_MBMSCountingRequest_r10_t **mbsfnCoutingRequest,
    const m2ap_mbms_service_counting_req_t *const m2ap_mbms_service_counting_req
) {
  //int i,j,k;

  asn_enc_rval_t enc_rval;
  //LTE_MBSFN_SubframeConfig_t *mbsfn_SubframeConfig1;
  //LTE_PMCH_Info_r9_t *pmch_Info_1;
  //LTE_MBMS_SessionInfo_r9_t *mbms_Session_1;
  // MBMS_SessionInfo_r9_t *mbms_Session_2;
  //eNB_RRC_INST *rrc               = RC.rrc[Mod_id];
  //rrc_eNB_carrier_data_t *carrier = &rrc->carrier[0];
  memset(mcch_message,0,sizeof(LTE_MCCH_Message_t));
  //mcch_message->message.present = LTE_MCCH_MessageType_PR_c1;
  mcch_message->message.present = LTE_MCCH_MessageType_PR_later;
  //mcch_message->message.choice.c1.present = LTE_MCCH_MessageType__c1_PR_mbsfnAreaConfiguration_r9;
  mcch_message->message.choice.later.present = LTE_MCCH_MessageType__later_PR_c2;
  mcch_message->message.choice.later.choice.c2.present = LTE_MCCH_MessageType__later__c2_PR_mbmsCountingRequest_r10;
  *mbsfnCoutingRequest = &mcch_message->message.choice.later.choice.c2.choice.mbmsCountingRequest_r10;


  //LTE_CountingRequestList_r10_t countingRequestList_r10; // A_SEQUENCE_OF(struct LTE_CountingRequestInfo_r10) list;
  struct LTE_CountingRequestInfo_r10 *lte_counting_request_info; //LTE_TMGI_r9_t    tmgi_r10;
  lte_counting_request_info = CALLOC(1,sizeof(struct LTE_CountingRequestInfo_r10));
  uint8_t TMGI[5] = {4,3,2,1,0};
  lte_counting_request_info->tmgi_r10.plmn_Id_r9.present = LTE_TMGI_r9__plmn_Id_r9_PR_plmn_Index_r9;
  lte_counting_request_info->tmgi_r10.plmn_Id_r9.choice.plmn_Index_r9= 1;
  memset(&lte_counting_request_info->tmgi_r10.serviceId_r9,0,sizeof(OCTET_STRING_t));
  OCTET_STRING_fromBuf(&lte_counting_request_info->tmgi_r10.serviceId_r9,(const char*)&TMGI[2],3);
  asn1cSeqAdd(&(*mbsfnCoutingRequest)->countingRequestList_r10.list,lte_counting_request_info);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_MCCH_Message,(void *)mcch_message);
  }
    
  xer_fprint(stdout,&asn_DEF_LTE_MCCH_Message,(void *)mcch_message);

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_MCCH_Message,
                                   NULL,
                                   (void *)mcch_message,
                                   buffer,
                                   100);

    if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_I(RRC,"[eNB] MCCH Message Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : MCCH  encoding failed for MBSFNAreaConfiguration\n");
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
  
}


static uint8_t rrc_M2AP_do_MBSFNAreaConfig(
    uint8_t Mod_id,
    uint8_t sync_area,
    uint8_t *buffer,
    LTE_MCCH_Message_t *mcch_message,
    LTE_MBSFNAreaConfiguration_r9_t **mbsfnAreaConfiguration,
    const m2ap_mbms_scheduling_information_t *const m2ap_mbms_scheduling_information
) {
  int i,j,k;

  asn_enc_rval_t enc_rval;
  LTE_MBSFN_SubframeConfig_t *mbsfn_SubframeConfig1;
  LTE_PMCH_Info_r9_t *pmch_Info_1;
  LTE_MBMS_SessionInfo_r9_t *mbms_Session_1;
  // MBMS_SessionInfo_r9_t *mbms_Session_2;
  //eNB_RRC_INST *rrc               = RC.rrc[Mod_id];
  //rrc_eNB_carrier_data_t *carrier = &rrc->carrier[0];
  memset(mcch_message,0,sizeof(LTE_MCCH_Message_t));
  mcch_message->message.present = LTE_MCCH_MessageType_PR_c1;
  mcch_message->message.choice.c1.present = LTE_MCCH_MessageType__c1_PR_mbsfnAreaConfiguration_r9;
  *mbsfnAreaConfiguration = &mcch_message->message.choice.c1.choice.mbsfnAreaConfiguration_r9;
  // Common Subframe Allocation (CommonSF-Alloc-r9)

  for(i=0; i<m2ap_mbms_scheduling_information->num_mbms_area_config_list; i++){
	  for(j=0;j < m2ap_mbms_scheduling_information->mbms_area_config_list[i].num_mbms_sf_config_list; j++){
      mbsfn_SubframeConfig1 = CALLOC(1, sizeof(*mbsfn_SubframeConfig1));
      //
		  mbsfn_SubframeConfig1->radioframeAllocationPeriod= m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].radioframe_allocation_period;//LTE_MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
		  mbsfn_SubframeConfig1->radioframeAllocationOffset= m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].radioframe_allocation_offset;
		  if(m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].is_four_sf){
	         	  LOG_I(RRC,"is_four_sf\n");
			  mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames;
			  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(3);
                          mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[2] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation) & 0xFF);
                          mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[1] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation>>8) & 0xFF);
                          mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation>>16) & 0xFF);
			  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 3;
			  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 0;
		

   		  }else {
			  LOG_I(RRC,"is_one_sf\n");
			  mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
			  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
			  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 1;
			  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 2;
			  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0] = (m2ap_mbms_scheduling_information->mbms_area_config_list[i].mbms_sf_config_list[j].subframe_allocation & 0x3F)<<2;
		  }

		  asn1cSeqAdd(&(*mbsfnAreaConfiguration)->commonSF_Alloc_r9.list,mbsfn_SubframeConfig1);
	  }
	  //  commonSF-AllocPeriod-r9
	  (*mbsfnAreaConfiguration)->commonSF_AllocPeriod_r9= m2ap_mbms_scheduling_information->mbms_area_config_list[i].common_sf_allocation_period;//LTE_MBSFNAreaConfiguration_r9__commonSF_AllocPeriod_r9_rf16;
	  // PMCHs Information List (PMCH-InfoList-r9)
	  for(j=0; j < m2ap_mbms_scheduling_information->mbms_area_config_list[i].num_pmch_config_list; j++){
		  // PMCH_1  Config
      pmch_Info_1 = CALLOC(1, sizeof(LTE_PMCH_Info_r9_t));
      /*
		   * take the value of last mbsfn subframe in this CSA period because there is only one PMCH in this mbsfn area
		   * Note: this has to be set based on the subframeAllocation and CSA
		   */
		  pmch_Info_1->pmch_Config_r9.sf_AllocEnd_r9= m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].allocated_sf_end;
		  pmch_Info_1->pmch_Config_r9.dataMCS_r9= m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].data_mcs;
		  //pmch_Info_1->pmch_Config_r9.mch_SchedulingPeriod_r9= LTE_PMCH_Config_r9__mch_SchedulingPeriod_r9_rf16;
		  pmch_Info_1->pmch_Config_r9.mch_SchedulingPeriod_r9 = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mch_scheduling_period;
		  // MBMSs-SessionInfoList-r9
		  for(k=0; k < m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].num_mbms_session_list; k++){
			  //  pmch_Info_1->mbms_SessionInfoList_r9 = CALLOC(1,sizeof(struct MBMS_SessionInfoList_r9));
			  //  Session 1
        mbms_Session_1 = CALLOC(1, sizeof(LTE_MBMS_SessionInfo_r9_t));
        // TMGI value
			  mbms_Session_1->tmgi_r9.plmn_Id_r9.present= LTE_TMGI_r9__plmn_Id_r9_PR_plmn_Index_r9;
			  mbms_Session_1->tmgi_r9.plmn_Id_r9.choice.plmn_Index_r9= 1;
			  // Service ID
			  //uint8_t TMGI[5] = {4,3,2,1,0};//TMGI is a string of octet, ref. TS 24.008 fig. 10.5.4a
			  char buf[4];
			  buf[0] = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id << 24;
			  buf[1] = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id << 16;
			  buf[2] = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id << 8;
			  buf[3] = m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id << 0;
			  //INT32_TO_BUFFER(m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id,buf);
			  memset(&mbms_Session_1->tmgi_r9.serviceId_r9,0,sizeof(OCTET_STRING_t));// need to check
			  OCTET_STRING_fromBuf(&mbms_Session_1->tmgi_r9.serviceId_r9,(const char *)&buf[1],3);
			  // Session ID is still missing here, it can be used as an rab id or mrb id
			  mbms_Session_1->sessionId_r9 = CALLOC(1,sizeof(OCTET_STRING_t));
			  mbms_Session_1->sessionId_r9->buf= MALLOC(1);
			  mbms_Session_1->sessionId_r9->size= 1;
			  mbms_Session_1->sessionId_r9->buf[0]= m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id; //1;
			  // Logical Channel ID
			  mbms_Session_1->logicalChannelIdentity_r9=m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid; //1;
			  LOG_D(RRC,"lcid %lu %d\n",mbms_Session_1->logicalChannelIdentity_r9,m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].lcid);
			  LOG_D(RRC,"service_id %d\n",m2ap_mbms_scheduling_information->mbms_area_config_list[i].pmch_config_list[j].mbms_session_list[k].service_id);
			  asn1cSeqAdd(&pmch_Info_1->mbms_SessionInfoList_r9.list,mbms_Session_1);
		  }
		  asn1cSeqAdd(&(*mbsfnAreaConfiguration)->pmch_InfoList_r9.list,pmch_Info_1);
	  }
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_MCCH_Message,(void *)mcch_message);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_MCCH_Message,
                                   NULL,
                                   (void *)mcch_message,
                                   buffer,
                                   100);

    if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_I(RRC,"[eNB] MCCH Message Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : MCCH  encoding failed for MBSFNAreaConfiguration\n");
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
  
}

static void rrc_M2AP_init_MBMS(
  module_id_t enb_mod_idP, 
  int CC_id,
  frame_t frameP
){
   // init the configuration for MTCH
  protocol_ctxt_t               ctxt;

  if (RC.rrc[enb_mod_idP]->carrier[CC_id].MBMS_flag > 0) {
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, enb_mod_idP, ENB_FLAG_YES, NOT_A_RNTI, frameP, 0,enb_mod_idP);
    LOG_I(RRC, "[eNB %d] Frame %d : Radio Bearer config request for MBMS\n", enb_mod_idP, frameP);   //check the lcid
    // Configuring PDCP and RLC for MBMS Radio Bearer
    rrc_pdcp_config_asn1_req(&ctxt,
                             (LTE_SRB_ToAddModList_t *)NULL,   // LTE_SRB_ToAddModList
                             (LTE_DRB_ToAddModList_t *)NULL,   // LTE_DRB_ToAddModList
                             (LTE_DRB_ToReleaseList_t *)NULL,
                             0,     // security mode
                             NULL,  // key rrc encryption
                             NULL,  // key rrc integrity
                             NULL   // key encryption
                             , &(RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->pmch_InfoList_r9)
                             ,NULL);

    rrc_rlc_config_asn1_req(&ctxt,
                            NULL, // LTE_SRB_ToAddModList
                            NULL,   // LTE_DRB_ToAddModList
                            NULL,   // DRB_ToReleaseList
                            &(RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->pmch_InfoList_r9)
                            ,0, 0
                            );
            //rrc_mac_config_req();
  }
}


static void rrc_M2AP_init_MCCH(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t enb_mod_idP, 
  int CC_id,
 const m2ap_mbms_scheduling_information_t *const m2ap_mbms_scheduling_information
){

    int                                 sync_area = 0;
  
    RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE =
      malloc(RC.rrc[enb_mod_idP]->carrier[CC_id].num_mbsfn_sync_area * sizeof(uint8_t *));

    RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE_COUNTING =
      malloc(RC.rrc[enb_mod_idP]->carrier[CC_id].num_mbsfn_sync_area * sizeof(uint8_t *));
  
    for (sync_area = 0; sync_area < RC.rrc[enb_mod_idP]->carrier[CC_id].num_mbsfn_sync_area; sync_area++) {
      RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] = 0;
      RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area] = (uint8_t *) malloc16(32);
      AssertFatal(RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area] != NULL,
                  "[eNB %d]init_MCCH: FATAL, no memory for MCCH MESSAGE allocated \n", enb_mod_idP);
      RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] = rrc_M2AP_do_MBSFNAreaConfig(enb_mod_idP,
          sync_area,
          (uint8_t *)RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area],
          &RC.rrc[enb_mod_idP]->carrier[CC_id].mcch,
          &RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message,
	  m2ap_mbms_scheduling_information 
      );

      RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE_COUNTING[sync_area] = 0;
      RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE_COUNTING[sync_area] = (uint8_t *) malloc16(32);
      AssertFatal(RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE_COUNTING[sync_area] != NULL,
                  "[eNB %d]init_MCCH: FATAL, no memory for MCCH MESSAGE allocated \n", enb_mod_idP);

      RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE_COUNTING[sync_area] = rrc_M2AP_do_MBSFNCountingRequest(enb_mod_idP,
          sync_area,
          (uint8_t *)RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE_COUNTING[sync_area],
          &RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_counting,
          &RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message_counting,
          NULL//m2ap_mbms_scheduling_information 
      );

      LOG_I(RRC, "mcch message pointer %p for sync area %d \n",
            RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area],
            sync_area);
      LOG_D(RRC, "[eNB %d] MCCH_MESSAGE  contents for Sync Area %d (partial)\n", enb_mod_idP, sync_area);
      LOG_D(RRC, "[eNB %d] CommonSF_AllocPeriod_r9 %ld\n", enb_mod_idP,
            RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->commonSF_AllocPeriod_r9);
      LOG_D(RRC,
            "[eNB %d] CommonSF_Alloc_r9.list.count (number of MBSFN Subframe Pattern) %d\n",
            enb_mod_idP, RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->commonSF_Alloc_r9.list.count);
      LOG_D(RRC, "[eNB %d] MBSFN Subframe Pattern: %02x (in hex)\n",
            enb_mod_idP,
            RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->commonSF_Alloc_r9.list.array[0]->subframeAllocation.
            choice.oneFrame.buf[0]);
      AssertFatal(RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] != 255,
                  "RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] == 255");
      RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESS[sync_area].Active = 1;

      RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESS_COUNTING[sync_area].Active = 1;
    }

    rrc_mac_config_req_eNB_t tmp = {0};
    tmp.CC_id = CC_id;
    tmp.pmch_InfoList = &RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->pmch_InfoList_r9;
    tmp.mbms_AreaConfiguration = RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message;
    rrc_mac_config_req_eNB(enb_mod_idP, &tmp);

    return;
}

 
static uint8_t rrc_M2AP_do_SIB1_MBMS_SIB13(
  			  const protocol_ctxt_t *const ctxt_pP,
			  uint8_t Mod_id, 
			  int CC_id,
  			  const m2ap_setup_resp_t *const m2ap_setup_resp,
  			  const m2ap_mbms_scheduling_information_t *const m2ap_mbms_scheduling_information
){

  int i,j,l;
  
  eNB_RRC_INST *rrc = RC.rrc[ctxt_pP->module_id];
  rrc_eNB_carrier_data_t *carrier=&rrc->carrier[CC_id];

  LTE_MBSFN_SubframeConfigList_t *MBSFNSubframeConfigList/*,*MBSFNSubframeConfigList_copy*/;

  asn_enc_rval_t enc_rval;
  LTE_BCCH_DL_SCH_Message_t *bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].systemInformation;
  LTE_BCCH_DL_SCH_Message_MBMS_t *bcch_message_fembms = &RC.rrc[Mod_id]->carrier[CC_id].siblock1_MBMS;

  LTE_MBSFN_AreaInfoList_r9_t *MBSFNArea_list /*,*MBSFNArea_list_copy*/;
  struct LTE_MBSFN_AreaInfo_r9 *MBSFN_Area1;

  uint8_t *encoded_buffer;
  if(ctxt_pP->brOption){
    AssertFatal(RC.rrc[Mod_id]->carrier[CC_id].SIB23_BR, "[eNB %d] sib2 is null, exiting\n", Mod_id);
    encoded_buffer = RC.rrc[Mod_id]->carrier[CC_id].SIB23_BR;
    LOG_I(RRC, "Running SIB2/3 Encoding for eMTC\n");

  }else
  {
    AssertFatal(RC.rrc[Mod_id]->carrier[CC_id].SIB23, "[eNB %d] sib2 is null, exiting\n", Mod_id);
    encoded_buffer = RC.rrc[Mod_id]->carrier[CC_id].SIB23;
  }

  if (!RC.rrc[Mod_id]->carrier[CC_id].sib1_MBMS) {
    LOG_I(RRC,"[eNB %d] sib1_MBMS is null, it should get created\n",Mod_id);
  }

  //if (!sib3) {
  //  LOG_E(RRC,"[eNB %d] sib3 is null, exiting\n", Mod_id);
  //  exit(-1);
  //}
 
 for (i=0; i < bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count; i++) {
    struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *typeandinfo;
    typeandinfo = bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.array[i];
    switch(typeandinfo->present) {
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_NOTHING: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib4: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib5: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib6: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib7: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib8: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib9: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib10: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib11: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib12_v920: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920:
        RC.rrc[Mod_id]->carrier[CC_id].sib13 = &typeandinfo->choice.sib13_v920;
        LTE_SystemInformationBlockType13_r9_t *sib13 = RC.rrc[Mod_id]->carrier[CC_id].sib13;
        sib13->notificationConfig_r9.notificationRepetitionCoeff_r9 = LTE_MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
        sib13->notificationConfig_r9.notificationOffset_r9 = 0;
        sib13->notificationConfig_r9.notificationSF_Index_r9 = 1;
        //  MBSFN-AreaInfoList
        MBSFNArea_list = &sib13->mbsfn_AreaInfoList_r9; // CALLOC(1,sizeof(*MBSFNArea_list));
        memset(MBSFNArea_list, 0, sizeof(*MBSFNArea_list));

        for (j = 0; j < m2ap_setup_resp->num_mcch_config_per_mbsfn; j++) {
          // MBSFN Area 1
          MBSFN_Area1 = CALLOC(1, sizeof(*MBSFN_Area1));
          MBSFN_Area1->mbsfn_AreaId_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[j].mbsfn_area;
          MBSFN_Area1->non_MBSFNregionLength = m2ap_setup_resp->mcch_config_per_mbsfn[j].pdcch_length;
          MBSFN_Area1->notificationIndicator_r9 = 0;
          MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[j].repetition_period; // LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
          MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[j].offset; // in accordance with mbsfn subframe configuration in sib2
          MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[j].modification_period;

          //  Subframe Allocation Info
          MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf = MALLOC(1);
          MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size = 1;
          MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0] = m2ap_setup_resp->mcch_config_per_mbsfn[j].subframe_allocation_info << 2; // FDD: SF1
          MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused = 2;
          MBSFN_Area1->mcch_Config_r9.signallingMCS_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[j].mcs;
          asn1cSeqAdd(&MBSFNArea_list->list, MBSFN_Area1);
        }
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib14_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib15_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib16_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib17_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib18_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib19_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib20_v1310: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib21_v1430: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib24_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib25_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib26_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib26a_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib27_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib28_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib29_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2:

        LOG_I(RRC, "Adding MBSFN subframe Configuration 1 to SIB2\n");

        for (j = 0; j < m2ap_mbms_scheduling_information->num_mbms_area_config_list; j++) {
          (&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList = CALLOC(1, sizeof(struct LTE_MBSFN_SubframeConfigList));
          MBSFNSubframeConfigList = (&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList;

          for (l = 0; l < m2ap_mbms_scheduling_information->mbms_area_config_list[j].num_mbms_sf_config_list; l++) {
            LTE_MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig1;
            sib2_mbsfn_SubframeConfig1 = CALLOC(1, sizeof(*sib2_mbsfn_SubframeConfig1));
            memset((void *)sib2_mbsfn_SubframeConfig1, 0, sizeof(*sib2_mbsfn_SubframeConfig1));

            sib2_mbsfn_SubframeConfig1->radioframeAllocationPeriod = m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].radioframe_allocation_period;
            sib2_mbsfn_SubframeConfig1->radioframeAllocationOffset = m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].radioframe_allocation_offset;

            if (m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].is_four_sf) {
              LOG_I(RRC, "is_four_sf\n");
              sib2_mbsfn_SubframeConfig1->subframeAllocation.present = LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames;

              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf = MALLOC(3);
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size = 3;
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused = 0;
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[2] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation) & 0xFF);
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[1] =
                  ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation >> 8) & 0xFF);
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0] =
                  ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation >> 16) & 0xFF);

            } else {
              LOG_I(RRC, "is_one_sf\n");
              sib2_mbsfn_SubframeConfig1->subframeAllocation.present = LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf = MALLOC(1);
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size = 1;
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused = 2;
              sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0] = (m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation << 2);
            }

            asn1cSeqAdd(&MBSFNSubframeConfigList->list, sib2_mbsfn_SubframeConfig1);
          }
        }

        break;
    }
  }

  if (RC.rrc[Mod_id]->carrier[CC_id].sib13 == NULL) {
    struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib13_part = CALLOC(1, sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    sib13_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920;

    RC.rrc[Mod_id]->carrier[CC_id].sib13 = &sib13_part->choice.sib13_v920;
    LTE_SystemInformationBlockType13_r9_t *sib13 = RC.rrc[Mod_id]->carrier[CC_id].sib13;
    sib13->notificationConfig_r9.notificationRepetitionCoeff_r9 = LTE_MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
    sib13->notificationConfig_r9.notificationOffset_r9 = 0;
    sib13->notificationConfig_r9.notificationSF_Index_r9 = 1;

    //  MBSFN-AreaInfoList
    MBSFNArea_list = &sib13->mbsfn_AreaInfoList_r9; // CALLOC(1,sizeof(*MBSFNArea_list));
    memset(MBSFNArea_list, 0, sizeof(*MBSFNArea_list));

    for (i = 0; i < m2ap_setup_resp->num_mcch_config_per_mbsfn; i++) {
      // MBSFN Area 1
      MBSFN_Area1 = CALLOC(1, sizeof(*MBSFN_Area1));
      MBSFN_Area1->mbsfn_AreaId_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].mbsfn_area;
      MBSFN_Area1->non_MBSFNregionLength = m2ap_setup_resp->mcch_config_per_mbsfn[i].pdcch_length;
      MBSFN_Area1->notificationIndicator_r9 = 0;
      MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].repetition_period; // LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
      MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].offset; // in accordance with mbsfn subframe configuration in sib2
      MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].modification_period;

      //  Subframe Allocation Info
      MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf = MALLOC(1);
      MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size = 1;
      MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0] = m2ap_setup_resp->mcch_config_per_mbsfn[i].subframe_allocation_info << 2; // FDD: SF1
      MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused = 2;
      MBSFN_Area1->mcch_Config_r9.signallingMCS_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].mcs;
      asn1cSeqAdd(&MBSFNArea_list->list, MBSFN_Area1);
    }

    asn1cSeqAdd(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list, sib13_part);
  }

  RC.rrc[Mod_id]->carrier[CC_id].sib1_MBMS = &bcch_message_fembms->message.choice.c1.choice.systemInformationBlockType1_MBMS_r14;
  LTE_SystemInformationBlockType1_MBMS_r14_t *sib1_MBMS = RC.rrc[Mod_id]->carrier[CC_id].sib1_MBMS;
  if (sib1_MBMS->systemInformationBlockType13_r14 == NULL)
    sib1_MBMS->systemInformationBlockType13_r14 = CALLOC(1, sizeof(*sib1_MBMS->systemInformationBlockType13_r14));
  memcpy(sib1_MBMS->systemInformationBlockType13_r14, RC.rrc[Mod_id]->carrier[CC_id].sib13, sizeof(*RC.rrc[Mod_id]->carrier[CC_id].sib13));

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message_MBMS, NULL, (void *)bcch_message_fembms, RC.rrc[Mod_id]->carrier[CC_id].SIB1_MBMS, 100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  LOG_I(RRC,"[eNB] MBMS SIB1_MBMS SystemInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI encoding failed for SIB23\n");
    return(-1);
  }


  
 //xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message);

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message, NULL, (void *)bcch_message, encoded_buffer, 900);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  LOG_I(RRC,"[eNB] MBMS SIB2 SystemInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI encoding failed for SIB23\n");
    return(-1);
  }

  carrier->MBMS_flag = 1;

  rrc_mac_config_req_eNB_t tmp = {0};
  tmp.CC_id = CC_id;
  tmp.rnti = 0xfffd;
  tmp.mbsfn_SubframeConfigList = carrier->sib2->mbsfn_SubframeConfigList;
  tmp.MBMS_Flag = carrier->MBMS_flag;
  tmp.mbsfn_AreaInfoList = &carrier->sib13->mbsfn_AreaInfoList_r9;
  tmp.FeMBMS_Flag = carrier->FeMBMS_flag;
  rrc_mac_config_req_eNB(Mod_id, &tmp);

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB23 = ((enc_rval.encoded+7)/8);
 
  return 0;
}
//static uint8_t rrc_M2AP_do_SIB1(
//  			  const protocol_ctxt_t *const ctxt_pP,
//			  uint8_t Mod_id, 
//			  int CC_id,
//  			  const m2ap_setup_resp_t *const m2ap_setup_resp,
//  			  const m2ap_mbms_scheduling_information_t *const m2ap_mbms_scheduling_information
//){
//  int i/*,j,l*/;
//  
//  //eNB_RRC_INST *rrc = RC.rrc[ctxt_pP->module_id];
//  //rrc_eNB_carrier_data_t *carrier=&rrc->carrier[CC_id];
//
//  asn_enc_rval_t enc_rval;
//
//  uint8_t *buffer;
//
//   LTE_SystemInformationBlockType1_t **sib1;
//
//  LTE_BCCH_DL_SCH_Message_t         *bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].systemInformation;
//
//  if (ctxt_pP->brOption) {
//    buffer       = RC.rrc[Mod_id]->carrier[CC_id].SIB1_BR;
//    bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].siblock1_BR;
//    sib1         = &RC.rrc[Mod_id]->carrier[CC_id].sib1_BR;
//  }
//  else
//    {
//      buffer       = RC.rrc[Mod_id]->carrier[CC_id].SIB1;
//      bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].siblock1;
//      sib1         = &RC.rrc[Mod_id]->carrier[CC_id].sib1;
//    }
//
//    *sib1 = &bcch_message->message.choice.c1.choice.systemInformationBlockType1;
//   
//    uint8_t find_sib13=0;
//    for(i=0; i<(*sib1)->schedulingInfoList.list.count; i++){
//		//for(j=0; j<(*sib1)->schedulingInfoList.list.array[i]->sib_MappingInfo.list.count;j++)
//		//	if((*sib1)->schedulingInfoList.list.array[i]->sib_MappingInfo.list.array[j] == LTE_SIB_Type_sibType13_v920)
//		//		find_sib13=1;
//    }
//    if(!find_sib13){
//   	LTE_SchedulingInfo_t schedulingInfo;
//   	LTE_SIB_Type_t sib_type;
//        memset(&schedulingInfo,0,sizeof(LTE_SchedulingInfo_t));
//        memset(&sib_type,0,sizeof(LTE_SIB_Type_t));
//	
//	schedulingInfo.si_Periodicity=LTE_SchedulingInfo__si_Periodicity_rf8;
//	sib_type=LTE_SIB_Type_sibType13_v920;
//	asn1cSeqAdd(&(*sib1)->schedulingInfoList.list,&schedulingInfo);
//    }
//
//    enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message,
//                                   NULL,
//                                   (void *)bcch_message,
//                                   buffer,
//                                   900);
//  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
//               enc_rval.failed_type->name, enc_rval.encoded);
//  LOG_W(RRC,"[eNB] SystemInformationBlockType1 Encoded %zd bits (%zd bytes) with new SIB13(%d) \n",enc_rval.encoded,(enc_rval.encoded+7)/8,find_sib13);
//
//
//  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB1 = ((enc_rval.encoded+7)/8);
//
//     
//  
//
//   return 0;
//}



static uint8_t rrc_M2AP_do_SIB23_SIB2(
  			  const protocol_ctxt_t *const ctxt_pP,
			  uint8_t Mod_id, 
			  int CC_id,
  			  const m2ap_mbms_scheduling_information_t *const m2ap_mbms_scheduling_information
){

  int i,j,l;
  
  eNB_RRC_INST *rrc = RC.rrc[ctxt_pP->module_id];
  rrc_eNB_carrier_data_t *carrier=&rrc->carrier[CC_id];


  //struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib13_part=NULL;
  LTE_MBSFN_SubframeConfigList_t *MBSFNSubframeConfigList/*,*MBSFNSubframeConfigList_copy*/;
  //LTE_MBSFN_AreaInfoList_r9_t *MBSFNArea_list/*,*MBSFNArea_list_copy*/;

 asn_enc_rval_t enc_rval;

 LTE_BCCH_DL_SCH_Message_t         *bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].systemInformation;
 uint8_t                       *buffer;
 LTE_SystemInformationBlockType2_t **sib2;


  if(ctxt_pP->brOption){
	buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23_BR;	
	sib2 	 =  &RC.rrc[Mod_id]->carrier[CC_id].sib2_BR;
        LOG_I(RRC,"Running SIB2/3 Encoding for eMTC\n");

  } else {
	buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23;	
	sib2 	 =  &RC.rrc[Mod_id]->carrier[CC_id].sib2;
  }



  if (bcch_message) {
    //memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_t));
  } else {
    LOG_E(RRC,"[eNB %d] BCCH_MESSAGE is null, exiting\n", Mod_id);
    exit(-1);
  }

  if (!sib2) {
    LOG_E(RRC,"[eNB %d] sib2 is null, exiting\n", Mod_id);
    exit(-1);
  }

  //if (!sib3) {
  //  LOG_E(RRC,"[eNB %d] sib3 is null, exiting\n", Mod_id);
  //  exit(-1);
  //}
 
 for (i=0; i < bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count; i++) {
    struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *typeandinfo;
    typeandinfo = bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.array[i];
    switch(typeandinfo->present) {
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_NOTHING: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib4: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib5: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib6: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib7: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib8: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib9: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib10: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib11: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib12_v920: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib14_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib15_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib16_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib17_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib18_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib19_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib20_v1310: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib21_v1430: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib24_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib25_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib26_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib26a_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib27_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib28_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib29_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2: 

	LOG_I(RRC,"Adding MBSFN subframe Configuration 1 to SIB2, %p %p\n",&typeandinfo->choice.sib2,*sib2);

	for(j=0; j < m2ap_mbms_scheduling_information->num_mbms_area_config_list ; j++) {

    			(&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList = CALLOC(1,sizeof(struct LTE_MBSFN_SubframeConfigList));
    			MBSFNSubframeConfigList = (&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList;

		for(l=0; l < m2ap_mbms_scheduling_information->mbms_area_config_list[j].num_mbms_sf_config_list; l++){
			LTE_MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig1;
      sib2_mbsfn_SubframeConfig1 = CALLOC(1, sizeof(*sib2_mbsfn_SubframeConfig1));

      sib2_mbsfn_SubframeConfig1->radioframeAllocationPeriod = m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].radioframe_allocation_period;
			sib2_mbsfn_SubframeConfig1->radioframeAllocationOffset = m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].radioframe_allocation_offset;


			if(m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].is_four_sf){
				LOG_I(RRC,"is_four_sf\n");
				sib2_mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames;

    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(3);
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 3;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 0;
  	                        sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[2] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation) & 0xFF);
                           	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[1] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation>>8) & 0xFF);
                           	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation>>16) & 0xFF);

			}else{
				LOG_I(RRC,"is_one_sf\n");
				sib2_mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 1;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 2;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=(m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation<<2);


			}

        		asn1cSeqAdd(&MBSFNSubframeConfigList->list,sib2_mbsfn_SubframeConfig1);
		}	
	}

	break;

    }
  }

  
 //xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message);

 enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)bcch_message,
                                   buffer,
                                   900);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  LOG_I(RRC,"[eNB] MBMS SIB2 SystemInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI encoding failed for SIB23\n");
    return(-1);
  }

  carrier->MBMS_flag = 1;

  rrc_mac_config_req_eNB_t tmp = {0};
  tmp.CC_id = CC_id;
  tmp.mbsfn_SubframeConfigList = carrier->sib2->mbsfn_SubframeConfigList;
  tmp.MBMS_Flag = carrier->MBMS_flag;
  rrc_mac_config_req_eNB(ctxt_pP->module_id, &tmp);

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB23 = ((enc_rval.encoded+7)/8);
 
  return 0;
}

static uint8_t rrc_M2AP_do_SIB23_SIB13(
  			  const protocol_ctxt_t *const ctxt_pP,
			  uint8_t Mod_id, 
			  int CC_id,
  			  const m2ap_setup_resp_t *const m2ap_setup_resp
){

  int i;
  eNB_RRC_INST *rrc = RC.rrc[ctxt_pP->module_id];
  rrc_eNB_carrier_data_t *carrier=&rrc->carrier[CC_id];

  //LTE_MBSFN_SubframeConfigList_t *MBSFNSubframeConfigList;
  LTE_MBSFN_AreaInfoList_r9_t *MBSFNArea_list;

 asn_enc_rval_t enc_rval;

 LTE_BCCH_DL_SCH_Message_t         *bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].systemInformation;
 uint8_t                       *buffer;
   LTE_SystemInformationBlockType2_t **sib2;


  if(ctxt_pP->brOption){
	buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23_BR;	
	sib2 	 =  &RC.rrc[Mod_id]->carrier[CC_id].sib2_BR;
    LOG_I(RRC,"Running SIB2/3 Encoding for eMTC\n");
  } else {
	buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23;	
	sib2 	 =  &RC.rrc[Mod_id]->carrier[CC_id].sib2;
  }

  if (bcch_message) {
    //memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_t));
  } else {
    LOG_E(RRC,"[eNB %d] BCCH_MESSAGE is null, exiting\n", Mod_id);
    exit(-1);
  }

  if (!sib2) {
    LOG_E(RRC,"[eNB %d] sib2 is null, exiting\n", Mod_id);
    exit(-1);
  }

  //if (!sib3) {
  //  LOG_E(RRC,"[eNB %d] sib3 is null, exiting\n", Mod_id);
  //  exit(-1);
  //}

 /*for (int i=0; i<(*si)->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count; i++) {
    struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *typeandinfo;
    typeandinfo = (*si)->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.array[i];
    switch(typeandinfo->present) {
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2:
	LTE_SystemInformationBlockType2_t *sib2 = &typeandinfo->choice.sib2
	 LOG_I(RRC,"Adding MBSFN subframe Configuration 1 to SIB2\n");
    	LTE_MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig1;
    	(*sib2)->mbsfn_SubframeConfigList = CALLOC(1,sizeof(struct LTE_MBSFN_SubframeConfigList));
    	MBSFNSubframeConfigList = (*sib2)->mbsfn_SubframeConfigList;
    	sib2_mbsfn_SubframeConfig1= CALLOC(1,sizeof(*sib2_mbsfn_SubframeConfig1));
    	memset((void *)sib2_mbsfn_SubframeConfig1,0,sizeof(*sib2_mbsfn_SubframeConfig1));
    	sib2_mbsfn_SubframeConfig1->radioframeAllocationPeriod= LTE_MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
    	sib2_mbsfn_SubframeConfig1->radioframeAllocationOffset= 1;
    	sib2_mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
    	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
    	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 1;
    	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 2;
    	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=0x38<<2;

        asn1cSeqAdd(&MBSFNSubframeConfigList->list,sib2_mbsfn_SubframeConfig1);


	break;

    }
  }*/

  
 LTE_SystemInformationBlockType13_r9_t   **sib13       = &RC.rrc[Mod_id]->carrier[CC_id].sib13;

  struct LTE_MBSFN_AreaInfo_r9 *MBSFN_Area1/*, *MBSFN_Area2*/;
  struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib13_part = CALLOC(1, sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  sib13_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920;

  *sib13 = &sib13_part->choice.sib13_v920;

  (*sib13)->notificationConfig_r9.notificationRepetitionCoeff_r9 = LTE_MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
  (*sib13)->notificationConfig_r9.notificationOffset_r9 = 0;
  (*sib13)->notificationConfig_r9.notificationSF_Index_r9 = 1;

  //  MBSFN-AreaInfoList
  MBSFNArea_list = &(*sib13)->mbsfn_AreaInfoList_r9; // CALLOC(1,sizeof(*MBSFNArea_list));
  memset(MBSFNArea_list, 0, sizeof(*MBSFNArea_list));

  for (i = 0; i < m2ap_setup_resp->num_mcch_config_per_mbsfn; i++) {
    // MBSFN Area 1
    MBSFN_Area1 = CALLOC(1, sizeof(*MBSFN_Area1));
    MBSFN_Area1->mbsfn_AreaId_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].mbsfn_area;
    MBSFN_Area1->non_MBSFNregionLength = m2ap_setup_resp->mcch_config_per_mbsfn[i].pdcch_length;
    MBSFN_Area1->notificationIndicator_r9 = 0;
    MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].repetition_period; // LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
    MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].offset; // in accordance with mbsfn subframe configuration in sib2
    MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].modification_period;

    //  Subframe Allocation Info
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf = MALLOC(1);
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size = 1;
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0] = m2ap_setup_resp->mcch_config_per_mbsfn[i].subframe_allocation_info << 2; // FDD: SF1
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused = 2;
    MBSFN_Area1->mcch_Config_r9.signallingMCS_r9 = m2ap_setup_resp->mcch_config_per_mbsfn[i].mcs;
    asn1cSeqAdd(&MBSFNArea_list->list, MBSFN_Area1);
  }

 asn1cSeqAdd(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list, sib13_part);

 //xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message);

 enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)bcch_message,
                                   buffer,
                                   900);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  LOG_I(RRC,"[eNB] MBMS SIB13 SystemInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI encoding failed for SIB23\n");
    return(-1);
  }

  rrc_mac_config_req_eNB_t tmp = {0};
  tmp.CC_id = CC_id;
  tmp.mbsfn_AreaInfoList = &carrier->sib13->mbsfn_AreaInfoList_r9;
  tmp.MBMS_Flag = carrier->MBMS_flag;
  rrc_mac_config_req_eNB(ctxt_pP->module_id, &tmp);

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB23 = ((enc_rval.encoded+7)/8);
 
  return 0;
}



static uint8_t rrc_M2AP_do_SIB23_SIB2_SIB13(
  			  const protocol_ctxt_t *const ctxt_pP,
			  uint8_t Mod_id, 
			  int CC_id,
  			  const m2ap_setup_resp_t *const m2ap_setup_resp,
  			  const m2ap_mbms_scheduling_information_t *const m2ap_mbms_scheduling_information
){

  int i,j,l;
  
  eNB_RRC_INST *rrc = RC.rrc[ctxt_pP->module_id];
  rrc_eNB_carrier_data_t *carrier=&rrc->carrier[CC_id];


  struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib13_part=NULL;
  LTE_MBSFN_SubframeConfigList_t *MBSFNSubframeConfigList/*,*MBSFNSubframeConfigList_copy*/;

 asn_enc_rval_t enc_rval;

 LTE_BCCH_DL_SCH_Message_t         *bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].systemInformation;
 uint8_t                       *buffer;
 LTE_SystemInformationBlockType2_t **sib2;

  LTE_MBSFN_AreaInfoList_r9_t *MBSFNArea_list/*,*MBSFNArea_list_copy*/;
 LTE_SystemInformationBlockType13_r9_t   **sib13       = &RC.rrc[Mod_id]->carrier[CC_id].sib13;
 struct LTE_MBSFN_AreaInfo_r9 *MBSFN_Area1;

  if(ctxt_pP->brOption){
	buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23_BR;	
	sib2 	 =  &RC.rrc[Mod_id]->carrier[CC_id].sib2_BR;
        LOG_I(RRC,"Running SIB2/3 Encoding for eMTC\n");

  } else {
	buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23;	
	sib2 	 =  &RC.rrc[Mod_id]->carrier[CC_id].sib2;
  }

  if (bcch_message) {
    //memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_t));
  } else {
    LOG_E(RRC,"[eNB %d] BCCH_MESSAGE is null, exiting\n", Mod_id);
    exit(-1);
  }

  if (!sib2) {
    LOG_E(RRC,"[eNB %d] sib2 is null, exiting\n", Mod_id);
    exit(-1);
  }

  if(!sib13){
    LOG_I(RRC,"[eNB %d] sib13 is null, it should get created\n",Mod_id);
  }

  //if (!sib3) {
  //  LOG_E(RRC,"[eNB %d] sib3 is null, exiting\n", Mod_id);
  //  exit(-1);
  //}
 
 for (i=0; i < bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count; i++) {
    struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *typeandinfo;
    typeandinfo = bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.array[i];
    switch(typeandinfo->present) {
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_NOTHING: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib4: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib5: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib6: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib7: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib8: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib9: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib10: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib11: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib12_v920: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920: 
	*sib13=&typeandinfo->choice.sib13_v920;
	(*sib13)->notificationConfig_r9.notificationRepetitionCoeff_r9=LTE_MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
 	(*sib13)->notificationConfig_r9.notificationOffset_r9=0;
 	(*sib13)->notificationConfig_r9.notificationSF_Index_r9=1;
	//  MBSFN-AreaInfoList
 	MBSFNArea_list= &(*sib13)->mbsfn_AreaInfoList_r9;//CALLOC(1,sizeof(*MBSFNArea_list));
 	memset(MBSFNArea_list,0,sizeof(*MBSFNArea_list));

	for( j=0; j < m2ap_setup_resp->num_mcch_config_per_mbsfn; j++){
 	   // MBSFN Area 1
 	   MBSFN_Area1= CALLOC(1, sizeof(*MBSFN_Area1));
 	   MBSFN_Area1->mbsfn_AreaId_r9= m2ap_setup_resp->mcch_config_per_mbsfn[j].mbsfn_area;
 	   MBSFN_Area1->non_MBSFNregionLength= m2ap_setup_resp->mcch_config_per_mbsfn[j].pdcch_length;
 	   MBSFN_Area1->notificationIndicator_r9= 0;
 	   MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9= m2ap_setup_resp->mcch_config_per_mbsfn[j].repetition_period;//LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
 	   MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9= m2ap_setup_resp->mcch_config_per_mbsfn[j].offset; // in accordance with mbsfn subframe configuration in sib2
 	   MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9= m2ap_setup_resp->mcch_config_per_mbsfn[j].modification_period;

 	   //  Subframe Allocation Info
 	   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf= MALLOC(1);
 	   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size= 1;
 	   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0]=m2ap_setup_resp->mcch_config_per_mbsfn[j].subframe_allocation_info<<2;  // FDD: SF1
 	   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused= 2;
 	   MBSFN_Area1->mcch_Config_r9.signallingMCS_r9= m2ap_setup_resp->mcch_config_per_mbsfn[j].mcs;
 	   asn1cSeqAdd(&MBSFNArea_list->list,MBSFN_Area1);
 	}
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib14_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib15_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib16_v1130: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib17_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib18_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib19_v1250: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib20_v1310: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib21_v1430: 
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib24_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib25_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib26_v1530:
	break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib26a_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib27_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib28_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib29_v1610:
        break;
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2: 

	LOG_I(RRC,"Adding MBSFN subframe Configuration 1 to SIB2, %p %p\n",&typeandinfo->choice.sib2,*sib2);

	for(j=0; j < m2ap_mbms_scheduling_information->num_mbms_area_config_list ; j++) {

    			(&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList = CALLOC(1,sizeof(struct LTE_MBSFN_SubframeConfigList));
    			MBSFNSubframeConfigList = (&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList;

		for(l=0; l < m2ap_mbms_scheduling_information->mbms_area_config_list[j].num_mbms_sf_config_list; l++){
			LTE_MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig1;
      sib2_mbsfn_SubframeConfig1 = CALLOC(1, sizeof(*sib2_mbsfn_SubframeConfig1));

      sib2_mbsfn_SubframeConfig1->radioframeAllocationPeriod = m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].radioframe_allocation_period;
			sib2_mbsfn_SubframeConfig1->radioframeAllocationOffset = m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].radioframe_allocation_offset;


			if(m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].is_four_sf){
				sib2_mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames;
				LOG_I(RRC,"rrc_M2AP_do_SIB23_SIB2_SIB13 is_four_sf\n");

    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(3);
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 3;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 0;
  	                        sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[2] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation) & 0xFF);
                           	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[1] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation>>8) & 0xFF);
                           	sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0] = ((m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation>>16) & 0xFF);

			}else{
				LOG_I(RRC,"rrc_M2AP_do_SIB23_SIB2_SIB13 rs_one_sf\n");
				sib2_mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 1;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 2;
    				sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=(m2ap_mbms_scheduling_information->mbms_area_config_list[j].mbms_sf_config_list[l].subframe_allocation<<2);


			}

        		asn1cSeqAdd(&MBSFNSubframeConfigList->list,sib2_mbsfn_SubframeConfig1);
		}	
	}

	break;

    }
  }

 if(*sib13==NULL){
   sib13_part = CALLOC(1, sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
   sib13_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920;
   *sib13 = &sib13_part->choice.sib13_v920;
   (*sib13)->notificationConfig_r9.notificationRepetitionCoeff_r9=LTE_MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
   (*sib13)->notificationConfig_r9.notificationOffset_r9=0;
   (*sib13)->notificationConfig_r9.notificationSF_Index_r9=1;

   //  MBSFN-AreaInfoList
   MBSFNArea_list= &(*sib13)->mbsfn_AreaInfoList_r9;//CALLOC(1,sizeof(*MBSFNArea_list));
   memset(MBSFNArea_list,0,sizeof(*MBSFNArea_list));

   for( i=0; i < m2ap_setup_resp->num_mcch_config_per_mbsfn; i++){
   // MBSFN Area 1
   MBSFN_Area1= CALLOC(1, sizeof(*MBSFN_Area1));
   MBSFN_Area1->mbsfn_AreaId_r9= m2ap_setup_resp->mcch_config_per_mbsfn[i].mbsfn_area;
   MBSFN_Area1->non_MBSFNregionLength= m2ap_setup_resp->mcch_config_per_mbsfn[i].pdcch_length;
   MBSFN_Area1->notificationIndicator_r9= 0;
   MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9= m2ap_setup_resp->mcch_config_per_mbsfn[i].repetition_period;//LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
   MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9= m2ap_setup_resp->mcch_config_per_mbsfn[i].offset; // in accordance with mbsfn subframe configuration in sib2
   MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9= m2ap_setup_resp->mcch_config_per_mbsfn[i].modification_period;
  
   //  Subframe Allocation Info
   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf= MALLOC(1);
   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size= 1;
   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0]=m2ap_setup_resp->mcch_config_per_mbsfn[i].subframe_allocation_info<<2;  // FDD: SF1
   MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused= 2;
   MBSFN_Area1->mcch_Config_r9.signallingMCS_r9= m2ap_setup_resp->mcch_config_per_mbsfn[i].mcs;
   asn1cSeqAdd(&MBSFNArea_list->list,MBSFN_Area1);
   }

 asn1cSeqAdd(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list, sib13_part);
 }

  
 //xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message);

 enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)bcch_message,
                                   buffer,
                                   900);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  LOG_I(RRC,"[eNB] MBMS SIB2 SystemInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI encoding failed for SIB23\n");
    return(-1);
  }

  carrier->MBMS_flag = 1;

  rrc_mac_config_req_eNB_t tmp = {0};
  tmp.CC_id = CC_id;
  tmp.mbsfn_SubframeConfigList = carrier->sib2->mbsfn_SubframeConfigList;
  tmp.MBMS_Flag = carrier->MBMS_flag;
  tmp.mbsfn_AreaInfoList = &carrier->sib13->mbsfn_AreaInfoList_r9;
  rrc_mac_config_req_eNB(ctxt_pP->module_id, &tmp);

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB23 = ((enc_rval.encoded+7)/8);
 
  return 0;
}


int
rrc_eNB_process_M2AP_SETUP_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  int CC_id,
  instance_t instance,
  const m2ap_setup_resp_t *const m2ap_setup_resp
)
{
 //protocol_ctxt_t 	ctxt;
 //LOG_W(RRC,"instance %d\n",instance);
 
 AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[0].SIB23 != NULL,"Memory fo SIB23 not allocated");
 AssertFatal(m2ap_setup_resp != NULL, "m2ap_setup_resp memory not allocated");


 pthread_mutex_lock(&RC.rrc[ctxt_pP->module_id]->cell_info_mutex);
 //RC.rrc[ctxt_pP->module_id]->carrier[CC_id].num_mbsfn_sync_area = m2ap_setup_resp->num_mcch_config_per_mbsfn;
 //rrc_M2AP_do_SIB23_SIB13(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_setup_resp); 
 m2ap_setup_resp_g = (m2ap_setup_resp_t*)calloc(1,sizeof(m2ap_setup_resp_t));
 memcpy(m2ap_setup_resp_g,m2ap_setup_resp,sizeof(m2ap_setup_resp_t));
 pthread_mutex_unlock(&RC.rrc[ctxt_pP->module_id]->cell_info_mutex);

 return 0;
}


int
rrc_eNB_process_M2AP_MBMS_SCHEDULING_INFORMATION(
  const protocol_ctxt_t *const ctxt_pP,
  int CC_id,
  instance_t instance,
  const m2ap_mbms_scheduling_information_t *const m2ap_mbms_scheduling_information
)
{
 //protocol_ctxt_t 	ctxt;
 //LOG_W(RRC,"instance %d\n",instance);
 
 AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[0].SIB23 != NULL,"Memory fo SIB23 not allocated");
 AssertFatal(m2ap_mbms_scheduling_information != NULL, "m2ap_mbms_scheduling_information memory not allocated");

 //RC.rrc[ctxt_pP->module_id]->carrier[CC_id].num_mbsfn_sync_area = m2ap_setup_resp->num_mcch_config_per_mbsfn;

 pthread_mutex_lock(&RC.rrc[ctxt_pP->module_id]->cell_info_mutex);
 m2ap_mbms_scheduling_information_g = (m2ap_mbms_scheduling_information_t*)calloc(1,sizeof(m2ap_mbms_scheduling_information_t));
 memcpy(m2ap_mbms_scheduling_information_g,m2ap_mbms_scheduling_information,sizeof(m2ap_mbms_scheduling_information_t));

 /*if(m2ap_setup_resp_g != NULL){
	 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].num_mbsfn_sync_area = m2ap_setup_resp_g->num_mcch_config_per_mbsfn;
	 rrc_M2AP_do_SIB23_SIB2(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_mbms_scheduling_information); 
	 rrc_M2AP_do_SIB23_SIB13(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_setup_resp_g); 
	 rrc_M2AP_init_MCCH(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_mbms_scheduling_information); 
	 rrc_M2AP_init_MBMS(ctxt_pP->module_id, CC_id, 0);
	 rrc_M2AP_openair_rrc_top_init_MBMS(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].MBMS_flag);
 }*/
 MessageDef      *msg_p;
 msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, M2AP_MBMS_SCHEDULING_INFORMATION_RESP);
 itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);

 pthread_mutex_unlock(&RC.rrc[ctxt_pP->module_id]->cell_info_mutex);


 return 0;
}

int
rrc_eNB_process_M2AP_MBMS_SESSION_START_REQ(
  const protocol_ctxt_t *const ctxt_pP,
  int CC_id,
  instance_t instance,
  const m2ap_session_start_req_t *const m2ap_mbms_session_start_req
)
{
 int split_cfg=0;
 pthread_mutex_lock(&RC.rrc[ctxt_pP->module_id]->cell_info_mutex);
 if(m2ap_setup_resp_g != NULL && m2ap_mbms_scheduling_information_g !=NULL ){
	 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].num_mbsfn_sync_area = m2ap_setup_resp_g->num_mcch_config_per_mbsfn;

	 //rrc_M2AP_do_SIB1(ctxt_pP,ctxt_pP->module_id,CC_id,NULL,NULL);
	if(split_cfg){
	 rrc_M2AP_do_SIB23_SIB2(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_mbms_scheduling_information_g); 
	 rrc_M2AP_do_SIB23_SIB13(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_setup_resp_g); 
	}else{
	   if(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].FeMBMS_flag){
		rrc_M2AP_do_SIB1_MBMS_SIB13(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_setup_resp_g,m2ap_mbms_scheduling_information_g);
	   }else{ 
	 	rrc_M2AP_do_SIB23_SIB2_SIB13(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_setup_resp_g,m2ap_mbms_scheduling_information_g); 
	   }
	}
	 rrc_M2AP_init_MCCH(ctxt_pP,ctxt_pP->module_id,CC_id,m2ap_mbms_scheduling_information_g); 
	 rrc_M2AP_init_MBMS(ctxt_pP->module_id, CC_id, 0);
	 rrc_M2AP_openair_rrc_top_init_MBMS(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].MBMS_flag);
 }
 pthread_mutex_unlock(&RC.rrc[ctxt_pP->module_id]->cell_info_mutex);

 MessageDef      *msg_p;
 msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, M2AP_MBMS_SESSION_START_RESP);
 itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);

 return 0;
}

int
rrc_eNB_process_M2AP_MBMS_SESSION_STOP_REQ(
  const protocol_ctxt_t *const ctxt_pP,
  const m2ap_session_stop_req_t *const m2ap_session_stop_req
)
{
 return 0;
}

int
rrc_eNB_process_M2AP_RESET(
  const protocol_ctxt_t *const ctxt_pP,
  const m2ap_reset_t *const m2ap_reset
)
{
 return 0;
}

int
rrc_eNB_process_M2AP_ENB_CONFIGURATION_UPDATE_ACK(
  const protocol_ctxt_t *const ctxt_pP,
  const m2ap_enb_configuration_update_ack_t *const m2ap_enb_configuration_update_ack
)
{
 return 0;
}

int
rrc_eNB_process_M2AP_ERROR_INDICATION(
  const protocol_ctxt_t *const ctxt_pP,
  const m2ap_error_indication_t *const m2ap_error_indication
)
{
 return 0;
}

int
rrc_eNB_process_M2AP_MBMS_SERVICE_COUNTING_REQ(
  const protocol_ctxt_t *const ctxt_pP,
  const m2ap_mbms_service_counting_req_t *const m2ap_mbms_service_counting_req
)
{
 return 0;
}

int
rrc_eNB_process_M2AP_MCE_CONFIGURATION_UPDATE(
  const protocol_ctxt_t *const ctxt_pP,
  const m2ap_mce_configuration_update_t *const m2ap_mce_configuration_update
)
{
 return 0;
}



void rrc_eNB_send_M2AP_MBMS_SCHEDULING_INFORMATION_RESP(
  const protocol_ctxt_t    *const ctxt_pP
  //,const rrc_eNB_mbms_context_t *const rrc_eNB_mbms_context
)
{
  MessageDef      *msg_p;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, M2AP_MBMS_SCHEDULING_INFORMATION_RESP);
  itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(ctxt_pP->instance), msg_p);
}

void rrc_eNB_send_M2AP_MBMS_SESSION_START_RESP(
  const protocol_ctxt_t    *const ctxt_pP
  //,const rrc_eNB_mbms_context_t *const rrc_eNB_mbms_context
)
{
  MessageDef      *msg_p;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, M2AP_MBMS_SESSION_START_RESP);
  itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(ctxt_pP->instance), msg_p);
}

void rrc_eNB_send_M2AP_MBMS_SESSION_STOP_RESP(
  const protocol_ctxt_t    *const ctxt_pP
  //,const rrc_eNB_mbms_context_t *const rrc_eNB_mbms_context
)
{
  MessageDef      *msg_p;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, M2AP_MBMS_SESSION_STOP_RESP);
  itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(ctxt_pP->instance), msg_p);
}

void rrc_eNB_send_M2AP_MBMS_SESSION_UPDATE_RESP(
  const protocol_ctxt_t    *const ctxt_pP
  //,const rrc_eNB_mbms_context_t *const rrc_eNB_mbms_context
)
{
  MessageDef      *msg_p;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, M2AP_MBMS_SESSION_UPDATE_RESP);
  itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(ctxt_pP->instance), msg_p);
}
