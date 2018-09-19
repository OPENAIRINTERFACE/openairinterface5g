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

// for SRB1_logicalChannelConfig_defaultValue
#include "rrc_extern.h"
#include "common/ran_context.h"

// undefine C_RNTI from
// openair1/PHY/LTE_TRANSPORT/transport_common.h which
// replaces in ie->value.choice.C_RNTI, causing
// a compile error

#undef C_RNTI 

extern f1ap_setup_req_t *f1ap_du_data;
extern RAN_CONTEXT_t RC;

/*  DL RRC Message Transfer */
int DU_handle_DL_RRC_MESSAGE_TRANSFER(instance_t       instance,
                                      uint32_t         assoc_id,
                                      uint32_t         stream,
                                      F1AP_F1AP_PDU_t *pdu) {

  printf("DU_handle_DL_RRC_MESSAGE_TRANSFER \n");
  
  MessageDef                     *message_p;
  F1AP_DLRRCMessageTransfer_t    *container;
  F1AP_DLRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  
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
  printf("cu_ue_f1ap_id %lu \n", cu_ue_f1ap_id);


  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  printf("du_ue_f1ap_id %lu \n", du_ue_f1ap_id);


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
  printf("srb_id %lu \n", srb_id);


  /* optional */
  /* ExecuteDuplication */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_ExecuteDuplication, true);
    executeDuplication = ie->value.choice.ExecuteDuplication;
    printf("ExecuteDuplication %d \n", executeDuplication);
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
    }
  }

  // decode RRC Container and act on the message type
  AssertFatal(srb_id<3,"illegal srb_id\n");

  if (srb_id == 0) {
    DL_CCCH_Message_t* dl_ccch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
			   &asn_DEF_DL_CCCH_Message,
			   (void**)&dl_ccch_msg,
			   ie->value.choice.RRCContainer.buf,
			   ie->value.choice.RRCContainer.size,0,0);
    switch (dl_ccch_msg->message.choice.c1.present) {
      
    case DL_CCCH_MessageType__c1_PR_NOTHING:
      LOG_I(RRC, "Received PR_NOTHING on DL-CCCH-Message\n");
      break;
      
    case DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishment:
      LOG_I(RRC,
	    "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishment\n");
      break;
      
    case DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishmentReject:
      LOG_I(RRC,
	    "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishmentReject\n");
      break;

    case DL_CCCH_MessageType__c1_PR_rrcConnectionReject:
      LOG_I(RRC,
	    "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReject \n");
      break;

    case DL_CCCH_MessageType__c1_PR_rrcConnectionSetup:
      {
	LOG_I(RRC,
	      "Logical Channel DL-CCCH (SRB0), Received RRCConnectionSetup RNTI %x\n",  (rnti_t)du_ue_f1ap_id);
	// Get configuration

	// find rrc instance and ue_context for this UE
	/*
	eNB_RRC_INST *rrc;
	rrc_eNB_ue_context_t *ue_context_pP = NULL;
	for (int i = 0; i<RC.nb_inst; i++) {
	  rrc = RC.rrc[i];
	  // note this requires that du_ue_f1ap_id == RNTI!
	  ue_context_pP = rrc_eNB_get_ue_context(rrc,(rnti_t)du_ue_f1ap_id);
	  if (ue_context_pP != NULL) break;
	}
	AssertFatal(ue_context_pP != NULL, "no UE context found!\n");
	*/

	// find MACRLC instance based on RNTI/UE_DU_ID
	int macrlc_instance=-1;

	for (macrlc_instance=0;macrlc_instance<RC.nb_macrlc_inst;macrlc_instance++)
	  if (find_UE_id(macrlc_instance,du_ue_f1ap_id)>=0) break;
	
	AssertFatal(macrlc_instance>=0,"cannot find macrlc instance for ue\n");

	RRCConnectionSetup_t* rrcConnectionSetup = &dl_ccch_msg->message.choice.c1.choice.rrcConnectionSetup;
	//	eNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
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
	}

        rrc_mac_config_req_eNB(
			       macrlc_instance,
			       0, //primaryCC_id,
			       0,0,0,0,0,
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
			       0,
#endif
			       (rnti_t) du_ue_f1ap_id, //rnti
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

    default:
      AssertFatal(1==0,
		  "Unknown message\n");
      break;
      }

    }
  }
  else if (srb_id == 1){ 

  }

  else if (srb_id == 2){

  }
  return 0;
  
}

//void DU_send_UL_RRC_MESSAGE_TRANSFER(F1AP_ULRRCMessageTransfer_t *ULRRCMessageTransfer) {
int DU_send_UL_RRC_MESSAGE_TRANSFER(void) {
  F1AP_F1AP_PDU_t                pdu;
  F1AP_ULRRCMessageTransfer_t    *out;
  F1AP_ULRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

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
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. SRBID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_SRBID;
  ie->value.choice.SRBID            = 1;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // issue in here
  /* mandatory */
  /* c4. RRCContainer */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
    return -1;
  }

  printf("\n");

  //du_f1ap_itti_send_sctp_data_req(instance, f1ap_setup_req->assoc_id, buffer, len, 0);
  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  //AssertFatal(1==0,"Not implemented yet\n");

  return 0;
}

/*  UL RRC Message Transfer */
int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(
  module_id_t     module_idP,
  int             CC_idP,
  int             UE_id,
  rnti_t          rntiP,
  uint8_t        *sduP,
  sdu_size_t      sdu_lenP
) {
  F1AP_F1AP_PDU_t                       pdu;
  F1AP_InitialULRRCMessageTransfer_t    *out;
  F1AP_InitialULRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

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
  ie->value.choice.GNB_DU_UE_F1AP_ID = F1AP_get_UE_identifier(module_idP, CC_idP, UE_id);
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
    printf("Failed to encode F1 setup request\n");
    return -1;
  }

  du_f1ap_itti_send_sctp_data_req(0, f1ap_du_data->assoc_id, buffer, len, 0);
  return 0;
}
