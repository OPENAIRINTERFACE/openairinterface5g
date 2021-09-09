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

/*! \file f1ap_cu_ue_context_management.c
 * \brief F1AP UE Context Management, CU side
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
#include "f1ap_cu_ue_context_management.h"
#include <string.h>

#include "rrc_extern.h"
#include "rrc_eNB_UE_context.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "rrc_eNB_S1AP.h"
#include "rrc_eNB_GTPV1U.h"
#include "openair2/RRC/NR/rrc_gNB_NGAP.h"

static void setQos(F1AP_NonDynamic5QIDescriptor_t *toFill) {
  asn1cCalloc(toFill,F1AP_NonDynamic5QIDescriptor_t, tmp);
  /* fiveQI */
  tmp->fiveQI = 1L;

  /* OPTIONAL */
  /* qoSPriorityLevel */
  if (0) {
    asn1cCallocOne(toFill->qoSPriorityLevel, long, 1L);
  }

  /* OPTIONAL */
  /* averagingWindow */
  if (0) {
    asn1cCallocOne(toFill->averagingWindow,
                   F1AP_AveragingWindow_t, 1L);
  }

  /* OPTIONAL */
  /* maxDataBurstVolume */
  if (0) {
    asn1cCallocOne(toFill->maxDataBurstVolume,
                   F1AP_MaxDataBurstVolume_t, 1L);
  }
}

