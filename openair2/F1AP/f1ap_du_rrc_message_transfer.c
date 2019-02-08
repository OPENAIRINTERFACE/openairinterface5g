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

/*! \file f1ap_du_rrc_message_transfer.c
 * \brief f1ap rrc message transfer for DU
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
#include "f1ap_decoder.h"
#include "f1ap_itti_messaging.h"

#include "f1ap_du_rrc_message_transfer.h"


#include "DL-CCCH-Message.h"
#include "DL-DCCH-Message.h"
#include "UL-DCCH-Message.h"

// for SRB1_logicalChannelConfig_defaultValue
#include "rrc_extern.h"
#include "common/ran_context.h"

#include "rrc_eNB_UE_context.h"

// undefine C_RNTI from
// openair1/PHY/LTE_TRANSPORT/transport_common.h which
// replaces in ie->value.choice.C_RNTI, causing
// a compile error

#undef C_RNTI 

extern f1ap_setup_req_t *f1ap_du_data;
extern RAN_CONTEXT_t RC;
extern f1ap_cudu_inst_t f1ap_du_inst[MAX_eNB];



/*  DL RRC Message Transfer */
int DU_handle_DL_RRC_MESSAGE_TRANSFER(instance_t       instance,
                                      uint32_t         assoc_id,
                                      uint32_t         stream,
                                      F1AP_F1AP_PDU_t *pdu) {
#ifndef UETARGET 
  LOG_D(DU_F1AP, "DU_handle_DL_RRC_MESSAGE_TRANSFER \n");
  
  F1AP_DLRRCMessageTransfer_t    *container;
  F1AP_DLRRCMessageTransferIEs_t *ie;

  uint64_t        cu_ue_f1ap_id;
  uint64_t        du_ue_f1ap_id;
  uint64_t        srb_id;
  int             executeDuplication;
  sdu_size_t      rrc_dl_sdu_len;
  uint64_t        subscriberProfileIDforRFP;
  uint64_t        rAT_FrequencySelectionPriority;

  DevAssert(pdu != NULL);

  if (stream != 0) {
    LOG_E(F1AP, "[SCTP %d] Received F1 on stream != 0 (%d)\n",
               assoc_id, stream);
    return -1;
  }

  container = &pdu->choice.initiatingMessage->value.choice.DLRRCMessageTransfer;


  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  cu_ue_f1ap_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  LOG_D(DU_F1AP, "cu_ue_f1ap_id %lu \n", cu_ue_f1ap_id);


  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(DU_F1AP, "du_ue_f1ap_id %lu associated with UE RNTI %x \n", 
                  du_ue_f1ap_id,   
                  f1ap_get_rnti_by_du_id(&f1ap_du_inst[instance], du_ue_f1ap_id)); // this should be the one transmitted via initial ul rrc message transfer

  if (f1ap_du_add_cu_ue_id(&f1ap_du_inst[instance],du_ue_f1ap_id, cu_ue_f1ap_id) < 0 ) {
    LOG_E(DU_F1AP, "Failed to find the F1AP UID \n");
    //return -1;
  }

  /* optional */
  /* oldgNB_DU_UE_F1AP_ID */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID, true);
  }

  /* mandatory */
  /* SRBID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBID, true);
  srb_id = ie->value.choice.SRBID;
  LOG_D(DU_F1AP, "srb_id %lu \n", srb_id);

  /* optional */
  /* ExecuteDuplication */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_ExecuteDuplication, true);
    executeDuplication = ie->value.choice.ExecuteDuplication;
    LOG_D(DU_F1AP, "ExecuteDuplication %d \n", executeDuplication);
  }

  // issue in here
  /* mandatory */
  /* RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);
  // BK: need check
  // create an ITTI message and copy SDU

  //  message_p = itti_alloc_new_message (TASK_CU_F1, RRC_MAC_CCCH_DATA_IND);
  //  memset (RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
  rrc_dl_sdu_len = ie->value.choice.RRCContainer.size;
  //  memcpy(RRC_MAC_CCCH_DATA_IND (message_p).sdu, ie->value.choice.RRCContainer.buf,
  //         ccch_sdu_len);
  printf ("RRCContainer :");
  for (int i=0;i<ie->value.choice.RRCContainer.size;i++) printf("%2x ",ie->value.choice.RRCContainer.buf[i]);
  printf("\n");

  /* optional */
  /* RAT_FrequencyPriorityInformation */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation, true);

    switch(ie->value.choice.RAT_FrequencyPriorityInformation.present) {
      case F1AP_RAT_FrequencyPriorityInformation_PR_subscriberProfileIDforRFP:
        subscriberProfileIDforRFP = ie->value.choice.RAT_FrequencyPriorityInformation.choice.subscriberProfileIDforRFP;
        break;
      case F1AP_RAT_FrequencyPriorityInformation_PR_rAT_FrequencySelectionPriority:
        rAT_FrequencySelectionPriority = ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority;
        break;
      default:
        LOG_W(DU_F1AP, "unhandled IE RAT_FrequencyPriorityInformation.present\n");
        break;
    }
  }

  // decode RRC Container and act on the message type
  AssertFatal(srb_id<3,"illegal srb_id\n");

  protocol_ctxt_t ctxt;
  ctxt.rnti      = f1ap_get_rnti_by_du_id(&f1ap_du_inst[instance], du_ue_f1ap_id);
  ctxt.module_id = instance;
  ctxt.instance  = instance;
  ctxt.enb_flag  = 1;

 struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(
                                                RC.rrc[ctxt.module_id],
                                                ctxt.rnti);

  if (srb_id == 0) {
    DL_CCCH_Message_t* dl_ccch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
         &asn_DEF_DL_CCCH_Message,
         (void**)&dl_ccch_msg,
         ie->value.choice.RRCContainer.buf,
         rrc_dl_sdu_len,0,0);
    switch (dl_ccch_msg->message.choice.c1.present) {

      case DL_CCCH_MessageType__c1_PR_NOTHING:
        LOG_I(DU_F1AP, "Received PR_NOTHING on DL-CCCH-Message\n");
        break;

      case DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishment:
        LOG_I(DU_F1AP,
        "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishment\n");
        break;

      case DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishmentReject:
        LOG_I(DU_F1AP,
        "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishmentReject\n");
        break;

      case DL_CCCH_MessageType__c1_PR_rrcConnectionReject:
        LOG_I(DU_F1AP,
        "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReject \n");
        break;

      case DL_CCCH_MessageType__c1_PR_rrcConnectionSetup:
      {
        LOG_I(DU_F1AP,
          "Logical Channel DL-CCCH (SRB0), Received RRCConnectionSetup DU_ID %lx/RNTI %x\n",
          du_ue_f1ap_id,
          f1ap_get_rnti_by_du_id(&f1ap_du_inst[instance], du_ue_f1ap_id));
          // Get configuration

        RRCConnectionSetup_t* rrcConnectionSetup = &dl_ccch_msg->message.choice.c1.choice.rrcConnectionSetup;
        AssertFatal(rrcConnectionSetup!=NULL,"rrcConnectionSetup is null\n");
        RadioResourceConfigDedicated_t* radioResourceConfigDedicated = &rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated;

        // get SRB logical channel information
        SRB_ToAddModList_t *SRB_configList;
        SRB_ToAddMod_t *SRB1_config;
        LogicalChannelConfig_t *SRB1_logicalChannelConfig;  //,*SRB2_logicalChannelConfig;
        SRB_configList                 = radioResourceConfigDedicated->srb_ToAddModList;

        AssertFatal(SRB_configList!=NULL,"SRB_configList is null\n");
        for (int cnt = 0; cnt < (SRB_configList)->list.count; cnt++) {
          if ((SRB_configList)->list.array[cnt]->srb_Identity == 1) {
            SRB1_config = (SRB_configList)->list.array[cnt];

            if (SRB1_config->logicalChannelConfig) {
              if (SRB1_config->logicalChannelConfig->present ==
                SRB_ToAddMod__logicalChannelConfig_PR_explicitValue) {
                SRB1_logicalChannelConfig = &SRB1_config->logicalChannelConfig->choice.explicitValue;
              } else {
                SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
              }
            } else {
              SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
            }
          }
        } // for
        rrc_rlc_config_asn1_req(&ctxt,
          SRB_configList,
          (DRB_ToAddModList_t*) NULL,
          (DRB_ToReleaseList_t*) NULL
#if (RRC_VERSION >= MAKE_VERSION(9, 0, 0))
          , (PMCH_InfoList_r9_t *) NULL,
          0,0
#   endif
          );

      // This should be somewhere in the f1ap_cudu_ue_inst_t
      /*int macrlc_instance = 0; 

      rnti_t rnti = f1ap_get_rnti_by_du_id(&f1ap_du_inst[0], du_ue_f1ap_id);
      struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[macrlc_instance],rnti);
      */  
      eNB_RRC_UE_t *ue_p = &ue_context_p->ue_context; 
      AssertFatal(ue_p->Srb0.Active == 1,"SRB0 is not active\n");

      memcpy((void*)ue_p->Srb0.Tx_buffer.Payload,
             (void*)ie->value.choice.RRCContainer.buf,
             rrc_dl_sdu_len); // ie->value.choice.RRCContainer.size

      ue_p->Srb0.Tx_buffer.payload_size = rrc_dl_sdu_len;

      rrc_mac_config_req_eNB(
          ctxt.module_id,
          0, //primaryCC_id,
          0,0,0,0,0,
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
          0,
#endif
          ctxt.rnti,
          (BCCH_BCH_Message_t *) NULL,
          (RadioResourceConfigCommonSIB_t *) NULL,
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
          (RadioResourceConfigCommonSIB_t *) NULL,
#endif
          radioResourceConfigDedicated->physicalConfigDedicated,
#if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
          (SCellToAddMod_r10_t *)NULL,
          //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
          (MeasObjectToAddMod_t **) NULL,
          radioResourceConfigDedicated->mac_MainConfig,
          1,
          SRB1_logicalChannelConfig,
          NULL, // measGapConfig,
          (TDD_Config_t *) NULL,
          NULL,
          (SchedulingInfoList_t *) NULL,
          0, NULL, NULL, (MBSFN_SubframeConfigList_t *) NULL
#if (RRC_VERSION >= MAKE_VERSION(9, 0, 0))
          , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
#endif
#if (RRC_VERSION >= MAKE_VERSION(13, 0, 0))
          ,
          (SystemInformationBlockType1_v1310_IEs_t *)NULL
#endif
          );
          break;
      } // case

      default:
        AssertFatal(1==0,
        "Unknown message\n");
        break;
    }// switch case
    return(0);
  } else if (srb_id == 1) { 

    DL_DCCH_Message_t* dl_dcch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
         &asn_DEF_DL_DCCH_Message,
         (void**)&dl_dcch_msg,
         &ie->value.choice.RRCContainer.buf[1], // buf[0] includes the pdcp header
         rrc_dl_sdu_len,0,0);
    
    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) 
      LOG_E(DU_F1AP," Failed to decode DL-DCCH (%zu bytes)\n",dec_rval.consumed);
    else
      LOG_D(DU_F1AP, "Received message: present %d and c1 present %d\n", 
        dl_dcch_msg->message.present, dl_dcch_msg->message.choice.c1.present);

    if (dl_dcch_msg->message.present == DL_DCCH_MessageType_PR_c1) {
     
      switch (dl_dcch_msg->message.choice.c1.present) {
	
      case DL_DCCH_MessageType__c1_PR_NOTHING:
        LOG_I(DU_F1AP, "Received PR_NOTHING on DL-DCCH-Message\n");
        return 0;
      case DL_DCCH_MessageType__c1_PR_dlInformationTransfer:
        LOG_I(DU_F1AP,"Received NAS DL Information Transfer\n");
        break;	
      case DL_DCCH_MessageType__c1_PR_csfbParametersResponseCDMA2000:
        LOG_I(DU_F1AP,"Received NAS sfbParametersResponseCDMA2000\n");
        break;  
      case DL_DCCH_MessageType__c1_PR_handoverFromEUTRAPreparationRequest:
        LOG_I(DU_F1AP,"Received NAS andoverFromEUTRAPreparationRequest\n");
        break;  
      case DL_DCCH_MessageType__c1_PR_mobilityFromEUTRACommand:
        LOG_I(DU_F1AP,"Received NAS mobilityFromEUTRACommand\n");
        break;
      case DL_DCCH_MessageType__c1_PR_rrcConnectionReconfiguration:
	     // handle RRCConnectionReconfiguration
        LOG_I(DU_F1AP,
	       "Logical Channel DL-DCCH (SRB1), Received RRCConnectionReconfiguration DU_ID %lx/RNTI %x\n",
	       du_ue_f1ap_id,
	       f1ap_get_rnti_by_du_id(&f1ap_du_inst[instance], du_ue_f1ap_id));
	
        RRCConnectionReconfiguration_t* rrcConnectionReconfiguration = &dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration;

        if (rrcConnectionReconfiguration->criticalExtensions.present == RRCConnectionReconfiguration__criticalExtensions_PR_c1) {
	        if (rrcConnectionReconfiguration->criticalExtensions.choice.c1.present ==
	         RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8) {
	          RRCConnectionReconfiguration_r8_IEs_t* rrcConnectionReconfiguration_r8 =
	          &rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8;
	    
            if (rrcConnectionReconfiguration_r8->mobilityControlInfo) {
	            LOG_I(DU_F1AP,"Mobility Control Information is present\n");
	            AssertFatal(1==0,"Can't handle this yet in DU\n");
	          }
	          if (rrcConnectionReconfiguration_r8->measConfig != NULL) {
	            LOG_I(DU_F1AP,"Measurement Configuration is present\n");
	          } 
	    
      	    if (rrcConnectionReconfiguration_r8->radioResourceConfigDedicated) {
      	      LOG_I(DU_F1AP,"Radio Resource Configuration is present\n");
      	      uint8_t DRB2LCHAN[8];
              long drb_id;
              int i;
      	      DRB_ToAddModList_t*                 DRB_configList = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->drb_ToAddModList;
              SRB_ToAddModList_t*                 SRB_configList = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->srb_ToAddModList;
              DRB_ToReleaseList_t*                DRB_ReleaseList = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->drb_ToReleaseList;
              MAC_MainConfig_t                    *mac_MainConfig = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->mac_MainConfig;
              MeasGapConfig_t                    *measGapConfig = NULL;
              struct PhysicalConfigDedicated**    physicalConfigDedicated = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->physicalConfigDedicated;
              rrc_rlc_config_asn1_req(
                &ctxt,
                SRB_configList, // NULL,  //LG-RK 14/05/2014 SRB_configList,
                DRB_configList,
                DRB_ReleaseList
      #if (RRC_VERSION >= MAKE_VERSION(9, 0, 0))
                , (PMCH_InfoList_r9_t *) NULL
                , 0, 0
      #endif
                );

              if (SRB_configList != NULL) {
                for (i = 0; (i < SRB_configList->list.count) && (i < 3); i++) {
                  if (SRB_configList->list.array[i]->srb_Identity == 1 ){
                    ue_context_p->ue_context.Srb1.Active=1;
                  }
                  else if (SRB_configList->list.array[i]->srb_Identity == 2 )  {
                    ue_context_p->ue_context.Srb2.Active=1;
                    ue_context_p->ue_context.Srb2.Srb_info.Srb_id=2;
                    LOG_I(DU_F1AP,"[DU %d] SRB2 is now active\n",ctxt.module_id);
                  } else {
                    LOG_W(DU_F1AP,"[DU %d] invalide SRB identity %ld\n",ctxt.module_id,
                   SRB_configList->list.array[i]->srb_Identity);
                  }
                }
              }

              if (DRB_configList != NULL) {
                for (i = 0; i < DRB_configList->list.count; i++) {  // num max DRB (11-3-8)
                  if (DRB_configList->list.array[i]) {
                    drb_id = (int)DRB_configList->list.array[i]->drb_Identity;
                    LOG_I(DU_F1AP,
                      "[DU %d] Logical Channel UL-DCCH, Received RRCConnectionReconfiguration for UE rnti %x, reconfiguring DRB %d/LCID %d\n",
                      ctxt.module_id,
                      ctxt.rnti,
                      (int)DRB_configList->list.array[i]->drb_Identity,
                      (int)*DRB_configList->list.array[i]->logicalChannelIdentity);

                  if (ue_context_p->ue_context.DRB_active[drb_id] == 0) {
                    ue_context_p->ue_context.DRB_active[drb_id] = 1;

                    if (DRB_configList->list.array[i]->logicalChannelIdentity) {
                      DRB2LCHAN[i] = (uint8_t) * DRB_configList->list.array[i]->logicalChannelIdentity;
                    }

                    rrc_mac_config_req_eNB(
                      ctxt.module_id,
                      0,0,0,0,0,0,
        #if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                   0,
        #endif
                   ue_context_p->ue_context.rnti,
                   (BCCH_BCH_Message_t *) NULL,
                   (RadioResourceConfigCommonSIB_t *) NULL,
        #if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                   (RadioResourceConfigCommonSIB_t *) NULL,
        #endif
                   physicalConfigDedicated,
        #if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
                   (SCellToAddMod_r10_t *)NULL,
                   //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
        #endif
                   (MeasObjectToAddMod_t **) NULL,
                   mac_MainConfig,
                   DRB2LCHAN[i],
                   DRB_configList->list.array[i]->logicalChannelConfig,
                   measGapConfig,
                   (TDD_Config_t *) NULL,
                   NULL,
                   (SchedulingInfoList_t *) NULL,
                   0, NULL, NULL, (MBSFN_SubframeConfigList_t *) NULL
        #if (RRC_VERSION >= MAKE_VERSION(9, 0, 0))
                   , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
        #endif
        #if (RRC_VERSION >= MAKE_VERSION(13, 0, 0))
                   ,
                   (SystemInformationBlockType1_v1310_IEs_t *)NULL
        #endif
                   );
                  }

                } else {        // remove LCHAN from MAC/PHY
                  AssertFatal(1==0,"Can't handle this yet in DU\n");  
                } 
        	     }
        	   }
           }
         }
       }
	    break;
  	  case DL_DCCH_MessageType__c1_PR_rrcConnectionRelease:
  	    // handle RRCConnectionRelease
            LOG_I(DU_F1AP,"Received RRCConnectionRelease\n");
  	    break;
  	  case DL_DCCH_MessageType__c1_PR_securityModeCommand:
         LOG_I(DU_F1AP,"Received securityModeCommand\n");
          break; 
  	  case DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
        LOG_I(DU_F1AP,"Received ueCapabilityEnquiry\n");
          break;
  	  case DL_DCCH_MessageType__c1_PR_counterCheck:
  #if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  	  case DL_DCCH_MessageType__c1_PR_loggedMeasurementConfiguration_r10:
  	  case DL_DCCH_MessageType__c1_PR_rnReconfiguration_r10:
  #endif
  	  case DL_DCCH_MessageType__c1_PR_spare1:
  	  case DL_DCCH_MessageType__c1_PR_spare2:
  	  case DL_DCCH_MessageType__c1_PR_spare3:
  #if (RRC_VERSION < MAKE_VERSION(14, 0, 0))
  	  case DL_DCCH_MessageType__c1_PR_spare4:
  #endif
  	    break;
      case DL_DCCH_MessageType__c1_PR_ueInformationRequest_r9:
        LOG_I(DU_F1AP, "Received ueInformationRequest_r9\n");
        break;
      case DL_DCCH_MessageType__c1_PR_rrcConnectionResume_r13:
        LOG_I(DU_F1AP, "Received rrcConnectionResume_r13\n");
	   } 
	 }	
  }
  else if (srb_id == 2) {
    
  }

  LOG_I(DU_F1AP, "Received DL RRC Transfer on srb_id %ld\n", srb_id);
  rlc_op_status_t    rlc_status;
  boolean_t          ret             = TRUE;
  mem_block_t       *pdcp_pdu_p      = NULL; 
  pdcp_pdu_p = get_free_mem_block(rrc_dl_sdu_len, __func__);
  memset(pdcp_pdu_p->data, 0, rrc_dl_sdu_len);
  memcpy(&pdcp_pdu_p->data[0], ie->value.choice.RRCContainer.buf, rrc_dl_sdu_len);
