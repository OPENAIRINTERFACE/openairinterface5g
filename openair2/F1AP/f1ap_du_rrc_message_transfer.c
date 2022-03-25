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


#include "LTE_DL-CCCH-Message.h"
#include "LTE_DL-DCCH-Message.h"
#include "LTE_UL-DCCH-Message.h"

#include "NR_DL-CCCH-Message.h"
#include "NR_UL-CCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_UL-DCCH-Message.h"
// for SRB1_logicalChannelConfig_defaultValue
#include "rrc_extern.h"
#include "common/ran_context.h"

#include "rrc_eNB_UE_context.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "asn1_msg.h"
#include "intertask_interface.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"

#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"

int DU_handle_DL_NR_RRC_MESSAGE_TRANSFER(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu);

/*  DL RRC Message Transfer */
int DU_handle_DL_RRC_MESSAGE_TRANSFER(instance_t       instance,
                                      uint32_t         assoc_id,
                                      uint32_t         stream,
                                      F1AP_F1AP_PDU_t *pdu) {
  if (RC.nrrrc && RC.nrrrc[instance]->node_type == ngran_gNB_DU) {
    LOG_I(F1AP, "node is gNB DU, call DU_handle_DL_NR_RRC_MESSAGE_TRANSFER \n");
    return DU_handle_DL_NR_RRC_MESSAGE_TRANSFER(instance, assoc_id, stream, pdu);
  }

  LOG_W(F1AP, "DU_handle_DL_RRC_MESSAGE_TRANSFER is a big race condition with rrc \n");
  F1AP_DLRRCMessageTransfer_t    *container;
  F1AP_DLRRCMessageTransferIEs_t *ie;
  uint64_t        cu_ue_f1ap_id;
  uint64_t        du_ue_f1ap_id;
  int             executeDuplication;
  sdu_size_t      rrc_dl_sdu_len;
  //uint64_t        subscriberProfileIDforRFP;
  //uint64_t        rAT_FrequencySelectionPriority;
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
  LOG_D(F1AP, "cu_ue_f1ap_id %lu \n", cu_ue_f1ap_id);
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(F1AP, "du_ue_f1ap_id %lu associated with UE RNTI %x \n",
        du_ue_f1ap_id,
        f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id));
  // this should be the one transmitted via initial ul rrc message transfer

  if (f1ap_du_add_cu_ue_id(instance,du_ue_f1ap_id, cu_ue_f1ap_id) < 0 ) {
    LOG_E(F1AP, "Failed to find the F1AP UID \n");
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
  uint64_t  srb_id = ie->value.choice.SRBID;
  LOG_D(F1AP, "srb_id %lu \n", srb_id);

  /* optional */
  /* ExecuteDuplication */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_ExecuteDuplication, true);
    executeDuplication = ie->value.choice.ExecuteDuplication;
    LOG_D(F1AP, "ExecuteDuplication %d \n", executeDuplication);
  }

  // issue in here
  /* mandatory */
  /* RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);
  rrc_dl_sdu_len = ie->value.choice.RRCContainer.size;

  /* optional */
  /* RAT_FrequencyPriorityInformation */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation, true);

    switch(ie->value.choice.RAT_FrequencyPriorityInformation.present) {
      case F1AP_RAT_FrequencyPriorityInformation_PR_eNDC:
        //subscriberProfileIDforRFP = ie->value.choice.RAT_FrequencyPriorityInformation.choice.subscriberProfileIDforRFP;
        break;

      case F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN:
        //rAT_FrequencySelectionPriority = ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority;
        break;

      default:
        LOG_W(F1AP, "unhandled IE RAT_FrequencyPriorityInformation.present\n");
        break;
    }
  }

  // decode RRC Container and act on the message type
  AssertFatal(srb_id<3,"illegal srb_id\n");
  protocol_ctxt_t ctxt;
  ctxt.rnti      = f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id);
  ctxt.instance = instance;
  ctxt.module_id = instance;
  ctxt.enb_flag  = 1;
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(
        RC.rrc[ctxt.instance],
        ctxt.rnti);

  if (srb_id == 0) {
    LTE_DL_CCCH_Message_t *dl_ccch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
                           &asn_DEF_LTE_DL_CCCH_Message,
                           (void **)&dl_ccch_msg,
                           ie->value.choice.RRCContainer.buf,
                           rrc_dl_sdu_len,0,0);
    AssertFatal(dec_rval.code == RC_OK, "could not decode F1AP message\n");

    switch (dl_ccch_msg->message.choice.c1.present) {
      case LTE_DL_CCCH_MessageType__c1_PR_NOTHING:
        LOG_I(F1AP, "Received PR_NOTHING on DL-CCCH-Message\n");
        break;

      case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishment:
        LOG_I(F1AP,
              "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishment\n");
        break;

      case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishmentReject:
        LOG_I(F1AP,
              "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishmentReject\n");
        break;

      case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReject:
        LOG_I(F1AP,
              "Logical Channel DL-CCCH (SRB0), Received RRCConnectionReject \n");
        break;

      case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionSetup: {
        LOG_I(F1AP,
              "Logical Channel DL-CCCH (SRB0), Received RRCConnectionSetup DU_ID %lx/RNTI %x\n",
              du_ue_f1ap_id,
              f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id));
        // Get configuration
        LTE_RRCConnectionSetup_t *rrcConnectionSetup = &dl_ccch_msg->message.choice.c1.choice.rrcConnectionSetup;
        AssertFatal(rrcConnectionSetup!=NULL,"rrcConnectionSetup is null\n");
        LTE_RadioResourceConfigDedicated_t *radioResourceConfigDedicated = &rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated;
        // get SRB logical channel information
        LTE_SRB_ToAddModList_t *SRB_configList = radioResourceConfigDedicated->srb_ToAddModList;
        AssertFatal(SRB_configList!=NULL,"SRB_configList is null\n");
        LTE_LogicalChannelConfig_t *SRB1_logicalChannelConfig = NULL;

        for (int cnt = 0; cnt < (SRB_configList)->list.count; cnt++) {
          if ((SRB_configList)->list.array[cnt]->srb_Identity == 1) {
           LTE_SRB_ToAddMod_t * SRB1_config = (SRB_configList)->list.array[cnt];

            if (SRB1_config->logicalChannelConfig) {
              if (SRB1_config->logicalChannelConfig->present ==
                  LTE_SRB_ToAddMod__logicalChannelConfig_PR_explicitValue) {
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
                                (LTE_DRB_ToAddModList_t *) NULL,
                                (LTE_DRB_ToReleaseList_t *) NULL
                                , (LTE_PMCH_InfoList_r9_t *) NULL,
                                0,0
                               );
        // This should be somewhere in the f1ap_cudu_ue_inst_t
        /*int macrlc_instance = 0;

        rnti_t rnti = f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id);
        struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[macrlc_instance],rnti);
        */
        eNB_RRC_UE_t *ue_p = &ue_context_p->ue_context;
        AssertFatal(ue_p->Srb0.Active == 1,"SRB0 is not active\n");
        memcpy((void *)ue_p->Srb0.Tx_buffer.Payload,
               (void *)ie->value.choice.RRCContainer.buf,
               rrc_dl_sdu_len); // ie->value.choice.RRCContainer.size
        ue_p->Srb0.Tx_buffer.payload_size = rrc_dl_sdu_len;
        LTE_MAC_MainConfig_t    *mac_MainConfig  = NULL;

        if (radioResourceConfigDedicated->mac_MainConfig)
          mac_MainConfig = &radioResourceConfigDedicated->mac_MainConfig->choice.explicitValue;

        rrc_mac_config_req_eNB(
          ctxt.instance,
          0, //primaryCC_id,
          0,0,0,0,0,0,
          ctxt.rnti,
          (LTE_BCCH_BCH_Message_t *) NULL,
          (LTE_RadioResourceConfigCommonSIB_t *) NULL,
          (LTE_RadioResourceConfigCommonSIB_t *) NULL,
          radioResourceConfigDedicated->physicalConfigDedicated,
          (LTE_SCellToAddMod_r10_t *)NULL,
          //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
          (LTE_MeasObjectToAddMod_t **) NULL,
          mac_MainConfig,
          1,
          SRB1_logicalChannelConfig,
          NULL, // measGapConfig,
          (LTE_TDD_Config_t *) NULL,
          NULL,
          (LTE_SchedulingInfoList_t *) NULL,
          0, NULL, NULL, (LTE_MBSFN_SubframeConfigList_t *) NULL
          , 0, (LTE_MBSFN_AreaInfoList_r9_t *) NULL, (LTE_PMCH_InfoList_r9_t *) NULL,
          (LTE_SystemInformationBlockType1_v1310_IEs_t *)NULL
          ,
          0,
          (LTE_BCCH_DL_SCH_Message_MBMS_t *) NULL,
          (LTE_SchedulingInfo_MBMS_r14_t *) NULL,
          (struct LTE_NonMBSFN_SubframeConfig_r14 *) NULL,
          (LTE_SystemInformationBlockType1_MBMS_r14_t *) NULL,
          (LTE_MBSFN_AreaInfoList_r9_t *) NULL,
          (LTE_MBSFNAreaConfiguration_r9_t *) NULL
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
    LTE_DL_DCCH_Message_t *dl_dcch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
                           &asn_DEF_LTE_DL_DCCH_Message,
                           (void **)&dl_dcch_msg,
                           &ie->value.choice.RRCContainer.buf[1], // buf[0] includes the pdcp header
                           rrc_dl_sdu_len,0,0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0))
      LOG_E(F1AP," Failed to decode DL-DCCH (%zu bytes)\n",dec_rval.consumed);
    else
      LOG_D(F1AP, "Received message: present %d and c1 present %d\n",
            dl_dcch_msg->message.present, dl_dcch_msg->message.choice.c1.present);

    if (dl_dcch_msg->message.present == LTE_DL_DCCH_MessageType_PR_c1) {
      switch (dl_dcch_msg->message.choice.c1.present) {
        case LTE_DL_DCCH_MessageType__c1_PR_NOTHING:
          LOG_I(F1AP, "Received PR_NOTHING on DL-DCCH-Message\n");
          return 0;

        case LTE_DL_DCCH_MessageType__c1_PR_dlInformationTransfer:
          LOG_I(F1AP,"Received NAS DL Information Transfer\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_csfbParametersResponseCDMA2000:
          LOG_I(F1AP,"Received NAS sfbParametersResponseCDMA2000\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_handoverFromEUTRAPreparationRequest:
          LOG_I(F1AP,"Received NAS andoverFromEUTRAPreparationRequest\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_mobilityFromEUTRACommand:
          LOG_I(F1AP,"Received NAS mobilityFromEUTRACommand\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionReconfiguration:
          // handle RRCConnectionReconfiguration
          LOG_I(F1AP,
                "Logical Channel DL-DCCH (SRB1), Received RRCConnectionReconfiguration DU_ID %lx/RNTI %x\n",
                du_ue_f1ap_id,
                f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id));
          LTE_RRCConnectionReconfiguration_t *rrcConnectionReconfiguration = &dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration;

          if (rrcConnectionReconfiguration->criticalExtensions.present == LTE_RRCConnectionReconfiguration__criticalExtensions_PR_c1) {
            if (rrcConnectionReconfiguration->criticalExtensions.choice.c1.present ==
                LTE_RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8) {
              LTE_RRCConnectionReconfiguration_r8_IEs_t *rrcConnectionReconfiguration_r8 =
                &rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8;

              if (rrcConnectionReconfiguration_r8->mobilityControlInfo) {
                LOG_I(F1AP, "Mobility Control Information is present\n");
                AssertFatal(1==0,"Can't handle this yet in DU\n");
              }

              if (rrcConnectionReconfiguration_r8->measConfig != NULL) {
                LOG_I(F1AP, "Measurement Configuration is present\n");
              }

              if (rrcConnectionReconfiguration_r8->radioResourceConfigDedicated) {
                LOG_I(F1AP, "Radio Resource Configuration is present\n");
                uint8_t DRB2LCHAN[8];
                long drb_id;
                int i;
                LTE_DRB_ToAddModList_t  *DRB_configList  = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->drb_ToAddModList;
                LTE_SRB_ToAddModList_t  *SRB_configList  = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->srb_ToAddModList;
                LTE_DRB_ToReleaseList_t *DRB_ReleaseList = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->drb_ToReleaseList;
                LTE_MAC_MainConfig_t    *mac_MainConfig  = NULL;

                for (i = 0; i< 8; i++) {
                  DRB2LCHAN[i] = 0;
                }

                if (rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->mac_MainConfig) {
                  LOG_D(F1AP, "MAC Main Configuration is present\n");
                  mac_MainConfig = &rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->mac_MainConfig->choice.explicitValue;

                  /* CDRX Configuration */
                  if (mac_MainConfig->drx_Config == NULL) {
                    LOG_W(F1AP, "drx_Configuration parameter is NULL, cannot configure local UE parameters or CDRX is deactivated\n");
                  } else {
                    MessageDef *message_p = NULL;
                    /* Send DRX configuration to MAC task to configure timers of local UE context */
                    message_p = itti_alloc_new_message(TASK_DU_F1, 0, RRC_MAC_DRX_CONFIG_REQ);
                    RRC_MAC_DRX_CONFIG_REQ(message_p).rnti = ctxt.rnti;
                    RRC_MAC_DRX_CONFIG_REQ(message_p).drx_Configuration = mac_MainConfig->drx_Config;
                    itti_send_msg_to_task(TASK_MAC_ENB, ctxt.instance, message_p);
                    LOG_D(F1AP, "DRX configured in MAC Main Configuration for RRC Connection Reconfiguration\n");
                  }

                  /* End of CDRX configuration */
                }

                LTE_MeasGapConfig_t     *measGapConfig   = NULL;
                struct LTE_PhysicalConfigDedicated *physicalConfigDedicated = rrcConnectionReconfiguration_r8->radioResourceConfigDedicated->physicalConfigDedicated;
                rrc_rlc_config_asn1_req(
                  &ctxt,
                  SRB_configList, // NULL,  //LG-RK 14/05/2014 SRB_configList,
                  DRB_configList,
                  DRB_ReleaseList, (LTE_PMCH_InfoList_r9_t *) NULL, 0, 0
                );

                if (SRB_configList != NULL) {
                  for (i = 0; (i < SRB_configList->list.count) && (i < 3); i++) {
                    if (SRB_configList->list.array[i]->srb_Identity == 1 ) {
                      ue_context_p->ue_context.Srb1.Active=1;
                    } else if (SRB_configList->list.array[i]->srb_Identity == 2 )  {
                      ue_context_p->ue_context.Srb2.Active=1;
                      ue_context_p->ue_context.Srb2.Srb_info.Srb_id=2;
                      LOG_I(F1AP, "[DU %ld] SRB2 is now active\n",ctxt.instance);
                    } else {
                      LOG_W(F1AP, "[DU %ld] invalid SRB identity %ld\n",ctxt.instance,
                            SRB_configList->list.array[i]->srb_Identity);
                    }
                  }
                }

                if (DRB_configList != NULL) {
                  for (i = 0; i < DRB_configList->list.count; i++) {  // num max DRB (11-3-8)
                    if (DRB_configList->list.array[i]) {
                      drb_id = (int)DRB_configList->list.array[i]->drb_Identity;
                      LOG_I(F1AP,
                            "[DU %ld] Logical Channel UL-DCCH, Received RRCConnectionReconfiguration for UE rnti %x, reconfiguring DRB %d/LCID %d\n",
                            ctxt.instance,
                            ctxt.rnti,
                            (int)DRB_configList->list.array[i]->drb_Identity,
                            (int)*DRB_configList->list.array[i]->logicalChannelIdentity);

                      if (ue_context_p->ue_context.DRB_active[drb_id] == 0) {
                        ue_context_p->ue_context.DRB_active[drb_id] = 1;

                        if (DRB_configList->list.array[i]->logicalChannelIdentity) {
                          DRB2LCHAN[i] = (uint8_t) * DRB_configList->list.array[i]->logicalChannelIdentity;
                        }

                        rrc_mac_config_req_eNB(
                          ctxt.instance,
                          0,0,0,0,0,0,
                          0,
                          ue_context_p->ue_context.rnti,
                          (LTE_BCCH_BCH_Message_t *) NULL,
                          (LTE_RadioResourceConfigCommonSIB_t *) NULL,
                          (LTE_RadioResourceConfigCommonSIB_t *) NULL,
                          physicalConfigDedicated,
                          (LTE_SCellToAddMod_r10_t *)NULL,
                          //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
                          (LTE_MeasObjectToAddMod_t **) NULL,
                          mac_MainConfig,
                          DRB2LCHAN[i],
                          DRB_configList->list.array[i]->logicalChannelConfig,
                          measGapConfig,
                          (LTE_TDD_Config_t *) NULL,
                          NULL,
                          (LTE_SchedulingInfoList_t *) NULL,
                          0, NULL, NULL, (LTE_MBSFN_SubframeConfigList_t *) NULL
                          , 0, (LTE_MBSFN_AreaInfoList_r9_t *) NULL, (LTE_PMCH_InfoList_r9_t *) NULL,
                          (LTE_SystemInformationBlockType1_v1310_IEs_t *)NULL,
                          0,
                          (LTE_BCCH_DL_SCH_Message_MBMS_t *) NULL,
                          (LTE_SchedulingInfo_MBMS_r14_t *) NULL,
                          (struct LTE_NonMBSFN_SubframeConfig_r14 *) NULL,
                          (LTE_SystemInformationBlockType1_MBMS_r14_t *) NULL,
                          (LTE_MBSFN_AreaInfoList_r9_t *) NULL,
                          (LTE_MBSFNAreaConfiguration_r9_t *) NULL
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

        case LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionRelease:
          // handle RRCConnectionRelease
          LOG_I(F1AP, "Received RRCConnectionRelease\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_securityModeCommand:
          LOG_I(F1AP, "Received securityModeCommand\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
          LOG_I(F1AP, "Received ueCapabilityEnquiry\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_counterCheck:
        case LTE_DL_DCCH_MessageType__c1_PR_loggedMeasurementConfiguration_r10:
        case LTE_DL_DCCH_MessageType__c1_PR_rnReconfiguration_r10:
        case LTE_DL_DCCH_MessageType__c1_PR_spare1:
        case LTE_DL_DCCH_MessageType__c1_PR_spare2:
        case LTE_DL_DCCH_MessageType__c1_PR_spare3:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_ueInformationRequest_r9:
          LOG_I(F1AP, "Received ueInformationRequest_r9\n");
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionResume_r13:
          LOG_I(F1AP, "Received rrcConnectionResume_r13\n");
      }
    }
  } else if (srb_id == 2) {
  }

  LOG_I(F1AP, "Received DL RRC Transfer on srb_id %ld\n", srb_id);
  rlc_op_status_t    rlc_status;
  bool               ret             = true;
  mem_block_t       *pdcp_pdu_p      = NULL;
  pdcp_pdu_p = get_free_mem_block(rrc_dl_sdu_len, __func__);

  //LOG_I(F1AP, "PRRCContainer size %lu:", ie->value.choice.RRCContainer.size);
  //for (int i = 0; i < ie->value.choice.RRCContainer.size; i++)
  //  printf("%02x ", ie->value.choice.RRCContainer.buf[i]);

  //printf (", PDCP PDU size %d:", rrc_dl_sdu_len);
  //for (int i=0;i<rrc_dl_sdu_len;i++) printf("%2x ",pdcp_pdu_p->data[i]);
  //printf("\n");

  if (pdcp_pdu_p != NULL) {
    memset(pdcp_pdu_p->data, 0, rrc_dl_sdu_len);
    memcpy(&pdcp_pdu_p->data[0], ie->value.choice.RRCContainer.buf, rrc_dl_sdu_len);
    rlc_status = rlc_data_req(&ctxt
                              , 1
                              , MBMS_FLAG_NO
                              , srb_id
                              , 0
                              , 0
                              , rrc_dl_sdu_len
                              , pdcp_pdu_p
                              ,NULL
                              ,NULL
                             );

    switch (rlc_status) {
      case RLC_OP_STATUS_OK:
        //LOG_I(F1AP, "Data sending request over RLC succeeded!\n");
        ret=true;
        break;

      case RLC_OP_STATUS_BAD_PARAMETER:
        LOG_W(F1AP, "Data sending request over RLC failed with 'Bad Parameter' reason!\n");
        ret= false;
        break;

      case RLC_OP_STATUS_INTERNAL_ERROR:
        LOG_W(F1AP, "Data sending request over RLC failed with 'Internal Error' reason!\n");
        ret= false;
        break;

      case RLC_OP_STATUS_OUT_OF_RESSOURCES:
        LOG_W(F1AP, "Data sending request over RLC failed with 'Out of Resources' reason!\n");
        ret= false;
        break;

      default:
        LOG_W(F1AP, "RLC returned an unknown status code after PDCP placed the order to send some data (Status Code:%d)\n", rlc_status);
        ret= false;
        break;
    } // switch case

    return ret;
  } // if pdcp_pdu_p

  return 0;
}

int DU_send_UL_RRC_MESSAGE_TRANSFER(instance_t instance,
                                    const f1ap_ul_rrc_message_t *msg) {
  const rnti_t rnti = msg->rnti;
  F1AP_F1AP_PDU_t                pdu;
  F1AP_ULRRCMessageTransfer_t    *out;
  F1AP_ULRRCMessageTransferIEs_t *ie;
  uint8_t *buffer = NULL;
  uint32_t len;
  LOG_I(F1AP, "[DU %ld] %s: size %d UE RNTI %x in SRB %d\n",
        instance, __func__, msg->rrc_container_length, rnti, msg->srb_id);
  //LOG_I(F1AP, "%s() RRCContainer size %d: ", __func__, msg->rrc_container_length);
  //for (int i = 0;i < msg->rrc_container_length; i++)
  //  printf("%02x ", msg->rrc_container[i]);
  //printf("\n");
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
  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(DUtype, instance, rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(DUtype, instance, rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c3. SRBID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_SRBID;
  ie->value.choice.SRBID            = msg->srb_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  // issue in here
  /* mandatory */
  /* c4. RRCContainer */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer,
                       (const char *) msg->rrc_container,
                       msg->rrc_container_length);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (msg->srb_id == 1 || msg->srb_id == 2) {
    struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[instance], rnti);
    LTE_UL_DCCH_Message_t *ul_dcch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
                           &asn_DEF_LTE_UL_DCCH_Message,
                           (void **)&ul_dcch_msg,
                           &ie->value.choice.RRCContainer.buf[1], // buf[0] includes the pdcp header
                           msg->rrc_container_length, 0, 0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0))
      LOG_E(F1AP, " Failed to decode UL-DCCH (%zu bytes)\n",dec_rval.consumed);
    else
      LOG_I(F1AP, "Received message: present %d and c1 present %d\n",
            ul_dcch_msg->message.present, ul_dcch_msg->message.choice.c1.present);

    if (ul_dcch_msg->message.present == LTE_UL_DCCH_MessageType_PR_c1) {
      switch (ul_dcch_msg->message.choice.c1.present) {
        case LTE_UL_DCCH_MessageType__c1_PR_NOTHING:   /* No components present */
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_csfbParametersRequestCDMA2000:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_measurementReport:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionReconfigurationComplete:
          LOG_I(F1AP, "[MSG] RRC UL rrcConnectionReconfigurationComplete\n");
          /* CDRX: activated when RRC Connection Reconfiguration Complete is received */
          int UE_id_mac = find_UE_id(instance, rnti);

          if (UE_id_mac == -1) {
            LOG_E(F1AP, "Can't find UE_id(MAC) of UE rnti %x\n", rnti);
            break;
          }

          UE_sched_ctrl_t *UE_scheduling_control = &(RC.mac[instance]->UE_info.UE_sched_ctrl[UE_id_mac]);

          if (UE_scheduling_control->cdrx_waiting_ack == true) {
            UE_scheduling_control->cdrx_waiting_ack = false;
            UE_scheduling_control->cdrx_configured = true; // Set to TRUE when RRC Connection Reconfiguration Complete is received
            LOG_I(F1AP, "CDRX configuration activated after RRC Connection Reconfiguration Complete reception\n");
          }

          /* End of CDRX processing */
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionReestablishmentComplete:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete:
          LOG_I(F1AP, "[MSG] RRC UL rrcConnectionSetupComplete \n");

          if(!ue_context_p) {
            LOG_E(F1AP, "Did not find the UE context associated with UE RNTOI %x, ue_context_p is NULL\n", rnti);
          } else {
            LOG_I(F1AP, "Processing RRCConnectionSetupComplete UE %x\n", rnti);
            ue_context_p->ue_context.StatusRrc = RRC_CONNECTED;
          }

          break;

        case LTE_UL_DCCH_MessageType__c1_PR_securityModeComplete:
          LOG_I(F1AP, "[MSG] RRC securityModeComplete \n");
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_securityModeFailure:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation:
          LOG_I(F1AP, "[MSG] RRC ueCapabilityInformation \n");
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ulHandoverPreparationTransfer:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ulInformationTransfer:
          LOG_I(F1AP,"[MSG] RRC UL Information Transfer \n");
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_counterCheckResponse:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ueInformationResponse_r9:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_proximityIndication_r9:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rnReconfigurationComplete_r10:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_mbmsCountingResponse_r10:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_interFreqRSTDMeasurementIndication_r10:
          break;
      }
    }
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UL RRC MESSAGE TRANSFER\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(false, instance, buffer, len, getCxt(DUtype, instance)->default_sctp_stream_id);
  return 0;
}

/*  UL RRC Message Transfer */
int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(instance_t     instanceP,
    int             CC_idP,
    int             UE_id,
    rnti_t          rntiP,
    const uint8_t   *sduP,
    sdu_size_t      sdu_lenP,
    const char      *sdu2P,
    sdu_size_t      sdu2_lenP) {
  F1AP_F1AP_PDU_t                       pdu= {0};
  F1AP_InitialULRRCMessageTransfer_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  int f1ap_uid = f1ap_add_ue (DUtype, instanceP, rntiP);

  if (f1ap_uid  < 0 ) {
    LOG_E(F1AP, "Failed to add UE \n");
    return -1;
  }

  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_InitialULRRCMessageTransfer;
  tmp->criticality   = F1AP_Criticality_ignore;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_InitialULRRCMessageTransfer;
  out = &tmp->value.choice.InitialULRRCMessageTransfer;
  /* mandatory */
  /* c1. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie1->value.choice.GNB_DU_UE_F1AP_ID = getCxt(DUtype, instanceP)->f1ap_ue[f1ap_uid].du_ue_f1ap_id;
  /* mandatory */
  /* c2. NRCGI */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_NRCGI;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_NRCGI;
  //Fixme: takes always the first cell
  addnRCGI(ie2->value.choice.NRCGI, getCxt(DUtype, instanceP)->setupReq.cell);
  /* mandatory */
  /* c3. C_RNTI */  // 16
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_C_RNTI;
  ie3->criticality                    = F1AP_Criticality_reject;
  ie3->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_C_RNTI;
  ie3->value.choice.C_RNTI=rntiP;
  /* mandatory */
  /* c4. RRCContainer */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie4);
  ie4->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie4->criticality                   = F1AP_Criticality_reject;
  ie4->value.present                 = F1AP_InitialULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie4->value.choice.RRCContainer, (const char *)sduP, sdu_lenP);

  /* optional */
  /* c5. DUtoCURRCContainer */
  if (sdu2P) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCContainer;
    ie5->criticality                    = F1AP_Criticality_reject;
    ie5->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_DUtoCURRCContainer;
    OCTET_STRING_fromBuf(&ie5->value.choice.DUtoCURRCContainer,
                         sdu2P,
                         sdu2_lenP);
  }
  /* mandatory */
  /* c6. Transaction ID (integer value) */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie6);
  ie6->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie6->criticality               = F1AP_Criticality_ignore;
  ie6->value.present             = F1AP_F1SetupRequestIEs__value_PR_TransactionID;
  ie6->value.choice.TransactionID = F1AP_get_next_transaction_identifier(f1ap_req(false, instanceP)->gNB_DU_id, f1ap_req(false, instanceP)->gNB_DU_id);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 INITIAL UL RRC MESSAGE TRANSFER\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(false, instanceP, buffer, len, getCxt(DUtype, instanceP)->default_sctp_stream_id);
  return 0;
}


int DU_send_UL_NR_RRC_MESSAGE_TRANSFER(instance_t instance,
                                       const f1ap_ul_rrc_message_t *msg) {
  const rnti_t rnti = msg->rnti;
  F1AP_F1AP_PDU_t                pdu= {0};
  F1AP_ULRRCMessageTransfer_t    *out;
  uint8_t *buffer = NULL;
  uint32_t len;
  LOG_I(F1AP, "[DU %ld] %s: size %d UE RNTI %x in SRB %d\n",
        instance, __func__, msg->rrc_container_length, rnti, msg->srb_id);
  //LOG_I(F1AP, "%s() RRCContainer size %d: ", __func__, msg->rrc_container_length);
  //for (int i = 0;i < msg->rrc_container_length; i++)
  //  printf("%02x ", msg->rrc_container[i]);
  //printf("\n");
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_ULRRCMessageTransfer;
  tmp->criticality   = F1AP_Criticality_ignore;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_ULRRCMessageTransfer;
  out = &tmp->value.choice.ULRRCMessageTransfer;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(DUtype, instance, rnti);
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(DUtype, instance, rnti);
  /* mandatory */
  /* c3. SRBID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie3);
  ie3->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie3->criticality                   = F1AP_Criticality_reject;
  ie3->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_SRBID;
  ie3->value.choice.SRBID            = msg->srb_id;
  // issue in here
  /* mandatory */
  /* c4. RRCContainer */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie4);
  ie4->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie4->criticality                   = F1AP_Criticality_reject;
  ie4->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie4->value.choice.RRCContainer,
                       (const char *) msg->rrc_container,
                       msg->rrc_container_length);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UL RRC MESSAGE TRANSFER \n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(false, instance, buffer, len, getCxt(DUtype, instance)->default_sctp_stream_id);
  return 0;
}

/*  DL NR RRC Message Transfer */
int DU_handle_DL_NR_RRC_MESSAGE_TRANSFER(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu) {
  LOG_D(F1AP, "DU_handle_DL_NR_RRC_MESSAGE_TRANSFER \n");
  F1AP_DLRRCMessageTransfer_t    *container;
  F1AP_DLRRCMessageTransferIEs_t *ie;
  uint64_t        cu_ue_f1ap_id;
  uint64_t        du_ue_f1ap_id;
  int             executeDuplication;
  //uint64_t        subscriberProfileIDforRFP;
  //uint64_t        rAT_FrequencySelectionPriority;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.DLRRCMessageTransfer;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  cu_ue_f1ap_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  LOG_D(F1AP, "cu_ue_f1ap_id %lu \n", cu_ue_f1ap_id);
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(F1AP, "du_ue_f1ap_id %lu associated with UE RNTI %x \n",
        du_ue_f1ap_id,
        f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id)); // this should be the one transmitted via initial ul rrc message transfer

  if (f1ap_du_add_cu_ue_id(instance,du_ue_f1ap_id, cu_ue_f1ap_id) < 0 ) {
    LOG_E(F1AP, "Failed to find the F1AP UID \n");
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
  uint64_t  srb_id = ie->value.choice.SRBID;
  LOG_D(F1AP, "srb_id %lu \n", srb_id);

  /* optional */
  /* ExecuteDuplication */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_ExecuteDuplication, true);
    executeDuplication = ie->value.choice.ExecuteDuplication;
    LOG_D(F1AP, "ExecuteDuplication %d \n", executeDuplication);
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
  //  memcpy(RRC_MAC_CCCH_DATA_IND (message_p).sdu, ie->value.choice.RRCContainer.buf,
  //         ccch_sdu_len);

  //LOG_I(F1AP, "%s() RRCContainer size %lu: ", __func__, ie->value.choice.RRCContainer.size);
  //for (int i = 0;i < ie->value.choice.RRCContainer.size; i++)
  //  printf("%02x ", ie->value.choice.RRCContainer.buf[i]);
  //printf("\n");

  /* optional */
  /* RAT_FrequencyPriorityInformation */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation, true);

    switch(ie->value.choice.RAT_FrequencyPriorityInformation.present) {
      case F1AP_RAT_FrequencyPriorityInformation_PR_eNDC:
        //subscriberProfileIDforRFP = ie->value.choice.RAT_FrequencyPriorityInformation.choice.subscriberProfileIDforRFP;
        break;

      case F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN:
        //rAT_FrequencySelectionPriority = ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority;
        break;

      default:
        LOG_W(F1AP, "unhandled IE RAT_FrequencyPriorityInformation.present\n");
        break;
    }
  }

  f1ap_dl_rrc_message_t dl_rrc = {
    .rrc_container_length = ie->value.choice.RRCContainer.size,
    .rrc_container = ie->value.choice.RRCContainer.buf,
    .rnti = f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id),
    .srb_id = srb_id
  };
  int rc = dl_rrc_message(instance, &dl_rrc);
  if (rc == 0)
    return 0; /* has been handled, otherwise continue below */

  // decode RRC Container and act on the message type
  AssertFatal(srb_id<3,"illegal srb_id\n");
  MessageDef *msg = itti_alloc_new_message(TASK_DU_F1, 0, NR_DU_RRC_DL_INDICATION);
  NRDuDlReq_t *req=&NRDuDlReq(msg);
  req->rnti=f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id);
  req->srb_id=srb_id;
  req->buf= get_free_mem_block( ie->value.choice.RRCContainer.size, __func__);
  memcpy(req->buf->data, ie->value.choice.RRCContainer.buf, ie->value.choice.RRCContainer.size);
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg);
  return 0;
}