int CU_send_UE_CONTEXT_SETUP_REQUEST(instance_t instance,
                                     f1ap_ue_context_setup_req_t *f1ap_ue_context_setup_req) {
  F1AP_F1AP_PDU_t  pdu= {0};
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, F1AP_InitiatingMessage_t, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextSetupRequest;
  F1AP_UEContextSetupRequest_t    *out = &pdu.choice.initiatingMessage->value.choice.UEContextSetupRequest;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(true, instance, f1ap_ue_context_setup_req->rnti); //f1ap_ue_context_setup_req->gNB_CU_ue_id;

  /* optional */
  /* c2. GNB_DU_UE_F1AP_ID */
  if (f1ap_ue_context_setup_req->gNB_DU_ue_id) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie2);
    ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
    ie2->criticality                    = F1AP_Criticality_ignore;
    ie2->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
    ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(true, instance, f1ap_ue_context_setup_req->rnti); //*f1ap_ue_context_setup_req->gNB_DU_ue_id;
  }

  /* mandatory */
  /* c3. SpCell_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
  ie3->criticality                    = F1AP_Criticality_reject;
  ie3->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_NRCGI;
  /* - nRCGI */
  addnRCGI(ie3->value.choice.NRCGI, f1ap_ue_context_setup_req);
  /* mandatory */
  /* c4. ServCellIndex */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie4);
  ie4->id                             = F1AP_ProtocolIE_ID_id_ServCellIndex;
  ie4->criticality                    = F1AP_Criticality_reject;
  ie4->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ServCellIndex;
  ie4->value.choice.ServCellIndex = 0;
  
  /* optional */
  /* c5. CellULConfigured */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_SpCellULConfigured;
    ie5->criticality                    = F1AP_Criticality_ignore;
    ie5->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CellULConfigured;
    ie5->value.choice.CellULConfigured = F1AP_CellULConfigured_ul;
  }

  /* mandatory */
  /* c6. CUtoDURRCInformation */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie6);
  ie6->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
  ie6->criticality                    = F1AP_Criticality_reject;
  ie6->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CUtoDURRCInformation;
  /* optional */
  /* 6.1 cG_ConfigInfo */
  if (0) {
    const char cG_ConfigInfoStr[]="asdsa1d32sa1d31asd31as";
    asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.cG_ConfigInfo, F1AP_CG_ConfigInfo_t, cG_ConfigInfo);
    OCTET_STRING_fromBuf(cG_ConfigInfo, cG_ConfigInfoStr, strlen( cG_ConfigInfoStr )) ;
    /* optional */
    /* 6.2 uE_CapabilityRAT_ContainerList */
    asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList, F1AP_UE_CapabilityRAT_ContainerList_t, uE_CapabilityRAT_ContainerList );
    OCTET_STRING_fromBuf(uE_CapabilityRAT_ContainerList, cG_ConfigInfoStr, strlen( cG_ConfigInfoStr )) ;
    /* optional */
    /* 6.3 measConfig */
    asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.measConfig, F1AP_MeasConfig_t, measConfig);
    OCTET_STRING_fromBuf(measConfig, cG_ConfigInfoStr, strlen( cG_ConfigInfoStr )) ;
  }

  /* mandatory */
  /* c7. Candidate_SpCell_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie7);
  ie7->id                             = F1AP_ProtocolIE_ID_id_Candidate_SpCell_List;  //90
  ie7->criticality                    = F1AP_Criticality_ignore;
  ie7->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_Candidate_SpCell_List;
  
  for (int i=0;   i<1;  i++) {
    asn1cSequenceAdd(ie7->value.choice.Candidate_SpCell_List.list,F1AP_Candidate_SpCell_ItemIEs_t, candidate_spCell_item_ies);
    candidate_spCell_item_ies->id            = F1AP_ProtocolIE_ID_id_Candidate_SpCell_Item; // 91
    candidate_spCell_item_ies->criticality   = F1AP_Criticality_reject;
    candidate_spCell_item_ies->value.present = F1AP_Candidate_SpCell_ItemIEs__value_PR_Candidate_SpCell_Item;
    /* 7.1 Candidate_SpCell_Item */
    F1AP_Candidate_SpCell_Item_t *candidate_spCell_item=
      &candidate_spCell_item_ies->value.choice.Candidate_SpCell_Item;
    /* - candidate_SpCell_ID */
    //FixMe: first cell ???
    addnRCGI(candidate_spCell_item->candidate_SpCell_ID,f1ap_ue_context_setup_req);
    /* TODO add correct mcc/mnc */
  }

  /* optional */
  /* c8. DRXCycle */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie8);
    ie8->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie8->criticality                    = F1AP_Criticality_ignore;
    ie8->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_DRXCycle;
    /* 8.1 longDRXCycleLength */
    ie8->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum

    /* optional */
    /* 8.2 shortDRXCycleLength */
    if (0) {
      asn1cCallocOne(ie6->value.choice.DRXCycle.shortDRXCycleLength,
                     F1AP_ShortDRXCycleLength_t, F1AP_ShortDRXCycleLength_ms2); // enum
    }

    /* optional */
    /* 8.3 shortDRXCycleTimer */
    if (0) {
      asn1cCallocOne(ie8->value.choice.DRXCycle.shortDRXCycleTimer,
                     F1AP_ShortDRXCycleTimer_t, 123L);
    }
  }

  /* optional */
  /* c9. ResourceCoordinationTransferContainer */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie9);
    ie9->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie9->criticality                    = F1AP_Criticality_ignore;
    ie9->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    ie9->value.choice.ResourceCoordinationTransferContainer.buf = malloc(4);
    ie9->value.choice.ResourceCoordinationTransferContainer.size = 4;
    strncpy((char *)ie9->value.choice.ResourceCoordinationTransferContainer.buf, "123", 4);
    OCTET_STRING_fromBuf(&ie9->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
  }
   /* mandatory */
  /* c10. SCell_ToBeSetup_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie10);
  ie10->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_List;
  ie10->criticality                    = F1AP_Criticality_ignore;
  ie10->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_SCell_ToBeSetup_List;
  
  for (int i=0; i<1; i++) {
    //
    asn1cSequenceAdd(ie10->value.choice.SCell_ToBeSetup_List.list, F1AP_SCell_ToBeSetup_ItemIEs_t, scell_toBeSetup_item_ies);
    scell_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_Item; //53
    scell_toBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
    scell_toBeSetup_item_ies->value.present = F1AP_SCell_ToBeSetup_ItemIEs__value_PR_SCell_ToBeSetup_Item;
    /* 10.1 SCell_ToBeSetup_Item */
    F1AP_SCell_ToBeSetup_Item_t *scell_toBeSetup_item=&scell_toBeSetup_item_ies->value.choice.SCell_ToBeSetup_Item;
    /* 10.1.1 sCell_ID */
    addnRCGI(scell_toBeSetup_item->sCell_ID, f1ap_ue_context_setup_req);
    /* TODO correct MCC/MNC */
    /* 10.1.2 sCellIndex */
    scell_toBeSetup_item->sCellIndex = 3;  // issue here

    /* OPTIONAL */
    /* 10.1.3 sCellULConfigured*/
    if (0) {
      asn1cCallocOne(scell_toBeSetup_item->sCellULConfigured,
                     F1AP_CellULConfigured_t, F1AP_CellULConfigured_ul_and_sul); // enum
    }
  }

  /* mandatory */
  /* c11. SRBs_ToBeSetup_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie11);
  ie11->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_List;
  ie11->criticality                    = F1AP_Criticality_reject;  // ?
  ie11->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_SRBs_ToBeSetup_List;

  for (int i=0; i<1; i++) {
    //
    asn1cSequenceAdd(ie11->value.choice.SRBs_ToBeSetup_List.list, F1AP_SRBs_ToBeSetup_ItemIEs_t, srbs_toBeSetup_item_ies);
    srbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_Item; // 73
    srbs_toBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeSetup_item_ies->value.present = F1AP_SRBs_ToBeSetup_ItemIEs__value_PR_SRBs_ToBeSetup_Item;
    /* 11.1 SRBs_ToBeSetup_Item */
    F1AP_SRBs_ToBeSetup_Item_t *srbs_toBeSetup_item=&srbs_toBeSetup_item_ies->value.choice.SRBs_ToBeSetup_Item;
    /* 11.1.1 sRBID */
    srbs_toBeSetup_item->sRBID = 2L;
    /* OPTIONAL */
    /* 11.1.2 duplicationIndication */
    //if (0) {
    asn1cCallocOne(srbs_toBeSetup_item->duplicationIndication,
                   F1AP_DuplicationIndication_t, F1AP_DuplicationIndication_true); // enum
    //}
  }

  /* mandatory */
  /* c12. DRBs_ToBeSetup_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie12);
  ie12->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_List;
  ie12->criticality                    = F1AP_Criticality_reject;
  ie12->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_DRBs_ToBeSetup_List;
  LOG_I(F1AP, "Length of drbs_to_be_setup: %d \n", f1ap_ue_context_setup_req->drbs_to_be_setup_length);

  for (int i = 0; i < f1ap_ue_context_setup_req->drbs_to_be_setup_length; i++) {
    //
    asn1cSequenceAdd(ie12->value.choice.DRBs_ToBeSetup_List.list, F1AP_DRBs_ToBeSetup_ItemIEs_t, drbs_toBeSetup_item_ies);
    drbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_Item;
    drbs_toBeSetup_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeSetup_item_ies->value.present = F1AP_DRBs_ToBeSetup_ItemIEs__value_PR_DRBs_ToBeSetup_Item;
    /* 12.1 DRBs_ToBeSetup_Item */
    F1AP_DRBs_ToBeSetup_Item_t *drbs_toBeSetup_item=&drbs_toBeSetup_item_ies->value.choice.DRBs_ToBeSetup_Item;
    /* 12.1.1 dRBID */
    drbs_toBeSetup_item->dRBID = f1ap_ue_context_setup_req->drbs_to_be_setup[i].drb_id; // 9
    /* 12.1.2 qoSInformation */
    int some_decide_qos = 0; // BK: Need Check

    if (some_decide_qos) {
      drbs_toBeSetup_item->qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
      /*  12.1.2.1 eUTRANQoS */
      asn1cCalloc(drbs_toBeSetup_item->qoSInformation.choice.eUTRANQoS, F1AP_EUTRANQoS_t, eUTRANQoS);
      /*  12.1.2.1.1 qCI */
      eUTRANQoS->qCI = 254L;
      /*  12.1.2.1.2 allocationAndRetentionPriority */
      {
        /*  12.1.2.1.2.1 priorityLevel */
        eUTRANQoS->allocationAndRetentionPriority.priorityLevel = F1AP_PriorityLevel_highest; // enum
        /*  12.1.2.1.2.2 pre_emptionCapability */
        eUTRANQoS->allocationAndRetentionPriority.pre_emptionCapability = F1AP_Pre_emptionCapability_may_trigger_pre_emption; // enum
        /*  12.1.2.1.2.2 pre_emptionVulnerability */
        eUTRANQoS->allocationAndRetentionPriority.pre_emptionVulnerability = F1AP_Pre_emptionVulnerability_not_pre_emptable; // enum
      }

      /* OPTIONAL */
      /*  12.1.2.1.3 gbrQosInformation */
      if (0) {
        eUTRANQoS->gbrQosInformation = (F1AP_GBR_QosInformation_t *)calloc(1, sizeof(F1AP_GBR_QosInformation_t));
        asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_MaximumBitrateDL, 1L);
        asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_MaximumBitrateUL, 1L);
        asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_GuaranteedBitrateDL, 1L);
        asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_GuaranteedBitrateUL, 1L);
      }
    } else {
      /* 12.1.2 DRB_Information */
      drbs_toBeSetup_item->qoSInformation.present = F1AP_QoSInformation_PR_choice_extension;
      F1AP_QoSInformation_ExtIEs_t *ie = (F1AP_QoSInformation_ExtIEs_t *)calloc(1, sizeof(*ie));
      ie->id                             = F1AP_ProtocolIE_ID_id_DRB_Information;
      ie->criticality                    = F1AP_Criticality_reject;
      ie->value.present                  = F1AP_QoSInformation_ExtIEs__value_PR_DRB_Information;
      F1AP_DRB_Information_t   *DRB_Information = &ie->value.choice.DRB_Information;
      drbs_toBeSetup_item->qoSInformation.choice.choice_extension = (struct F1AP_ProtocolIE_SingleContainer *)ie;
      /* 12.1.2.1 dRB_QoS */
      {
        /* qoS_Characteristics */
        {
          int some_decide_qoS_characteristics = 0; // BK: Need Check

          if (some_decide_qoS_characteristics) {
            DRB_Information->dRB_QoS.qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_non_Dynamic_5QI;
            setQos(DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI);
          } else {
            DRB_Information->dRB_QoS.qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_dynamic_5QI;
            asn1cCalloc(DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI, F1AP_Dynamic5QIDescriptor_t, tmp);
            /* qoSPriorityLevel */
            tmp->qoSPriorityLevel = 1L;
            /* packetDelayBudget */
            tmp->packetDelayBudget = 1L;
            /* packetErrorRate */
            tmp->packetErrorRate.pER_Scalar = 1L;
            tmp->packetErrorRate.pER_Exponent = 6L;

            /* OPTIONAL */
            /* delayCritical */
            if (0) {
              asn1cCallocOne(DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->delayCritical,
                             long, 1L);
            }

            /* OPTIONAL */
            /* averagingWindow */
            if (0) {
              asn1cCallocOne(DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->averagingWindow,
                             F1AP_AveragingWindow_t, 1L);
            }

            /* OPTIONAL */
            /* maxDataBurstVolume */
            if (0) {
              asn1cCallocOne(DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->maxDataBurstVolume,
                             F1AP_MaxDataBurstVolume_t, 1L);
            }
          } // if some_decide_qoS_characteristics
        } // qoS_Characteristics
        /* nGRANallocationRetentionPriority */
        {
          DRB_Information->dRB_QoS.nGRANallocationRetentionPriority.priorityLevel = F1AP_PriorityLevel_highest; // enum
          DRB_Information->dRB_QoS.nGRANallocationRetentionPriority.pre_emptionCapability = F1AP_Pre_emptionCapability_shall_not_trigger_pre_emption; // enum
          DRB_Information->dRB_QoS.nGRANallocationRetentionPriority.pre_emptionVulnerability = F1AP_Pre_emptionVulnerability_not_pre_emptable; // enum
        } // nGRANallocationRetentionPriority

        /* OPTIONAL */
        /* gBR_QoS_Flow_Information */
        if (0) {
          asn1cCalloc(DRB_Information->dRB_QoS.gBR_QoS_Flow_Information, F1AP_GBR_QoSFlowInformation_t, tmp);
          asn_long2INTEGER(&tmp->maxFlowBitRateDownlink, 1L);
          asn_long2INTEGER(&tmp->maxFlowBitRateUplink, 1L);
          asn_long2INTEGER(&tmp->guaranteedFlowBitRateDownlink, 1L);
          asn_long2INTEGER(&tmp->guaranteedFlowBitRateUplink, 1L);

          /* OPTIONAL */
          /* maxPacketLossRateDownlink */
          if (0) {
            asn1cCallocOne(DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxPacketLossRateDownlink,
                           F1AP_MaxPacketLossRate_t, 1L);
          }

          /* OPTIONAL */
          /* maxPacketLossRateUplink */
          if (0) {
            asn1cCallocOne(DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxPacketLossRateUplink,
                           F1AP_MaxPacketLossRate_t, 1L);
          }
        }

        /* OPTIONAL */
        /* reflective_QoS_Attribute */
        if (0) {
          asn1cCallocOne(DRB_Information->dRB_QoS.reflective_QoS_Attribute,
                         long, 1L);
        }
      } // dRB_QoS
      /* 12.1.2.2 sNSSAI */
      {
        /* sST */
	
        OCTET_STRING_fromBuf(&DRB_Information->sNSSAI.sST, "1", 1);

        /* OPTIONAL */
        /* sD */
        if (0) {
          asn1cCalloc(DRB_Information->sNSSAI.sD, OCTET_STRING_t, tmp);
          OCTET_STRING_fromBuf(tmp, "asdsa1d32sa1d31asd31as",
                               strlen("asdsa1d32sa1d31asd31as"));
        }
      }

      /* OPTIONAL */
      /* 12.1.2.3 notificationControl */
      if (0) {
        asn1cCallocOne(DRB_Information->notificationControl,
                       F1AP_NotificationControl_t, F1AP_NotificationControl_active); // enum
      }

      /* 12.1.2.4 flows_Mapped_To_DRB_List */  // BK: need verifiy

      for (int k = 0; k < 1; k ++) {
        asn1cSequenceAdd(DRB_Information->flows_Mapped_To_DRB_List.list,
                         F1AP_Flows_Mapped_To_DRB_Item_t, flows_mapped_to_drb_item);
        /* qoSFlowIndicator */
        flows_mapped_to_drb_item->qoSFlowIdentifier = 1L;
        /* qoSFlowLevelQoSParameters */
        {
          /* qoS_Characteristics */
          {
            int some_decide_qoS_characteristics = 0; // BK: Need Check
            F1AP_QoS_Characteristics_t *QosParams=&flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.qoS_Characteristics;

            if (some_decide_qoS_characteristics) {
              QosParams->present = F1AP_QoS_Characteristics_PR_non_Dynamic_5QI;
              setQos(QosParams->choice.non_Dynamic_5QI);
            } else {
              QosParams->present = F1AP_QoS_Characteristics_PR_dynamic_5QI;
              asn1cCalloc(QosParams->choice.dynamic_5QI, F1AP_Dynamic5QIDescriptor_t, tmp);
              /* qoSPriorityLevel */
              tmp->qoSPriorityLevel = 1L;
              /* packetDelayBudget */
              tmp->packetDelayBudget = 1L;
              /* packetErrorRate */
              tmp->packetErrorRate.pER_Scalar = 1L;
              tmp->packetErrorRate.pER_Exponent = 6L;

              /* OPTIONAL */
              /* delayCritical */
              if (0) {
                asn1cCalloc(QosParams->choice.dynamic_5QI->delayCritical, long, tmp);
                *tmp = 1L;
              }

              /* OPTIONAL */
              /* averagingWindow */
              if (0) {
                asn1cCalloc(QosParams->choice.dynamic_5QI->averagingWindow, F1AP_AveragingWindow_t, tmp);
                *tmp = 1L;
              }

              /* OPTIONAL */
              /* maxDataBurstVolume */
              if (0) {
                asn1cCalloc(QosParams->choice.dynamic_5QI->maxDataBurstVolume, F1AP_MaxDataBurstVolume_t, tmp);
                *tmp= 1L;
              }
            } // if some_decide_qoS_characteristics
          } // qoS_Characteristics
          /* nGRANallocationRetentionPriority */
          {
            flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.priorityLevel = F1AP_PriorityLevel_highest; // enum
            flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.pre_emptionCapability = F1AP_Pre_emptionCapability_shall_not_trigger_pre_emption; // enum
            flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.pre_emptionVulnerability = F1AP_Pre_emptionVulnerability_not_pre_emptable; // enum
          } // nGRANallocationRetentionPriority

          /* OPTIONAL */
          /* gBR_QoS_Flow_Information */
          if (0) {
            asn1cCalloc(flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information, F1AP_GBR_QoSFlowInformation_t, tmp);
            asn_long2INTEGER(&tmp->maxFlowBitRateDownlink, 1L);
            asn_long2INTEGER(&tmp->maxFlowBitRateUplink, 1L);
            asn_long2INTEGER(&tmp->guaranteedFlowBitRateDownlink, 1L);
            asn_long2INTEGER(&tmp->guaranteedFlowBitRateUplink, 1L);

            /* OPTIONAL */
            /* maxPacketLossRateDownlink */
            if (0) {
              asn1cCalloc(flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateDownlink, F1AP_MaxPacketLossRate_t, tmp);
              *tmp = 1L;
            }

            /* OPTIONAL */
            /* maxPacketLossRateUplink */
            if (0) {
              asn1cCalloc(flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateUplink, F1AP_MaxPacketLossRate_t, tmp);
              *tmp = 1L;
            }
          }

          /* OPTIONAL */
          /* reflective_QoS_Attribute */
          if (0) {
            asn1cCalloc(flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.reflective_QoS_Attribute, long, tmp);
            *tmp = 1L;
          }
        } // qoSFlowLevelQoSParameters
      }
    } // if some_decide_qos

    /* 12.1.3 uLUPTNLInformation_ToBeSetup_List */
    for (int j = 0; j < f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl_length; j++) {
      /*  12.3.1 ULTunnels_ToBeSetup_Item */
      asn1cSequenceAdd(drbs_toBeSetup_item->uLUPTNLInformation_ToBeSetup_List.list,
                       F1AP_ULUPTNLInformation_ToBeSetup_Item_t, uLUPTNLInformation_ToBeSetup_Item);
      uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
      asn1cCalloc( uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel,
                   F1AP_GTPTunnel_t,  gTPTunnel);
      /* 12.3.1.1.1 transportLayerAddress */
      TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl[j].tl_address,
						  &gTPTunnel->transportLayerAddress);
      /* 12.3.1.1.2 gTP_TEID */
      INT32_TO_OCTET_STRING(f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl[j].teid,
			    &gTPTunnel->gTP_TEID);
    }

    /* 12.1.4 rLCMode */
    /* TODO use rlc_mode from f1ap_drb_to_be_setup */
    switch (f1ap_ue_context_setup_req->drbs_to_be_setup[i].rlc_mode) {
      case RLC_MODE_AM:
        drbs_toBeSetup_item->rLCMode = F1AP_RLCMode_rlc_am;
        break;

      default:
        drbs_toBeSetup_item->rLCMode = F1AP_RLCMode_rlc_um_bidirectional;
    }

    /* OPTIONAL */
    /* 12.1.5 ULConfiguration */
    if (0) {
      asn1cCalloc(drbs_toBeSetup_item->uLConfiguration, F1AP_ULConfiguration_t, tmp);
      tmp->uLUEConfiguration = F1AP_ULUEConfiguration_no_data;
    }

    /* OPTIONAL */
    /* 12.1.6 duplicationActivation */
    if (0) {
      asn1cCalloc(drbs_toBeSetup_item->duplicationActivation, F1AP_DuplicationActivation_t, tmp);
      *tmp = F1AP_DuplicationActivation_active;  // enum
    }
  }

  /* OPTIONAL */
  /* InactivityMonitoringRequest */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie12);
    ie12->id                             = F1AP_ProtocolIE_ID_id_InactivityMonitoringRequest;
    ie12->criticality                    = F1AP_Criticality_reject;
    ie12->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_InactivityMonitoringRequest;
    ie12->value.choice.InactivityMonitoringRequest = F1AP_InactivityMonitoringRequest_true; // 0
  }

  /* OPTIONAL */
  /* RAT_FrequencyPriorityInformation */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie13);
    ie13->id                             = F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation;
    ie13->criticality                    = F1AP_Criticality_reject;
    ie13->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_RAT_FrequencyPriorityInformation;
    int endc = 1; // RK: Get this from somewhere ...

    if (endc) {
      ie13->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_eNDC;
      ie13->value.choice.RAT_FrequencyPriorityInformation.choice.eNDC = 11L;
    } else {
      ie13->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN;
      ie13->value.choice.RAT_FrequencyPriorityInformation.choice.nGRAN = 11L;
    }
  }

  /* OPTIONAL */
  /* RRCContainer */
  if(f1ap_ue_context_setup_req->rrc_container_length > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie14);
    ie14->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie14->criticality                    = F1AP_Criticality_reject;
    ie14->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie14->value.choice.RRCContainer, (const char *)f1ap_ue_context_setup_req->rrc_container,
                         f1ap_ue_context_setup_req->rrc_container_length);
  }

  /* OPTIONAL */
  /* MaskedIMEISV */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie15);
    ie15->id                             = F1AP_ProtocolIE_ID_id_MaskedIMEISV;
    ie15->criticality                    = F1AP_Criticality_reject;
    ie15->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_MaskedIMEISV;
    MaskedIMEISV_TO_BIT_STRING(12340000l, &ie15->value.choice.MaskedIMEISV); // size (64)
  }

  /* encode */
  uint8_t  *buffer=NULL;
  uint32_t  len=0;

  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE CONTEXT SETUP REQUEST\n");
    return -1;
  }

  // xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, (void *)pdu);
  // asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  // res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_F1AP_F1AP_PDU, pdu);
  // buffer = res.buffer;
  // len = res.result.encoded;
  // if (res.result.encoded <= 0) {
  //   LOG_E(F1AP, "ASN1 message encoding failed (%s, %lu)!\n", res.result.failed_type->name, res.result.encoded);
  //   return -1;
  // }
  LOG_D(F1AP,"F1AP UEContextSetupRequest Encoded %u bits\n", len);
  f1ap_itti_send_sctp_data_req(true, instance, buffer, len, 0 /* BK: fix me*/);
  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_RESPONSE(instance_t       instance,
                                        uint32_t         assoc_id,
                                        uint32_t         stream,
                                        F1AP_F1AP_PDU_t *pdu) {
  MessageDef                       *msg_p;
  F1AP_UEContextSetupResponse_t    *container;
  F1AP_UEContextSetupResponseIEs_t *ie;
  DevAssert(pdu);
  msg_p = itti_alloc_new_message(TASK_DU_F1, 0,  F1AP_UE_CONTEXT_SETUP_RESP);
  f1ap_ue_context_setup_req_t *f1ap_ue_context_setup_resp = &F1AP_UE_CONTEXT_SETUP_RESP(msg_p);
  container = &pdu->choice.successfulOutcome->value.choice.UEContextSetupResponse;
  int i;

  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  f1ap_ue_context_setup_resp->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  f1ap_ue_context_setup_resp->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* DUtoCURRCInformation */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DUtoCURRCInformation, true);
  if (ie == NULL){
    LOG_E(F1AP,"%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }
  memcpy(f1ap_ue_context_setup_resp->du_to_cu_rrc_information, ie->value.choice.DUtoCURRCInformation.cellGroupConfig.buf, ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size);
  f1ap_ue_context_setup_resp->du_to_cu_rrc_information_length = ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size;

  /* DRBs_Setup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DRBs_Setup_List, true);
  if(ie!=NULL){
    f1ap_ue_context_setup_resp->drbs_setup_length = ie->value.choice.DRBs_Setup_List.list.count;
    f1ap_ue_context_setup_resp->drbs_setup = calloc(f1ap_ue_context_setup_resp->drbs_setup_length,
        sizeof(f1ap_drb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->drbs_setup,
              "could not allocate memory for f1ap_ue_context_setup_resp->drbs_setup\n");
    for (i = 0; i < f1ap_ue_context_setup_resp->drbs_setup_length; ++i) {
      f1ap_drb_to_be_setup_t *drb_p = &f1ap_ue_context_setup_resp->drbs_setup[i];
      F1AP_DRBs_Setup_Item_t *drbs_setup_item_p;
      drbs_setup_item_p = &((F1AP_DRBs_Setup_ItemIEs_t *)ie->value.choice.DRBs_Setup_List.list.array[i])->value.choice.DRBs_Setup_Item;
      drb_p->drb_id = drbs_setup_item_p->dRBID;
      /* TODO in the following, assume only one UP UL TNL is present.
       * this matches/assumes OAI CU/DU implementation, can be up to 2! */
      drb_p->up_dl_tnl_length = 1;
      AssertFatal(drbs_setup_item_p->dLUPTNLInformation_ToBeSetup_List.list.count > 0,
          "no DL UP TNL Information in DRBs to be Setup list\n");
      F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dl_up_tnl_info_p = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)drbs_setup_item_p->dLUPTNLInformation_ToBeSetup_List.list.array[0];
      F1AP_GTPTunnel_t *dl_up_tnl0 = dl_up_tnl_info_p->dLUPTNLInformation.choice.gTPTunnel;
      BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&dl_up_tnl0->transportLayerAddress, drb_p->up_dl_tnl[0].tl_address);
      OCTET_STRING_TO_INT32(&dl_up_tnl0->gTP_TEID, drb_p->up_dl_tnl[0].teid);

    }
  }
  /* SRBs_FailedToBeSetup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_List, true);
  if(ie!=NULL){
    f1ap_ue_context_setup_resp->srbs_failed_to_be_setup_length = ie->value.choice.SRBs_FailedToBeSetup_List.list.count;
    f1ap_ue_context_setup_resp->srbs_failed_to_be_setup = calloc(f1ap_ue_context_setup_resp->srbs_failed_to_be_setup_length,
        sizeof(f1ap_rb_failed_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->srbs_failed_to_be_setup,
        "could not allocate memory for f1ap_ue_context_setup_resp->srbs_failed_to_be_setup\n");
    for (i = 0; i < f1ap_ue_context_setup_resp->srbs_failed_to_be_setup_length; ++i) {
      f1ap_rb_failed_to_be_setup_t *srb_p = &f1ap_ue_context_setup_resp->srbs_failed_to_be_setup[i];
      srb_p->rb_id = ((F1AP_SRBs_FailedToBeSetup_Item_t *)ie->value.choice.SRBs_FailedToBeSetup_List.list.array[i])->sRBID;
    }

  }
  /* DRBs_FailedToBeSetup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_List, true);
  if(ie!=NULL){
    f1ap_ue_context_setup_resp->drbs_failed_to_be_setup_length = ie->value.choice.DRBs_FailedToBeSetup_List.list.count;
    f1ap_ue_context_setup_resp->drbs_failed_to_be_setup = calloc(f1ap_ue_context_setup_resp->drbs_failed_to_be_setup_length,
        sizeof(f1ap_rb_failed_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->drbs_failed_to_be_setup,
        "could not allocate memory for f1ap_ue_context_setup_resp->drbs_failed_to_be_setup\n");
    for (i = 0; i < f1ap_ue_context_setup_resp->drbs_failed_to_be_setup_length; ++i) {
      f1ap_rb_failed_to_be_setup_t *drb_p = &f1ap_ue_context_setup_resp->drbs_failed_to_be_setup[i];
      drb_p->rb_id = ((F1AP_DRBs_FailedToBeSetup_Item_t *)ie->value.choice.DRBs_FailedToBeSetup_List.list.array[i])->dRBID;
    }
  }

  /* SCell_FailedtoSetup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_List, true);
  if(ie!=NULL){
    LOG_E (F1AP, "Not supporting handling of SCell_FailedtoSetup_List \n");
  }

  /* SRBs_Setup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
      F1AP_ProtocolIE_ID_id_SRBs_Setup_List, true);
  if(ie!=NULL){
    f1ap_ue_context_setup_resp->srbs_setup_length = ie->value.choice.SRBs_Setup_List.list.count;
    f1ap_ue_context_setup_resp->srbs_setup = calloc(f1ap_ue_context_setup_resp->srbs_setup_length,
        sizeof(f1ap_srb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->drbs_setup,
        "could not allocate memory for f1ap_ue_context_setup_resp->drbs_setup\n");
    for (i = 0; i < f1ap_ue_context_setup_resp->srbs_setup_length; ++i) {
      f1ap_srb_to_be_setup_t *srb_p = &f1ap_ue_context_setup_resp->srbs_setup[i];
      F1AP_SRBs_Setup_Item_t *srbs_setup_item_p;
      srbs_setup_item_p = &((F1AP_SRBs_Setup_ItemIEs_t *)ie->value.choice.SRBs_Setup_List.list.array[i])->value.choice.SRBs_Setup_Item;
      srb_p->srb_id = srbs_setup_item_p->sRBID;
      srb_p->lcid = srbs_setup_item_p->lCID;
    }
  }

  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_FAILURE(instance_t       instance,
                                       uint32_t         assoc_id,
                                       uint32_t         stream,
                                       F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_RELEASE_REQUEST(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu) {
  F1AP_UEContextReleaseRequest_t    *container;
  F1AP_UEContextReleaseRequestIEs_t *ie;
  DevAssert(pdu);
  container = &pdu->choice.initiatingMessage->value.choice.UEContextReleaseRequest;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  const rnti_t rnti = f1ap_get_rnti_by_cu_id(true, instance,
                      ie->value.choice.GNB_CU_UE_F1AP_ID);
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  const rnti_t rnti2 = f1ap_get_rnti_by_du_id(true, instance,
                       ie->value.choice.GNB_DU_UE_F1AP_ID);
  AssertFatal(rnti == rnti2, "RNTI obtained through DU ID (%x) is different from CU ID (%x)\n",
              rnti2, rnti);
  /* Cause */
  /* We don't care for the moment
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_Cause, true);

  switch(ie->value.choice.Cause.present)
  {
    case F1AP_Cause_PR_radioNetwork:
      //ie->value.choice.Cause.choice.radioNetwork
      break;
    case F1AP_Cause_PR_transport:
      //ie->value.choice.Cause.choice.transport
      break;
    case F1AP_Cause_PR_protocol:
      //ie->value.choice.Cause.choice.protocol
      break;
    case F1AP_Cause_PR_misc:
      //ie->value.choice.Cause.choice.misc
      break;
    case F1AP_Cause_PR_NOTHING:
    default:
      break;
  }
  */
  LOG_I(F1AP, "Received UE CONTEXT RELEASE REQUEST: Trigger RRC for RNTI %x\n", rnti);

  if (f1ap_req(true, instance)->cell_type==CELL_MACRO_GNB) {
    AssertFatal(false,"must be devlopped\n");
  } else {
    struct rrc_eNB_ue_context_s *ue_context_pP;
    ue_context_pP = rrc_eNB_get_ue_context(RC.rrc[instance], rnti);
    rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_REQ(
      instance,
      ue_context_pP,
      S1AP_CAUSE_RADIO_NETWORK,
      21); // send cause 21: connection with ue lost
  }

  return 0;
}