#ifdef DEBUG_MSG
  printf ("PRRCContainer size %d:", ie->value.choice.RRCContainer.size);
  for (int i=0;i<ie->value.choice.RRCContainer.size;i++) printf("%2x ",ie->value.choice.RRCContainer.buf[i]);
  printf("\n");

  printf ("PDCP PDU size %d:", rrc_dl_sdu_len);
  for (int i=0;i<rrc_dl_sdu_len;i++) printf("%2x ",pdcp_pdu_p->data[i]);
  printf("\n");
#endif 

    if (pdcp_pdu_p != NULL) {
      rlc_status = rlc_data_req(&ctxt
                                , 1
                                , MBMS_FLAG_NO
                                , srb_id
                                , 0
                                , 0
                                , rrc_dl_sdu_len
                                , pdcp_pdu_p
#ifdef Rel14
                                ,NULL
                                ,NULL
#endif
                                );
      switch (rlc_status) {
        case RLC_OP_STATUS_OK:
          LOG_I(DU_F1AP, "Data sending request over RLC succeeded!\n");
          ret=TRUE;
          break;

        case RLC_OP_STATUS_BAD_PARAMETER:
          LOG_W(DU_F1AP, "Data sending request over RLC failed with 'Bad Parameter' reason!\n");
          ret= FALSE;
          break;

        case RLC_OP_STATUS_INTERNAL_ERROR:
          LOG_W(DU_F1AP, "Data sending request over RLC failed with 'Internal Error' reason!\n");
          ret= FALSE;
          break;

        case RLC_OP_STATUS_OUT_OF_RESSOURCES:
          LOG_W(DU_F1AP, "Data sending request over RLC failed with 'Out of Resources' reason!\n");
          ret= FALSE;
          break;

        default:
          LOG_W(DU_F1AP, "RLC returned an unknown status code after PDCP placed the order to send some data (Status Code:%d)\n", rlc_status);
          ret= FALSE;
          break;
      } // switch case
      return ret; 
    } // if pdcp_pdu_p
  