int CU_send_UE_CONTEXT_RELEASE_COMMAND(instance_t instance,
                                       f1ap_ue_context_release_cmd_t *cmd) {
  F1AP_F1AP_PDU_t                   pdu= {0};
  F1AP_UEContextReleaseCommand_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, F1AP_InitiatingMessage_t, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextRelease;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextReleaseCommand;
  out = &tmp->value.choice.UEContextReleaseCommand;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(true, instance, cmd->rnti);
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(true, instance, cmd->rnti);
  /* mandatory */
  /* c3. Cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_Cause;
  ie3->criticality                    = F1AP_Criticality_ignore;
  ie3->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_Cause;

  switch (cmd->cause) {
    case F1AP_CAUSE_RADIO_NETWORK:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_radioNetwork;
      ie3->value.choice.Cause.choice.radioNetwork = cmd->cause_value;
      break;

    case F1AP_CAUSE_TRANSPORT:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_transport;
      ie3->value.choice.Cause.choice.transport = cmd->cause_value;
      break;

    case F1AP_CAUSE_PROTOCOL:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_protocol;
      ie3->value.choice.Cause.choice.protocol = cmd->cause_value;
      break;

    case F1AP_CAUSE_MISC:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_misc;
      ie3->value.choice.Cause.choice.misc = cmd->cause_value;
      break;

    case F1AP_CAUSE_NOTHING:
    default:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_NOTHING;
      break;
  }

  /* optional */
  /* c4. RRCContainer */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie4);
  ie4->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie4->criticality                    = F1AP_Criticality_ignore;
  ie4->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie4->value.choice.RRCContainer, (const char *)cmd->rrc_container,
                       cmd->rrc_container_length);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release command\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(true, instance, buffer, len, 0);
  return 0;
}
int CU_handle_UE_CONTEXT_RELEASE_COMPLETE(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu) {
  F1AP_UEContextReleaseComplete_t    *container;
  F1AP_UEContextReleaseCompleteIEs_t *ie;
  DevAssert(pdu);
  container = &pdu->choice.successfulOutcome->value.choice.UEContextReleaseComplete;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCompleteIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  const rnti_t rnti = f1ap_get_rnti_by_cu_id(true, instance,
                      ie->value.choice.GNB_CU_UE_F1AP_ID);
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCompleteIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  const rnti_t rnti2 = f1ap_get_rnti_by_du_id(true, instance,
                       ie->value.choice.GNB_DU_UE_F1AP_ID);
  AssertFatal(rnti == rnti2, "RNTI obtained through DU ID (%x) is different from CU ID (%x)\n",
              rnti2, rnti);
  /* Optional*/
  /* CriticalityDiagnostics */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCompleteIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_CriticalityDiagnostics, false);

  if (ie) {
    // ie->value.choice.CriticalityDiagnostics.procedureCode
    // ie->value.choice.CriticalityDiagnostics.triggeringMessage
    // ie->value.choice.CriticalityDiagnostics.procedureCriticality
    // ie->value.choice.CriticalityDiagnostics.transactionID
    // F1AP_CriticalityDiagnostics_IE_List
  }

  protocol_ctxt_t ctxt;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, instance, ENB_FLAG_YES, rnti, 0, 0, instance);

  if (f1ap_req(true, instance)->cell_type==CELL_MACRO_GNB) {
    struct rrc_gNB_ue_context_s *ue_context_p =
      rrc_gNB_get_ue_context(RC.nrrrc[instance], rnti);

    if (ue_context_p) {
      MessageDef *msg = itti_alloc_new_message(TASK_CU_F1, 0, NGAP_UE_CONTEXT_RELEASE_COMPLETE);
      NGAP_UE_CONTEXT_RELEASE_COMPLETE(msg).gNB_ue_ngap_id = ue_context_p->ue_context.gNB_ue_ngap_id;
      itti_send_msg_to_task(TASK_NGAP, instance, msg);
      rrc_gNB_remove_ue_context(&ctxt, RC.nrrrc[instance], ue_context_p);
    } else {
      LOG_E(F1AP, "could not find ue_context of UE RNTI %x\n", rnti);
    }

#ifdef ITTI_SIM
    return 0;
#endif
  } else {
    struct rrc_eNB_ue_context_s *ue_context_p =
      rrc_eNB_get_ue_context(RC.rrc[instance], rnti);

    if (ue_context_p) {
      /* The following is normally done in the function rrc_rx_tx() */
      rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_CPLT(instance,
          ue_context_p->ue_context.eNB_ue_s1ap_id);
      rrc_eNB_send_GTPV1U_ENB_DELETE_TUNNEL_REQ(instance, ue_context_p);

      // erase data of GTP tunnels in UE context
      for (int e_rab = 0; e_rab < ue_context_p->ue_context.nb_of_e_rabs; e_rab++) {
        ue_context_p->ue_context.enb_gtp_teid[e_rab] = 0;
        memset(&ue_context_p->ue_context.enb_gtp_addrs[e_rab],
               0, sizeof(ue_context_p->ue_context.enb_gtp_addrs[e_rab]));
        ue_context_p->ue_context.enb_gtp_ebi[e_rab]  = 0;
      }

      struct rrc_ue_s1ap_ids_s *rrc_ue_s1ap_ids =
        rrc_eNB_S1AP_get_ue_ids(RC.rrc[instance], 0,
                                ue_context_p->ue_context.eNB_ue_s1ap_id);

      if (rrc_ue_s1ap_ids)
        rrc_eNB_S1AP_remove_ue_ids(RC.rrc[instance], rrc_ue_s1ap_ids);

      /* trigger UE release in RRC */
      rrc_eNB_remove_ue_context(&ctxt, RC.rrc[instance], ue_context_p);
    } else {
      LOG_E(F1AP, "could not find ue_context of UE RNTI %x\n", rnti);
    }
  }

  pdcp_remove_UE(&ctxt);

  /* notify the agent */
  if (flexran_agent_get_rrc_xface(instance))
    flexran_agent_get_rrc_xface(instance)->flexran_agent_notify_ue_state_change(
      instance, rnti, PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);

  LOG_I(F1AP, "Received UE CONTEXT RELEASE COMPLETE: Removing CU UE entry for RNTI %x\n", rnti);
  f1ap_remove_ue(true, instance, rnti);
  return 0;
}
//void CU_send_UE_CONTEXT_MODIFICATION_REQUEST(F1AP_UEContextModificationRequest_t *UEContextModificationRequest) {
int CU_send_UE_CONTEXT_MODIFICATION_REQUEST(instance_t instance) {
  F1AP_F1AP_PDU_t                        pdu= {0};
  F1AP_UEContextModificationRequest_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  // for test
  cellIDs_t hardCoded= { .mcc=208, .mnc=93, .mnc_digit_length=2};
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, F1AP_InitiatingMessage_t, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModification;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextModificationRequest;
  out = &tmp->value.choice.UEContextModificationRequest;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = 651L;

  /* optional */
  /* c3. NRCGI */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie3);
    ie3->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
    ie3->criticality                    = F1AP_Criticality_ignore;
    ie3->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_NRCGI;
    /* - nRCGI */
    addnRCGI(ie3->value.choice.NRCGI, &hardCoded);
  }

  /* mandatory */
  /* c4. ServCellIndex */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie4);
  ie4->id                             = F1AP_ProtocolIE_ID_id_ServCellIndex;
  ie4->criticality                    = F1AP_Criticality_reject;
  ie4->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ServCellIndex;
  ie4->value.choice.ServCellIndex     = 5L;

  /* optional */
  /* c5. DRXCycle */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie5->criticality                    = F1AP_Criticality_ignore;
    ie5->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRXCycle;
    ie5->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum

    if (0) {
      asn1cCalloc(ie5->value.choice.DRXCycle.shortDRXCycleLength, F1AP_ShortDRXCycleLength_t,tmp);
      *tmp = F1AP_ShortDRXCycleLength_ms2; // enum
    }

    if (0) {
      asn1cCalloc(ie5->value.choice.DRXCycle.shortDRXCycleTimer, F1AP_ShortDRXCycleTimer_t, tmp);
      *tmp = 123L;
    }
  }

  /* optional */
  /* c5. CUtoDURRCInformation */
  if (1) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
    ie5->criticality                    = F1AP_Criticality_reject;
    ie5->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_CUtoDURRCInformation;
    asn1cCalloc(ie5->value.choice.CUtoDURRCInformation.cG_ConfigInfo, F1AP_CG_ConfigInfo_t, tmp);
    /* optional */
    OCTET_STRING_fromBuf(tmp, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    asn1cCalloc(ie5->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList,
                F1AP_UE_CapabilityRAT_ContainerList_t,tmp2);
    /* optional */
    OCTET_STRING_fromBuf(tmp2, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
  }

  /* optional */
  /* c6. TransmissionActionIndicator */
  if (1) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie6);
    ie6->id                                     = F1AP_ProtocolIE_ID_id_TransmissionActionIndicator;
    ie6->criticality                            = F1AP_Criticality_ignore;
    ie6->value.present                          = F1AP_UEContextModificationRequestIEs__value_PR_TransmissionActionIndicator;
    ie6->value.choice.TransmissionActionIndicator = F1AP_TransmissionActionIndicator_stop;
  }

  /* optional */
  /* c7. ResourceCoordinationTransferContainer */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie7);
    ie7->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie7->criticality                    = F1AP_Criticality_ignore;
    ie7->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie7->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
  }

  /* optional */
  /* c7. RRCRconfigurationCompleteIndicator */
  if (1) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie71);
    ie71->id                             = F1AP_ProtocolIE_ID_id_RRCReconfigurationCompleteIndicator;
    ie71->criticality                    = F1AP_Criticality_ignore;
    ie71->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCReconfigurationCompleteIndicator;
    ie71->value.choice.RRCReconfigurationCompleteIndicator = F1AP_RRCReconfigurationCompleteIndicator_true;
  }

  /* optional */
  /* c8. RRCContainer */
  if (1) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie8);
    ie8->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie8->criticality                    = F1AP_Criticality_ignore;
    ie8->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie8->value.choice.RRCContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
  }

  /* mandatory */
  /* c9. SCell_ToBeSetupMod_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie9);
  ie9->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_List;
  ie9->criticality                    = F1AP_Criticality_ignore;
  ie9->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeSetupMod_List;

  for (int i=0; i<1; i++) {
    //
    asn1cSequenceAdd(ie9->value.choice.SCell_ToBeSetupMod_List.list,
                     F1AP_SCell_ToBeSetupMod_ItemIEs_t, scell_toBeSetupMod_item_ies);
    scell_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_Item;
    scell_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
    scell_toBeSetupMod_item_ies->value.present = F1AP_SCell_ToBeSetupMod_ItemIEs__value_PR_SCell_ToBeSetupMod_Item;
    /* 8.1 SCell_ToBeSetup_Item */
    F1AP_SCell_ToBeSetupMod_Item_t *scell_toBeSetupMod_item=
      &scell_toBeSetupMod_item_ies->value.choice.SCell_ToBeSetupMod_Item;
    //   /* - sCell_ID */
    addnRCGI(scell_toBeSetupMod_item->sCell_ID, &hardCoded);
    /* sCellIndex */
    scell_toBeSetupMod_item->sCellIndex = 6;  // issue here
  }

  /* mandatory */
  /* c10. SCell_ToBeRemoved_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie10);
  ie10->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_List;
  ie10->criticality                    = F1AP_Criticality_ignore;
  ie10->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeRemoved_List;

  for (int i=0;  i<1; i++) {
    //
    asn1cSequenceAdd(ie10->value.choice.SCell_ToBeRemoved_List.list,
                     F1AP_SCell_ToBeRemoved_ItemIEs_t, scell_toBeRemoved_item_ies);
    scell_toBeRemoved_item_ies = (F1AP_SCell_ToBeRemoved_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
    //memset((void *)&scell_toBeRemoved_item_ies, 0, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
    scell_toBeRemoved_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_Item;
    scell_toBeRemoved_item_ies->criticality   = F1AP_Criticality_ignore;
    scell_toBeRemoved_item_ies->value.present = F1AP_SCell_ToBeRemoved_ItemIEs__value_PR_SCell_ToBeRemoved_Item;
    /* 10.1 SCell_ToBeRemoved_Item */
    F1AP_SCell_ToBeRemoved_Item_t *scell_toBeRemoved_item=
      &scell_toBeRemoved_item_ies->value.choice.SCell_ToBeRemoved_Item;
    /* - sCell_ID */
    addnRCGI(scell_toBeRemoved_item->sCell_ID, &hardCoded);
  }

  /* mandatory */
  /* c11. SRBs_ToBeSetupMod_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie11);
  ie11->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_List;
  ie11->criticality                    = F1AP_Criticality_reject;
  ie11->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeSetupMod_List;

  for (int i=0;  i<1; i++) {
    //
    asn1cSequenceAdd(ie11->value.choice.SRBs_ToBeSetupMod_List.list,
                     F1AP_SRBs_ToBeSetupMod_ItemIEs_t, srbs_toBeSetupMod_item_ies);
    srbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_Item;
    srbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeSetupMod_item_ies->value.present = F1AP_SRBs_ToBeSetupMod_ItemIEs__value_PR_SRBs_ToBeSetupMod_Item;
    /* 9.1 SRBs_ToBeSetupMod_Item */
    F1AP_SRBs_ToBeSetupMod_Item_t *srbs_toBeSetupMod_item=
      &srbs_toBeSetupMod_item_ies->value.choice.SRBs_ToBeSetupMod_Item;
    /* - sRBID */
    srbs_toBeSetupMod_item->sRBID = 3L;
  }

  /* mandatory */
  /* c12. DRBs_ToBeSetupMod_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie12);
  ie12->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_List;
  ie12->criticality                    = F1AP_Criticality_reject;
  ie12->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeSetupMod_List;

  for (int i=0;   i<1; i++) {
    //
    asn1cSequenceAdd(ie12->value.choice.DRBs_ToBeSetupMod_List.list,
                     F1AP_DRBs_ToBeSetupMod_ItemIEs_t, drbs_toBeSetupMod_item_ies);
    drbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_Item;
    drbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeSetupMod_item_ies->value.present = F1AP_DRBs_ToBeSetupMod_ItemIEs__value_PR_DRBs_ToBeSetupMod_Item;
    /* 12.1 DRBs_ToBeSetupMod_Item */
    F1AP_DRBs_ToBeSetupMod_Item_t *drbs_toBeSetupMod_item=
      &drbs_toBeSetupMod_item_ies->value.choice.DRBs_ToBeSetupMod_Item;
    /* dRBID */
    drbs_toBeSetupMod_item->dRBID = 30L;
    /* qoSInformation */
    drbs_toBeSetupMod_item->qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
    drbs_toBeSetupMod_item->qoSInformation.choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
    drbs_toBeSetupMod_item->qoSInformation.choice.eUTRANQoS->qCI = 253L;
    /* uLUPTNLInformation_ToBeSetup_List */
    int maxnoofULTunnels = 1; // 2;

    for (int j=0;  j<maxnoofULTunnels;  j++) {
      /*  ULTunnels_ToBeSetup_Item */
      asn1cSequenceAdd( drbs_toBeSetupMod_item->uLUPTNLInformation_ToBeSetup_List.list,
                        F1AP_ULUPTNLInformation_ToBeSetup_Item_t, uLUPTNLInformation_ToBeSetup_Item);
      uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
      asn1cCalloc(uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel,
                  F1AP_GTPTunnel_t, gTPTunnel)
      /* transportLayerAddress */
      TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);
      /* gTP_TEID */
      OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "4567",
                           strlen("4567"));
    }

    /* rLCMode */
    drbs_toBeSetupMod_item->rLCMode = F1AP_RLCMode_rlc_um_bidirectional; // enum

    /* OPTIONAL */
    /* ULConfiguration */
    if (0) {
      drbs_toBeSetupMod_item->uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
    }
  }

  /* mandatory */
  /* c13. DRBs_ToBeModified_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie13);
  ie13->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_List;
  ie13->criticality                    = F1AP_Criticality_reject;
  ie13->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeModified_List;

  for (int i=0;   i<1; i++) {
    //
    asn1cSequenceAdd( ie13->value.choice.DRBs_ToBeModified_List.list,
                      F1AP_DRBs_ToBeModified_ItemIEs_t, drbs_toBeModified_item_ies);
    drbs_toBeModified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_Item;
    drbs_toBeModified_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeModified_item_ies->value.present = F1AP_DRBs_ToBeModified_ItemIEs__value_PR_DRBs_ToBeModified_Item;
    /* 13.1 SRBs_ToBeModified_Item */
    F1AP_DRBs_ToBeModified_Item_t *drbs_toBeModified_item=
      &drbs_toBeModified_item_ies->value.choice.DRBs_ToBeModified_Item;
    /* dRBID */
    drbs_toBeModified_item->dRBID = 30L;
    /* qoSInformation */
    asn1cCalloc(drbs_toBeModified_item->qoSInformation, F1AP_QoSInformation_t, tmp);
    tmp->present = F1AP_QoSInformation_PR_eUTRANQoS;
    tmp->choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
    tmp->choice.eUTRANQoS->qCI = 254L;
    /* ULTunnels_ToBeModified_List */
    int maxnoofULTunnels = 1; // 2;

    for (int j=0; j<maxnoofULTunnels; j++) {
      /*  ULTunnels_ToBeModified_Item */
      asn1cSequenceAdd(drbs_toBeModified_item->uLUPTNLInformation_ToBeSetup_List.list,
                       F1AP_ULUPTNLInformation_ToBeSetup_Item_t, uLUPTNLInformation_ToBeSetup_Item);
      uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
      asn1cCalloc(uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel,
                  F1AP_GTPTunnel_t, gTPTunnel);
      TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);
      OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                           strlen("1204"));
    }

    /* OPTIONAL */
    /* ULConfiguration */
    if (0) {
      drbs_toBeModified_item->uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
    }
  }

  /* mandatory */
  /* c14. SRBs_ToBeReleased_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie14);
  ie14->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_List;
  ie14->criticality                    = F1AP_Criticality_reject;
  ie14->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeReleased_List;

  for (int i=0;  i<1; i++) {
    //
    asn1cSequenceAdd(ie14->value.choice.SRBs_ToBeReleased_List.list,
                     F1AP_SRBs_ToBeReleased_ItemIEs_t, srbs_toBeReleased_item_ies);
    srbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_Item;
    srbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeReleased_item_ies->value.present = F1AP_SRBs_ToBeReleased_ItemIEs__value_PR_SRBs_ToBeReleased_Item;
    /* 9.1 SRBs_ToBeReleased_Item */
    F1AP_SRBs_ToBeReleased_Item_t *srbs_toBeReleased_item=
      &srbs_toBeReleased_item_ies->value.choice.SRBs_ToBeReleased_Item;
    /* - sRBID */
    srbs_toBeReleased_item->sRBID = 2L;
  }

  /* mandatory */
  /* c15. DRBs_ToBeReleased_List */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie15);
  ie15->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_List;
  ie15->criticality                    = F1AP_Criticality_reject;
  ie15->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeReleased_List;

  for (int i=0; i<1;  i++) {
    //
    asn1cSequenceAdd(ie15->value.choice.DRBs_ToBeReleased_List.list,
                     F1AP_DRBs_ToBeReleased_ItemIEs_t, drbs_toBeReleased_item_ies);
    drbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_Item;
    drbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeReleased_item_ies->value.present = F1AP_DRBs_ToBeReleased_ItemIEs__value_PR_DRBs_ToBeReleased_Item;
    /* 14.1 SRBs_ToBeReleased_Item */
    F1AP_DRBs_ToBeReleased_Item_t *drbs_toBeReleased_item=
      &drbs_toBeReleased_item_ies->value.choice.DRBs_ToBeReleased_Item;
    /* dRBID */
    drbs_toBeReleased_item->dRBID = 30L;
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE CONTEXT_MODIFICATION REQUEST\n");
    return -1;
  }

  return 0;
}

int CU_handle_UE_CONTEXT_MODIFICATION_RESPONSE(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_MODIFICATION_FAILURE(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_MODIFICATION_REQUIRED(instance_t       instance,
    uint32_t         assoc_id,
    uint32_t         stream,
    F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_UE_CONTEXT_MODIFICATION_CONFIRM(instance_t instance,
    F1AP_UEContextModificationConfirm_t UEContextModificationConfirm_t) {
  AssertFatal(1==0,"Not implemented yet\n");
}