#endif
  return 0;
  
}

int DU_send_UL_RRC_MESSAGE_TRANSFER(const protocol_ctxt_t* const ctxt_pP,
				    const rb_id_t     rb_idP,
				    const sdu_size_t  sdu_sizeP,
				    const uint8_t     *sdu_pP
                                    ) {


  rnti_t     rnti      = ctxt_pP->rnti;

  F1AP_F1AP_PDU_t                pdu;
  F1AP_ULRRCMessageTransfer_t    *out;
  F1AP_ULRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;


 LOG_I(DU_F1AP,"[DU %d] Received UL_RRC_MESSAGE_TRANSFER : size %d UE RNTI %x in SRB %d\n", 
        ctxt_pP->module_id, sdu_sizeP, rnti, rb_idP);

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_ULRRCMessageTransfer;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_ULRRCMessageTransfer;
  out = &pdu.choice.initiatingMessage->value.choice.ULRRCMessageTransfer;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_CU_UE_F1AP_ID;

  instance_t instance = ctxt_pP->module_id;

  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(&f1ap_du_inst[instance], rnti);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(&f1ap_du_inst[instance], rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. SRBID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_SRBID;
  ie->value.choice.SRBID            = rb_idP;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // issue in here
  /* mandatory */
  /* c4. RRCContainer */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, sdu_pP, sdu_sizeP);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (rb_idP == 1 || rb_idP == 2) { 
    struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(
                                                RC.rrc[ctxt_pP->module_id],
                                                rnti);

   
    UL_DCCH_Message_t* ul_dcch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
         &asn_DEF_UL_DCCH_Message,
         (void**)&ul_dcch_msg,
         &ie->value.choice.RRCContainer.buf[1], // buf[0] includes the pdcp header
         sdu_sizeP,0,0);
    
    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) 
      LOG_E(DU_F1AP," Failed to decode UL-DCCH (%zu bytes)\n",dec_rval.consumed);
    else
      LOG_I(DU_F1AP, "Received message: present %d and c1 present %d\n", 
        ul_dcch_msg->message.present, ul_dcch_msg->message.choice.c1.present);

    if (ul_dcch_msg->message.present == UL_DCCH_MessageType_PR_c1) {

      switch (ul_dcch_msg->message.choice.c1.present) {
      case UL_DCCH_MessageType__c1_PR_NOTHING:   /* No components present */
        break;

      case UL_DCCH_MessageType__c1_PR_csfbParametersRequestCDMA2000:
        break;

      case UL_DCCH_MessageType__c1_PR_measurementReport:
        break;

      case UL_DCCH_MessageType__c1_PR_rrcConnectionReconfigurationComplete:
        break;

      case UL_DCCH_MessageType__c1_PR_rrcConnectionReestablishmentComplete:
        break;

      case UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete:
        LOG_I(DU_F1AP,"[MSG] RRC UL rrcConnectionSetupComplete \n");
       if(!ue_context_p){
          LOG_E(DU_F1AP, "Did not find the UE context associated with UE RNTOI %x, ue_context_p is NULL\n", ctxt_pP->rnti);
        }else {
          LOG_I(DU_F1AP, "Processing RRCConnectionSetupComplete UE %x\n", rnti);
          ue_context_p->ue_context.Status = RRC_CONNECTED;
        }

        break;
      case UL_DCCH_MessageType__c1_PR_securityModeComplete:
         LOG_I(DU_F1AP,"[MSG] RRC securityModeComplete \n");
        break;

      case UL_DCCH_MessageType__c1_PR_securityModeFailure:
        break;

      case UL_DCCH_MessageType__c1_PR_ueCapabilityInformation:
           LOG_I(DU_F1AP,"[MSG] RRC ueCapabilityInformation \n");
        break;

      case UL_DCCH_MessageType__c1_PR_ulHandoverPreparationTransfer:
        break;

      case UL_DCCH_MessageType__c1_PR_ulInformationTransfer:
        LOG_I(DU_F1AP,"[MSG] RRC UL Information Transfer \n");
        break;

      case UL_DCCH_MessageType__c1_PR_counterCheckResponse:
        break;

#if (RRC_VERSION >= MAKE_VERSION(9, 0, 0))

      case UL_DCCH_MessageType__c1_PR_ueInformationResponse_r9:
        break;
      case UL_DCCH_MessageType__c1_PR_proximityIndication_r9:
       break;
#endif

#if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
      case UL_DCCH_MessageType__c1_PR_rnReconfigurationComplete_r10:
        break;

      case UL_DCCH_MessageType__c1_PR_mbmsCountingResponse_r10:
       break;

      case UL_DCCH_MessageType__c1_PR_interFreqRSTDMeasurementIndication_r10:
       break;
#endif

      }
    }
  }
    /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(DU_F1AP, "Failed to encode F1 setup request\n");
    return -1;
  }
  LOG_W(DU_F1AP, "DU_send_UL_RRC_MESSAGE_TRANSFER on SRB %d for UE %x \n", rb_idP, rnti);

  du_f1ap_itti_send_sctp_data_req(instance, f1ap_du_data->assoc_id, buffer, len, f1ap_du_data->default_sctp_stream_id);
  return 0;
}


/*  UL RRC Message Transfer */
int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(module_id_t     module_idP,
                                            int             CC_idP,
                                            int             UE_id,
                                            rnti_t          rntiP,
                                            uint8_t        *sduP,
                                            sdu_size_t      sdu_lenP) {
#ifndef UETARGET
  F1AP_F1AP_PDU_t                       pdu;
  F1AP_InitialULRRCMessageTransfer_t    *out;
  F1AP_InitialULRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int f1ap_uid = f1ap_add_ue (&f1ap_du_inst[module_idP], module_idP, CC_idP,UE_id, rntiP);

  if (f1ap_uid  < 0 ) {
    LOG_E(DU_F1AP, "Failed to add UE \n");
    return -1;
  }

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_InitialULRRCMessageTransfer;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_InitialULRRCMessageTransfer;
  out = &pdu.choice.initiatingMessage->value.choice.InitialULRRCMessageTransfer;
  

  /* mandatory */
  /* c1. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_InitialULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_InitialULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_du_inst[module_idP].f1ap_ue[f1ap_uid].du_ue_f1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. NRCGI */
  ie = (F1AP_InitialULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_InitialULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_NRCGI;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_NRCGI;

  F1AP_NRCGI_t nRCGI;
  MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[0], f1ap_du_data->mnc[0], f1ap_du_data->mnc_digit_length[0],
                                         &nRCGI.pLMN_Identity);
  NR_CELL_ID_TO_BIT_STRING(f1ap_du_data->nr_cellid[0], &nRCGI.nRCellIdentity);
  ie->value.choice.NRCGI = nRCGI;

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. C_RNTI */  // 16
  ie = (F1AP_InitialULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_InitialULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_C_RNTI;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_C_RNTI;
  C_RNTI_TO_BIT_STRING(rntiP, &ie->value.choice.C_RNTI);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c4. RRCContainer */
  ie = (F1AP_InitialULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_InitialULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_InitialULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, sduP, sdu_lenP);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. DUtoCURRCContainer */
  if (0) {
    ie = (F1AP_InitialULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_InitialULRRCMessageTransferIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCContainer;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_DUtoCURRCContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.DUtoCURRCContainer, "dummy_val",
                       strlen("dummy_val"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

    /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(DU_F1AP, "Failed to encode F1 setup request\n");
    return -1;
  }


  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_allocate_new_UE_context(RC.rrc[module_idP]);
  ue_context_p->ue_id_rnti                    = rntiP; 
  ue_context_p->ue_context.rnti               = rntiP;
  ue_context_p->ue_context.random_ue_identity = rntiP;
  ue_context_p->ue_context.Srb0.Active        = 1;
  RB_INSERT(rrc_ue_tree_s, &RC.rrc[module_idP]->rrc_ue_head, ue_context_p);
  du_f1ap_itti_send_sctp_data_req(module_idP, f1ap_du_data->assoc_id, buffer, len,  f1ap_du_data->default_sctp_stream_id);
#endif

  return 0;
}
    

void init_f1ap_du_ue_inst (void) {

   memset(f1ap_du_inst, 0, sizeof(f1ap_du_inst));
}


