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

extern f1ap_setup_req_t *f1ap_du_data_from_du;
extern f1ap_cudu_inst_t f1ap_cu_inst[MAX_eNB];
extern RAN_CONTEXT_t RC;
extern uint32_t f1ap_assoc_id;

int CU_send_UE_CONTEXT_SETUP_REQUEST(instance_t instance,
                                     f1ap_ue_context_setup_req_t *f1ap_ue_context_setup_req) {
  F1AP_F1AP_PDU_t                 pdu;
  F1AP_UEContextSetupRequest_t    *out;
  F1AP_UEContextSetupRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0, j = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_UEContextSetupRequest;
  out = &pdu.choice.initiatingMessage->value.choice.UEContextSetupRequest;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_ue_context_setup_req->gNB_CU_ue_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c2. GNB_DU_UE_F1AP_ID */
  if (f1ap_ue_context_setup_req->gNB_DU_ue_id) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
    ie->value.choice.GNB_DU_UE_F1AP_ID = *f1ap_ue_context_setup_req->gNB_DU_ue_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c3. SpCell_ID */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_NRCGI;
  /* - nRCGI */
  F1AP_NRCGI_t nRCGI;
  memset(&nRCGI, 0, sizeof(F1AP_NRCGI_t));
  MCC_MNC_TO_PLMNID(f1ap_du_data_from_du->mcc[0],//f1ap_ue_context_setup_req->mcc,
                    f1ap_du_data_from_du->mnc[0],//f1ap_ue_context_setup_req->mnc,
                    f1ap_du_data_from_du->mnc_digit_length[0],//f1ap_ue_context_setup_req->mnc_digit_length,
                    &nRCGI.pLMN_Identity);
  NR_CELL_ID_TO_BIT_STRING(f1ap_du_data_from_du->nr_cellid[0], &nRCGI.nRCellIdentity);

  ie->value.choice.NRCGI = nRCGI;

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c4. ServCellIndex */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_ServCellIndex;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ServCellIndex;
  ie->value.choice.ServCellIndex = 2;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. CellULConfigured */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_SpCellULConfigured;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CellULConfigured;
    ie->value.choice.CellULConfigured = F1AP_CellULConfigured_ul;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c6. CUtoDURRCInformation */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CUtoDURRCInformation;

  /* optional */
  /* 6.1 cG_ConfigInfo */
  ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo = (F1AP_CG_ConfigInfo_t *)calloc(1, sizeof(F1AP_CG_ConfigInfo_t));
  OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));
  /* optional */
  /* 6.2 uE_CapabilityRAT_ContainerList */
  ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList = (F1AP_UE_CapabilityRAT_ContainerList_t *)calloc(1, sizeof(F1AP_UE_CapabilityRAT_ContainerList_t));
  OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));
  /* optional */
  /* 6.3 measConfig */
  ie->value.choice.CUtoDURRCInformation.measConfig = (F1AP_MeasConfig_t *)calloc(1, sizeof(F1AP_MeasConfig_t));
  OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.measConfig, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

//if (0) {
  /* mandatory */
  /* c7. Candidate_SpCell_List */
  if (0) {
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_Candidate_SpCell_List;  //90
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_Candidate_SpCell_List;

  for (i=0;
       i<0;
       i++) {

    F1AP_Candidate_SpCell_ItemIEs_t *candidate_spCell_item_ies;
    candidate_spCell_item_ies = (F1AP_Candidate_SpCell_ItemIEs_t *)calloc(1, sizeof(F1AP_Candidate_SpCell_ItemIEs_t));
    candidate_spCell_item_ies->id            = F1AP_ProtocolIE_ID_id_Candidate_SpCell_Item; // 91
    candidate_spCell_item_ies->criticality   = F1AP_Criticality_reject;
    candidate_spCell_item_ies->value.present = F1AP_Candidate_SpCell_ItemIEs__value_PR_Candidate_SpCell_Item;

    /* 7.1 Candidate_SpCell_Item */
    F1AP_Candidate_SpCell_Item_t candidate_spCell_item;
    memset((void *)&candidate_spCell_item, 0, sizeof(F1AP_Candidate_SpCell_Item_t));

    /* - candidate_SpCell_ID */
    F1AP_NRCGI_t nRCGI;
    /* TODO add correct mcc/mnc */
    MCC_MNC_TO_PLMNID(f1ap_ue_context_setup_req->mcc,
                      f1ap_ue_context_setup_req->mnc,
                      f1ap_ue_context_setup_req->mnc_digit_length,
                      &nRCGI.pLMN_Identity);
    NR_CELL_ID_TO_BIT_STRING(f1ap_ue_context_setup_req->nr_cellid, &nRCGI.nRCellIdentity);

    candidate_spCell_item.candidate_SpCell_ID = nRCGI;

    /* ADD */
    candidate_spCell_item_ies->value.choice.Candidate_SpCell_Item = candidate_spCell_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.Candidate_SpCell_List.list,
                    candidate_spCell_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c8. DRXCycle */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_DRXCycle;
    /* 8.1 longDRXCycleLength */
    ie->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum

    /* optional */
    /* 8.2 shortDRXCycleLength */
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleLength = (F1AP_ShortDRXCycleLength_t *)calloc(1, sizeof(F1AP_ShortDRXCycleLength_t));
      *ie->value.choice.DRXCycle.shortDRXCycleLength = F1AP_ShortDRXCycleLength_ms2; // enum
    }

    /* optional */
    /* 8.3 shortDRXCycleTimer */
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleTimer = (F1AP_ShortDRXCycleTimer_t *)calloc(1, sizeof(F1AP_ShortDRXCycleTimer_t));
      *ie->value.choice.DRXCycle.shortDRXCycleTimer = 123L;
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c9. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    
    ie->value.choice.ResourceCoordinationTransferContainer.buf = malloc(4);
    ie->value.choice.ResourceCoordinationTransferContainer.size = 4;
    strncpy((char *)ie->value.choice.ResourceCoordinationTransferContainer.buf, "123", 4);


    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c10. SCell_ToBeSetup_List */
  if(0){
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_SCell_ToBeSetup_List;

  for (i=0;
       i<0;
       i++) {
     //
     F1AP_SCell_ToBeSetup_ItemIEs_t *scell_toBeSetup_item_ies;
     scell_toBeSetup_item_ies = (F1AP_SCell_ToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeSetup_ItemIEs_t));
     scell_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_Item; //53
     scell_toBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_toBeSetup_item_ies->value.present = F1AP_SCell_ToBeSetup_ItemIEs__value_PR_SCell_ToBeSetup_Item;

     /* 10.1 SCell_ToBeSetup_Item */
     F1AP_SCell_ToBeSetup_Item_t scell_toBeSetup_item;
     memset((void *)&scell_toBeSetup_item, 0, sizeof(F1AP_SCell_ToBeSetup_Item_t));

     /* 10.1.1 sCell_ID */
     F1AP_NRCGI_t nRCGI;
     /* TODO correct MCC/MNC */
     MCC_MNC_TO_PLMNID(f1ap_ue_context_setup_req->mcc,
                       f1ap_ue_context_setup_req->mnc,
                       f1ap_ue_context_setup_req->mnc_digit_length,
                       &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
     scell_toBeSetup_item.sCell_ID = nRCGI;

     /* 10.1.2 sCellIndex */
     scell_toBeSetup_item.sCellIndex = 3;  // issue here

     /* OPTIONAL */
     /* 10.1.3 sCellULConfigured*/
     if (0) {
       scell_toBeSetup_item.sCellULConfigured = (F1AP_CellULConfigured_t *)calloc(1, sizeof(F1AP_CellULConfigured_t));
       *scell_toBeSetup_item.sCellULConfigured = F1AP_CellULConfigured_ul_and_sul; // enum
     }

     /* ADD */
     scell_toBeSetup_item_ies->value.choice.SCell_ToBeSetup_Item = scell_toBeSetup_item;

     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_ToBeSetup_List.list,
                     scell_toBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }


  /* mandatory */
  /* c11. SRBs_ToBeSetup_List */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_reject;  // ?
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_SRBs_ToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_ToBeSetup_ItemIEs_t *srbs_toBeSetup_item_ies;
    srbs_toBeSetup_item_ies = (F1AP_SRBs_ToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_ToBeSetup_ItemIEs_t));
    srbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_Item; // 73
    srbs_toBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeSetup_item_ies->value.present = F1AP_SRBs_ToBeSetup_ItemIEs__value_PR_SRBs_ToBeSetup_Item;

    /* 11.1 SRBs_ToBeSetup_Item */
    F1AP_SRBs_ToBeSetup_Item_t srbs_toBeSetup_item;
    memset((void *)&srbs_toBeSetup_item, 0, sizeof(F1AP_SRBs_ToBeSetup_Item_t));

    /* 11.1.1 sRBID */
    srbs_toBeSetup_item.sRBID = 2L;

    /* OPTIONAL */
    /* 11.1.2 duplicationIndication */

    //if (0) {
      srbs_toBeSetup_item.duplicationIndication = (F1AP_DuplicationIndication_t *)calloc(1, sizeof(F1AP_DuplicationIndication_t));
      srbs_toBeSetup_item.duplicationIndication = F1AP_DuplicationIndication_true; // enum
    //}

    /* ADD */
    srbs_toBeSetup_item_ies->value.choice.SRBs_ToBeSetup_Item = srbs_toBeSetup_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_ToBeSetup_List.list,
                    srbs_toBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c12. DRBs_ToBeSetup_List */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_DRBs_ToBeSetup_List;

  LOG_I(F1AP, "Length of drbs_to_be_setup: %d \n", f1ap_ue_context_setup_req->drbs_to_be_setup_length);
  for (i = 0; i < f1ap_ue_context_setup_req->drbs_to_be_setup_length; i++) {
    //
    F1AP_DRBs_ToBeSetup_ItemIEs_t *drbs_toBeSetup_item_ies;
    drbs_toBeSetup_item_ies = (F1AP_DRBs_ToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeSetup_ItemIEs_t));
    drbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_Item;
    drbs_toBeSetup_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeSetup_item_ies->value.present = F1AP_DRBs_ToBeSetup_ItemIEs__value_PR_DRBs_ToBeSetup_Item;

    /* 12.1 DRBs_ToBeSetup_Item */
    F1AP_DRBs_ToBeSetup_Item_t drbs_toBeSetup_item;
    memset((void *)&drbs_toBeSetup_item, 0, sizeof(F1AP_DRBs_ToBeSetup_Item_t));

    /* 12.1.1 dRBID */
    drbs_toBeSetup_item.dRBID = f1ap_ue_context_setup_req->drbs_to_be_setup[i].drb_id; // 9

    /* 12.1.2 qoSInformation */
    int some_decide_qos = 0; // BK: Need Check
    if (some_decide_qos) {
      drbs_toBeSetup_item.qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;

      /*  12.1.2.1 eUTRANQoS */
      drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));

      /*  12.1.2.1.1 qCI */
      drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->qCI = 254L;

      /*  12.1.2.1.2 allocationAndRetentionPriority */
      {
        /*  12.1.2.1.2.1 priorityLevel */
        drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->allocationAndRetentionPriority.priorityLevel = F1AP_PriorityLevel_highest; // enum
        
        /*  12.1.2.1.2.2 pre_emptionCapability */
        drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->allocationAndRetentionPriority.pre_emptionCapability = F1AP_Pre_emptionCapability_may_trigger_pre_emption; // enum

        /*  12.1.2.1.2.2 pre_emptionVulnerability */
        drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->allocationAndRetentionPriority.pre_emptionVulnerability = F1AP_Pre_emptionVulnerability_not_pre_emptable; // enum
      }

      /* OPTIONAL */
      /*  12.1.2.1.3 gbrQosInformation */
      if (0) {
        drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->gbrQosInformation = (F1AP_GBR_QosInformation_t *)calloc(1, sizeof(F1AP_GBR_QosInformation_t));
        asn_long2INTEGER(&drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->gbrQosInformation->e_RAB_MaximumBitrateDL, 1L);
        asn_long2INTEGER(&drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->gbrQosInformation->e_RAB_MaximumBitrateUL, 1L);
        asn_long2INTEGER(&drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->gbrQosInformation->e_RAB_GuaranteedBitrateDL, 1L);
        asn_long2INTEGER(&drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->gbrQosInformation->e_RAB_GuaranteedBitrateUL, 1L);
      }

    } else { 
      /* 12.1.2 DRB_Information */
      drbs_toBeSetup_item.qoSInformation.present = F1AP_QoSInformation_PR_choice_extension;

      F1AP_QoSInformation_ExtIEs_t *ie;
      ie = (F1AP_QoSInformation_ExtIEs_t *)calloc(1, sizeof(F1AP_QoS_Characteristics_ExtIEs_t));
      ie->id                             = F1AP_ProtocolIE_ID_id_DRB_Information;
      ie->criticality                    = F1AP_Criticality_reject;
      ie->value.present                  = F1AP_QoSInformation_ExtIEs__value_PR_DRB_Information;
      F1AP_DRB_Information_t   *DRB_Information = &ie->value.choice.DRB_Information;

      drbs_toBeSetup_item.qoSInformation.choice.choice_extension = (struct F1AP_ProtocolIE_SingleContainer*)ie;


      /* 12.1.2.1 dRB_QoS */
      {
        /* qoS_Characteristics */
        {
          int some_decide_qoS_characteristics = 1; // BK: Need Check
          if (some_decide_qoS_characteristics) {
            DRB_Information->dRB_QoS.qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_non_Dynamic_5QI;
            DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI = (F1AP_NonDynamic5QIDescriptor_t *)calloc(1, sizeof(F1AP_NonDynamic5QIDescriptor_t));
            
            /* fiveQI */
            DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI->fiveQI = 1L;

            /* OPTIONAL */
            /* qoSPriorityLevel */
            if (0) {
              DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI->qoSPriorityLevel = (long *)calloc(1, sizeof(long));
              *DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI->qoSPriorityLevel = 1L;
            }

            /* OPTIONAL */
            /* averagingWindow */
            if (0) {
              DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI->averagingWindow = (F1AP_AveragingWindow_t *)calloc(1, sizeof(F1AP_AveragingWindow_t));
              *DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI->averagingWindow = 1L;
            }

            /* OPTIONAL */
            /* maxDataBurstVolume */
            if (0) {
              DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI->maxDataBurstVolume = (F1AP_MaxDataBurstVolume_t *)calloc(1, sizeof(F1AP_MaxDataBurstVolume_t));
              *DRB_Information->dRB_QoS.qoS_Characteristics.choice.non_Dynamic_5QI->maxDataBurstVolume = 1L;
            }

          } else {
            DRB_Information->dRB_QoS.qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_dynamic_5QI;
            DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI = (F1AP_Dynamic5QIDescriptor_t *)calloc(1, sizeof(F1AP_Dynamic5QIDescriptor_t));
            
            /* qoSPriorityLevel */
            DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->qoSPriorityLevel = 1L;

            /* packetDelayBudget */
            DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->packetDelayBudget = 1L;

            /* packetErrorRate */
            DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->packetErrorRate.pER_Scalar = 1L;
            DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->packetErrorRate.pER_Exponent = 6L;


            /* OPTIONAL */
            /* delayCritical */
            if (0) {
              DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->delayCritical = (long *)calloc(1, sizeof(long));
              *DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->delayCritical = 1L;
            }

            /* OPTIONAL */
            /* averagingWindow */
            if (0) {
              DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->averagingWindow = (F1AP_AveragingWindow_t *)calloc(1, sizeof(F1AP_AveragingWindow_t));
              *DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->averagingWindow = 1L;
            }

            /* OPTIONAL */
            /* maxDataBurstVolume */
            if (0) {
              DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->maxDataBurstVolume = (F1AP_MaxDataBurstVolume_t *)calloc(1, sizeof(F1AP_MaxDataBurstVolume_t));
              *DRB_Information->dRB_QoS.qoS_Characteristics.choice.dynamic_5QI->maxDataBurstVolume = 1L;
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
          DRB_Information->dRB_QoS.gBR_QoS_Flow_Information = (F1AP_GBR_QoSFlowInformation_t *)calloc(1, sizeof(F1AP_GBR_QoSFlowInformation_t)); 
          asn_long2INTEGER(&DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxFlowBitRateDownlink, 1L);
          asn_long2INTEGER(&DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxFlowBitRateUplink, 1L);
          asn_long2INTEGER(&DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->guaranteedFlowBitRateDownlink, 1L);
          asn_long2INTEGER(&DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->guaranteedFlowBitRateUplink, 1L);

          /* OPTIONAL */
          /* maxPacketLossRateDownlink */
          if (0) {
            DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxPacketLossRateDownlink = (F1AP_MaxPacketLossRate_t *)calloc(1, sizeof(F1AP_MaxPacketLossRate_t)); 
            *DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxPacketLossRateDownlink = 1L;
          }

          /* OPTIONAL */
          /* maxPacketLossRateUplink */
          if (0) {
            DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxPacketLossRateUplink = (F1AP_MaxPacketLossRate_t *)calloc(1, sizeof(F1AP_MaxPacketLossRate_t)); 
            *DRB_Information->dRB_QoS.gBR_QoS_Flow_Information->maxPacketLossRateUplink = 1L;
          }

        }

        /* OPTIONAL */
        /* reflective_QoS_Attribute */
        if (0) {
          DRB_Information->dRB_QoS.reflective_QoS_Attribute = (long *)calloc(1, sizeof(long)); 
          *DRB_Information->dRB_QoS.reflective_QoS_Attribute = 1L;
        }

      } // dRB_QoS

      /* 12.1.2.2 sNSSAI */
      {
        /* sST */
        OCTET_STRING_fromBuf(&DRB_Information->sNSSAI.sST, "asdsa1d32sa1d31asd31as",
                           strlen("asdsa1d32sa1d31asd31as"));
        /* OPTIONAL */
        /* sD */
        if (0) {
          DRB_Information->sNSSAI.sD = (OCTET_STRING_t *)calloc(1, sizeof(OCTET_STRING_t));
          OCTET_STRING_fromBuf(DRB_Information->sNSSAI.sD, "asdsa1d32sa1d31asd31as",
                           strlen("asdsa1d32sa1d31asd31as"));
        }
      }
      /* OPTIONAL */
      /* 12.1.2.3 notificationControl */
      if (0) {
        DRB_Information->notificationControl = (F1AP_NotificationControl_t *)calloc(1, sizeof(F1AP_NotificationControl_t));
        *DRB_Information->notificationControl = F1AP_NotificationControl_active; // enum
      }

      /* 12.1.2.4 flows_Mapped_To_DRB_List */  // BK: need verifiy
      int k;
      for (k = 0; k < 1; k ++) {
        
        F1AP_Flows_Mapped_To_DRB_Item_t flows_mapped_to_drb_item;
        memset((void *)&flows_mapped_to_drb_item, 0, sizeof(F1AP_Flows_Mapped_To_DRB_Item_t));
        
        /* qoSFlowIndicator */
        flows_mapped_to_drb_item.qoSFlowIdentifier = 1L;

        /* qoSFlowLevelQoSParameters */
        {  
          /* qoS_Characteristics */
          {
            int some_decide_qoS_characteristics = 1; // BK: Need Check
            if (some_decide_qoS_characteristics) {
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_non_Dynamic_5QI;
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI = (F1AP_NonDynamic5QIDescriptor_t *)calloc(1, sizeof(F1AP_NonDynamic5QIDescriptor_t));
              
              /* fiveQI */
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI->fiveQI = 1L;

              /* OPTIONAL */
              /* qoSPriorityLevel */
              if (0) {
                flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI->qoSPriorityLevel = (long *)calloc(1, sizeof(long));
                *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI->qoSPriorityLevel = 1L;
              }

              /* OPTIONAL */
              /* averagingWindow */
              if (0) {
                flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI->averagingWindow = (F1AP_AveragingWindow_t *)calloc(1, sizeof(F1AP_AveragingWindow_t));
                *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI->averagingWindow = 1L;
              }

              /* OPTIONAL */
              /* maxDataBurstVolume */
              if (0) {
                flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI->maxDataBurstVolume = (F1AP_MaxDataBurstVolume_t *)calloc(1, sizeof(F1AP_MaxDataBurstVolume_t));
                *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.non_Dynamic_5QI->maxDataBurstVolume = 1L;
              }

            } else {
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_dynamic_5QI;
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI = (F1AP_Dynamic5QIDescriptor_t *)calloc(1, sizeof(F1AP_Dynamic5QIDescriptor_t));
              
              /* qoSPriorityLevel */
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->qoSPriorityLevel = 1L;

              /* packetDelayBudget */
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->packetDelayBudget = 1L;

              /* packetErrorRate */
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->packetErrorRate.pER_Scalar = 1L;
	      flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->packetErrorRate.pER_Exponent = 6L;
              /* OPTIONAL */
              /* delayCritical */
              if (0) {
                flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->delayCritical = (long *)calloc(1, sizeof(long));
                *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->delayCritical = 1L;
              }

              /* OPTIONAL */
              /* averagingWindow */
              if (0) {
                flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->averagingWindow = (F1AP_AveragingWindow_t *)calloc(1, sizeof(F1AP_AveragingWindow_t));
                *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->averagingWindow = 1L;
              }

              /* OPTIONAL */
              /* maxDataBurstVolume */
              if (0) {
                flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->maxDataBurstVolume = (F1AP_MaxDataBurstVolume_t *)calloc(1, sizeof(F1AP_MaxDataBurstVolume_t));
                *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.qoS_Characteristics.choice.dynamic_5QI->maxDataBurstVolume = 1L;
              }

            } // if some_decide_qoS_characteristics

          } // qoS_Characteristics

          /* nGRANallocationRetentionPriority */
          {
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.priorityLevel = F1AP_PriorityLevel_highest; // enum
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.pre_emptionCapability = F1AP_Pre_emptionCapability_shall_not_trigger_pre_emption; // enum
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.pre_emptionVulnerability = F1AP_Pre_emptionVulnerability_not_pre_emptable; // enum
          } // nGRANallocationRetentionPriority

          /* OPTIONAL */
          /* gBR_QoS_Flow_Information */
          if (0) {
            flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information = (F1AP_GBR_QoSFlowInformation_t *)calloc(1, sizeof(F1AP_GBR_QoSFlowInformation_t)); 
            asn_long2INTEGER(&flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxFlowBitRateDownlink, 1L);
            asn_long2INTEGER(&flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxFlowBitRateUplink, 1L);
            asn_long2INTEGER(&flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->guaranteedFlowBitRateDownlink, 1L);
            asn_long2INTEGER(&flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->guaranteedFlowBitRateUplink, 1L);

            /* OPTIONAL */
            /* maxPacketLossRateDownlink */
            if (0) {
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateDownlink = (F1AP_MaxPacketLossRate_t *)calloc(1, sizeof(F1AP_MaxPacketLossRate_t)); 
              *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateDownlink = 1L;
            }

            /* OPTIONAL */
            /* maxPacketLossRateUplink */
            if (0) {
              flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateUplink = (F1AP_MaxPacketLossRate_t *)calloc(1, sizeof(F1AP_MaxPacketLossRate_t)); 
              *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateUplink = 1L;
            }

          }

          /* OPTIONAL */
          /* reflective_QoS_Attribute */
          if (0) {
            flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.reflective_QoS_Attribute = (long *)calloc(1, sizeof(long)); 
            *flows_mapped_to_drb_item.qoSFlowLevelQoSParameters.reflective_QoS_Attribute = 1L;
          }

        } // qoSFlowLevelQoSParameters
        // BK: need check
        ASN_SEQUENCE_ADD(&DRB_Information->flows_Mapped_To_DRB_List.list, &flows_mapped_to_drb_item);
      }

    } // if some_decide_qos

    /* 12.1.3 uLUPTNLInformation_ToBeSetup_List */
    for (j = 0; j < f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl_length; j++) {
      f1ap_up_tnl_t *up_tnl = &f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl[j];

      /*  12.3.1 ULTunnels_ToBeSetup_Item */
      F1AP_ULUPTNLInformation_ToBeSetup_Item_t *uLUPTNLInformation_ToBeSetup_Item;

      /* 12.3.1.1 gTPTunnel */
      uLUPTNLInformation_ToBeSetup_Item = calloc(1, sizeof(F1AP_ULUPTNLInformation_ToBeSetup_Item_t));
      uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
      F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

      /* 12.3.1.1.1 transportLayerAddress */
      TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(up_tnl->tl_address, &gTPTunnel->transportLayerAddress);

      /* 12.3.1.1.2 gTP_TEID */
      INT32_TO_OCTET_STRING(up_tnl->gtp_teid, &gTPTunnel->gTP_TEID);

      // Add
      uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel = gTPTunnel;
      ASN_SEQUENCE_ADD(&drbs_toBeSetup_item.uLUPTNLInformation_ToBeSetup_List.list, uLUPTNLInformation_ToBeSetup_Item);
    }

    /* 12.1.4 rLCMode */
    /* TODO use rlc_mode from f1ap_drb_to_be_setup */
    switch (f1ap_ue_context_setup_req->drbs_to_be_setup[i].rlc_mode) {
      case RLC_MODE_AM:
        drbs_toBeSetup_item.rLCMode = F1AP_RLCMode_rlc_am;
        break;
      default:
        drbs_toBeSetup_item.rLCMode = F1AP_RLCMode_rlc_um_bidirectional;
    }

    /* OPTIONAL */
    /* 12.1.5 ULConfiguration */
    if (0) {
       drbs_toBeSetup_item.uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
       drbs_toBeSetup_item.uLConfiguration->uLUEConfiguration = F1AP_ULUEConfiguration_no_data;
    }

    /* OPTIONAL */
    /* 12.1.6 duplicationActivation */
    if (0) {
       drbs_toBeSetup_item.duplicationActivation = (F1AP_DuplicationActivation_t *)calloc(1, sizeof(F1AP_DuplicationActivation_t));
       drbs_toBeSetup_item.duplicationActivation = F1AP_DuplicationActivation_active;  // enum
    }

    /* ADD */
    drbs_toBeSetup_item_ies->value.choice.DRBs_ToBeSetup_Item = drbs_toBeSetup_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeSetup_List.list,
                   drbs_toBeSetup_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//}
  /* OPTIONAL */
  /* InactivityMonitoringRequest */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_InactivityMonitoringRequest;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_InactivityMonitoringRequest;
    ie->value.choice.InactivityMonitoringRequest = F1AP_InactivityMonitoringRequest_true; // 0
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* OPTIONAL */
  /* RAT_FrequencyPriorityInformation */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_RAT_FrequencyPriorityInformation;

    int endc = 1; // RK: Get this from somewhere ... 
    if (endc) {
      ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_eNDC;
      ie->value.choice.RAT_FrequencyPriorityInformation.choice.eNDC = 11L;
    } else {
      ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN;
      ie->value.choice.RAT_FrequencyPriorityInformation.choice.nGRAN = 11L;
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* OPTIONAL */
  /* RRCContainer */
  if(f1ap_ue_context_setup_req->rrc_container_length > 0){
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, (const char*)f1ap_ue_context_setup_req->rrc_container,
                        f1ap_ue_context_setup_req->rrc_container_length);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* OPTIONAL */
  /* MaskedIMEISV */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_MaskedIMEISV;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_MaskedIMEISV;
    MaskedIMEISV_TO_BIT_STRING(12340000l, &ie->value.choice.MaskedIMEISV); // size (64)
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  //xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, &pdu); //(void *)pdu
  /* encode */
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

  cu_f1ap_itti_send_sctp_data_req(instance, f1ap_assoc_id /* BK: fix me*/ , buffer, len, 0 /* BK: fix me*/);

  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_RESPONSE(instance_t       instance,
                                        uint32_t         assoc_id,
                                        uint32_t         stream,
                                        F1AP_F1AP_PDU_t *pdu) {
  F1AP_UEContextSetupResponse_t    *container;
  F1AP_UEContextSetupResponseIEs_t *ie;

  DevAssert(pdu);

  container = &pdu->choice.successfulOutcome->value.choice.UEContextSetupResponse;

  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);

  /* DUtoCURRCInformation */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DUtoCURRCInformation, true);

  /* DRBs_Setup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DRBs_Setup_List, true);

  /* SRBs_FailedToBeSetup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_List, true);

  /* DRBs_FailedToBeSetup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_List, true);

  /* SCell_FailedtoSetup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_List, true);

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
  const rnti_t rnti = f1ap_get_rnti_by_cu_id(&f1ap_cu_inst[instance],
                                             ie->value.choice.GNB_CU_UE_F1AP_ID);

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  const rnti_t rnti2 = f1ap_get_rnti_by_du_id(&f1ap_cu_inst[instance],
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
  struct rrc_eNB_ue_context_s *ue_context_pP;
  ue_context_pP = rrc_eNB_get_ue_context(RC.rrc[instance], rnti);
  rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_REQ(
      instance,
      ue_context_pP,
      S1AP_CAUSE_RADIO_NETWORK,
      21); // send cause 21: connection with ue lost

  return 0;
}


int CU_send_UE_CONTEXT_RELEASE_COMMAND(instance_t instance,
                                       f1ap_ue_context_release_cmd_t *cmd) {
  F1AP_F1AP_PDU_t                   pdu;
  F1AP_UEContextReleaseCommand_t    *out;
  F1AP_UEContextReleaseCommandIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_UEContextRelease;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_UEContextReleaseCommand;
  out = &pdu.choice.initiatingMessage->value.choice.UEContextReleaseCommand;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextReleaseCommandIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseCommandIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(&f1ap_cu_inst[instance], cmd->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextReleaseCommandIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseCommandIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(&f1ap_cu_inst[instance], cmd->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. Cause */
  ie = (F1AP_UEContextReleaseCommandIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseCommandIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_Cause;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_Cause;

  switch (cmd->cause) {
    case F1AP_CAUSE_RADIO_NETWORK:
      ie->value.choice.Cause.present = F1AP_Cause_PR_radioNetwork;
      ie->value.choice.Cause.choice.radioNetwork = cmd->cause_value;
      break;
    case F1AP_CAUSE_TRANSPORT:
      ie->value.choice.Cause.present = F1AP_Cause_PR_transport;
      ie->value.choice.Cause.choice.transport = cmd->cause_value;
      break;
    case F1AP_CAUSE_PROTOCOL:
      ie->value.choice.Cause.present = F1AP_Cause_PR_protocol;
      ie->value.choice.Cause.choice.protocol = cmd->cause_value;
      break;
    case F1AP_CAUSE_MISC:
      ie->value.choice.Cause.present = F1AP_Cause_PR_misc;
      ie->value.choice.Cause.choice.misc = cmd->cause_value;
      break;
    case F1AP_CAUSE_NOTHING:
    default:
      ie->value.choice.Cause.present = F1AP_Cause_PR_NOTHING;
      break;
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c4. RRCContainer */
  ie = (F1AP_UEContextReleaseCommandIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseCommandIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_RRCContainer;

  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, (const char *)cmd->rrc_container,
                       cmd->rrc_container_length);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release command\n");
    return -1;
  }

  cu_f1ap_itti_send_sctp_data_req(instance, f1ap_du_data_from_du->assoc_id, buffer, len, 0);

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
  const rnti_t rnti = f1ap_get_rnti_by_cu_id(&f1ap_cu_inst[instance],
                                             ie->value.choice.GNB_CU_UE_F1AP_ID);

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCompleteIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  const rnti_t rnti2 = f1ap_get_rnti_by_du_id(&f1ap_cu_inst[instance],
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

  if (RC.nrrrc[instance]->node_type == ngran_gNB_CU) {
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
  f1ap_remove_ue(&f1ap_cu_inst[instance], rnti);
  return 0;
}


//void CU_send_UE_CONTEXT_MODIFICATION_REQUEST(F1AP_UEContextModificationRequest_t *UEContextModificationRequest) {
int CU_send_UE_CONTEXT_MODIFICATION_REQUEST(instance_t instance) {
  F1AP_F1AP_PDU_t                        pdu;
  F1AP_UEContextModificationRequest_t    *out;
  F1AP_UEContextModificationRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  // for test
  int mcc = 208;
  int mnc = 93;
  int mnc_digit_length = 8;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_UEContextModification;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_UEContextModificationRequest;
  out = &pdu.choice.initiatingMessage->value.choice.UEContextModificationRequest;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. NRCGI */
  if (0) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_NRCGI;
    /* - nRCGI */
    F1AP_NRCGI_t nRCGI;
    MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
                                         &nRCGI.pLMN_Identity);
    NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
    ie->value.choice.NRCGI = nRCGI;

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. ServCellIndex */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_ServCellIndex;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ServCellIndex;
  ie->value.choice.ServCellIndex     = 5L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. DRXCycle */
  if (0) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRXCycle;
    ie->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleLength = (F1AP_ShortDRXCycleLength_t *)calloc(1, sizeof(F1AP_ShortDRXCycleLength_t));
      *ie->value.choice.DRXCycle.shortDRXCycleLength = F1AP_ShortDRXCycleLength_ms2; // enum
    }
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleTimer = (F1AP_ShortDRXCycleTimer_t *)calloc(1, sizeof(F1AP_ShortDRXCycleTimer_t));
      *ie->value.choice.DRXCycle.shortDRXCycleTimer = 123L;
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c5. CUtoDURRCInformation */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_CUtoDURRCInformation;
    ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo = (F1AP_CG_ConfigInfo_t *)calloc(1, sizeof(F1AP_CG_ConfigInfo_t));
    /* optional */
    OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList = (F1AP_UE_CapabilityRAT_ContainerList_t *)calloc(1, sizeof(F1AP_UE_CapabilityRAT_ContainerList_t));
    /* optional */
    OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c6. TransmissionActionIndicator */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                                     = F1AP_ProtocolIE_ID_id_TransmissionActionIndicator;
    ie->criticality                            = F1AP_Criticality_ignore;
    ie->value.present                          = F1AP_UEContextModificationRequestIEs__value_PR_TransmissionActionIndicator;
    ie->value.choice.TransmissionActionIndicator = F1AP_TransmissionActionIndicator_stop;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c7. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c7. RRCRconfigurationCompleteIndicator */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_RRCReconfigurationCompleteIndicator;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCReconfigurationCompleteIndicator;
    ie->value.choice.RRCReconfigurationCompleteIndicator = F1AP_RRCReconfigurationCompleteIndicator_true;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c8. RRCContainer */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c9. SCell_ToBeSetupMod_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_ToBeSetupMod_ItemIEs_t *scell_toBeSetupMod_item_ies;
     scell_toBeSetupMod_item_ies = (F1AP_SCell_ToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeSetupMod_ItemIEs_t));
     //memset((void *)&scell_toBeSetupMod_item_ies, 0, sizeof(F1AP_SCell_ToBeSetupMod_ItemIEs_t));
     scell_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_Item;
     scell_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_toBeSetupMod_item_ies->value.present = F1AP_SCell_ToBeSetupMod_ItemIEs__value_PR_SCell_ToBeSetupMod_Item;

     /* 8.1 SCell_ToBeSetup_Item */
     F1AP_SCell_ToBeSetupMod_Item_t scell_toBeSetupMod_item;
     memset((void *)&scell_toBeSetupMod_item, 0, sizeof(F1AP_SCell_ToBeSetupMod_Item_t));

  //   /* - sCell_ID */
     F1AP_NRCGI_t nRCGI;
     memset(&nRCGI, 0, sizeof(F1AP_NRCGI_t));
     MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
                                        &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
     scell_toBeSetupMod_item.sCell_ID = nRCGI;

     /* sCellIndex */
     scell_toBeSetupMod_item.sCellIndex = 6;  // issue here

     //   /* ADD */
     scell_toBeSetupMod_item_ies->value.choice.SCell_ToBeSetupMod_Item = scell_toBeSetupMod_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_ToBeSetupMod_List.list,
                      scell_toBeSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c10. SCell_ToBeRemoved_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeRemoved_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_ToBeRemoved_ItemIEs_t *scell_toBeRemoved_item_ies;
     scell_toBeRemoved_item_ies = (F1AP_SCell_ToBeRemoved_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
     //memset((void *)&scell_toBeRemoved_item_ies, 0, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
     scell_toBeRemoved_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_Item;
     scell_toBeRemoved_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_toBeRemoved_item_ies->value.present = F1AP_SCell_ToBeRemoved_ItemIEs__value_PR_SCell_ToBeRemoved_Item;

     /* 10.1 SCell_ToBeRemoved_Item */
     F1AP_SCell_ToBeRemoved_Item_t scell_toBeRemoved_item;
     memset((void *)&scell_toBeRemoved_item, 0, sizeof(F1AP_SCell_ToBeRemoved_Item_t));

     /* - sCell_ID */
     F1AP_NRCGI_t nRCGI;
     memset(&nRCGI, 0, sizeof(F1AP_NRCGI_t));
     MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
                                        &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
     scell_toBeRemoved_item.sCell_ID = nRCGI;

     /* ADD */
     scell_toBeRemoved_item_ies->value.choice.SCell_ToBeRemoved_Item = scell_toBeRemoved_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_ToBeRemoved_List.list,
                      scell_toBeRemoved_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c11. SRBs_ToBeSetupMod_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_ToBeSetupMod_ItemIEs_t *srbs_toBeSetupMod_item_ies;
    srbs_toBeSetupMod_item_ies = (F1AP_SRBs_ToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_ToBeSetupMod_ItemIEs_t));
    //memset((void *)&srbs_toBeSetupMod_item_ies, 0, sizeof(F1AP_SRBs_ToBeSetupMod_ItemIEs_t));
    srbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_Item;
    srbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeSetupMod_item_ies->value.present = F1AP_SRBs_ToBeSetupMod_ItemIEs__value_PR_SRBs_ToBeSetupMod_Item;

    /* 9.1 SRBs_ToBeSetupMod_Item */
    F1AP_SRBs_ToBeSetupMod_Item_t srbs_toBeSetupMod_item;
    memset((void *)&srbs_toBeSetupMod_item, 0, sizeof(F1AP_SRBs_ToBeSetupMod_Item_t));

    /* - sRBID */
    srbs_toBeSetupMod_item.sRBID = 3L;

    /* ADD */
    srbs_toBeSetupMod_item_ies->value.choice.SRBs_ToBeSetupMod_Item = srbs_toBeSetupMod_item;

    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_ToBeSetupMod_List.list,
                     srbs_toBeSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c12. DRBs_ToBeSetupMod_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_ToBeSetupMod_ItemIEs_t *drbs_toBeSetupMod_item_ies;
    drbs_toBeSetupMod_item_ies = (F1AP_DRBs_ToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeSetupMod_ItemIEs_t));
    drbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_Item;
    drbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeSetupMod_item_ies->value.present = F1AP_DRBs_ToBeSetupMod_ItemIEs__value_PR_DRBs_ToBeSetupMod_Item;

    /* 12.1 DRBs_ToBeSetupMod_Item */
    F1AP_DRBs_ToBeSetupMod_Item_t drbs_toBeSetupMod_item;
    memset((void *)&drbs_toBeSetupMod_item, 0, sizeof(F1AP_DRBs_ToBeSetupMod_Item_t));

    /* dRBID */
    drbs_toBeSetupMod_item.dRBID = 30L;

    /* qoSInformation */
    drbs_toBeSetupMod_item.qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
    drbs_toBeSetupMod_item.qoSInformation.choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
    drbs_toBeSetupMod_item.qoSInformation.choice.eUTRANQoS->qCI = 253L;

    /* uLUPTNLInformation_ToBeSetup_List */
    int j = 0;
    int maxnoofULTunnels = 1; // 2;
    for (j=0;
            j<maxnoofULTunnels;
            j++) {
            /*  ULTunnels_ToBeSetup_Item */

            F1AP_ULUPTNLInformation_ToBeSetup_Item_t *uLUPTNLInformation_ToBeSetup_Item;
            uLUPTNLInformation_ToBeSetup_Item = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_ULUPTNLInformation_ToBeSetup_Item_t));
            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
            F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

            /* transportLayerAddress */
            TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

            /* gTP_TEID */
            OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "4567",
                             strlen("4567"));

            // Add
            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel = gTPTunnel;
            ASN_SEQUENCE_ADD(&drbs_toBeSetupMod_item.uLUPTNLInformation_ToBeSetup_List.list, uLUPTNLInformation_ToBeSetup_Item);
    }

    /* rLCMode */
    drbs_toBeSetupMod_item.rLCMode = F1AP_RLCMode_rlc_um_bidirectional; // enum

    /* OPTIONAL */
    /* ULConfiguration */
    if (0) {
       drbs_toBeSetupMod_item.uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
    }

    /* ADD */
    drbs_toBeSetupMod_item_ies->value.choice.DRBs_ToBeSetupMod_Item = drbs_toBeSetupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeSetupMod_List.list,
                   drbs_toBeSetupMod_item_ies);
    
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c13. DRBs_ToBeModified_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeModified_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_ToBeModified_ItemIEs_t *drbs_toBeModified_item_ies;
    drbs_toBeModified_item_ies = (F1AP_DRBs_ToBeModified_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeModified_ItemIEs_t));
    drbs_toBeModified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_Item;
    drbs_toBeModified_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeModified_item_ies->value.present = F1AP_DRBs_ToBeModified_ItemIEs__value_PR_DRBs_ToBeModified_Item;

    /* 13.1 SRBs_ToBeModified_Item */
    F1AP_DRBs_ToBeModified_Item_t drbs_toBeModified_item;
    memset((void *)&drbs_toBeModified_item, 0, sizeof(F1AP_DRBs_ToBeModified_Item_t));

    /* dRBID */
    drbs_toBeModified_item.dRBID = 30L;

    /* qoSInformation */
    drbs_toBeModified_item.qoSInformation =calloc(1,sizeof(*drbs_toBeModified_item.qoSInformation));
    drbs_toBeModified_item.qoSInformation->present = F1AP_QoSInformation_PR_eUTRANQoS;
    drbs_toBeModified_item.qoSInformation->choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
    drbs_toBeModified_item.qoSInformation->choice.eUTRANQoS->qCI = 254L;

    /* ULTunnels_ToBeModified_List */
    int j = 0;
    int maxnoofULTunnels = 1; // 2;
    for (j=0;
            j<maxnoofULTunnels;
            j++) {
            /*  ULTunnels_ToBeModified_Item */
            F1AP_ULUPTNLInformation_ToBeSetup_Item_t *uLUPTNLInformation_ToBeSetup_Item;
            uLUPTNLInformation_ToBeSetup_Item = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_ULUPTNLInformation_ToBeSetup_Item_t));
            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
            F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

            TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

            OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                                 strlen("1204"));

            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

            ASN_SEQUENCE_ADD(&drbs_toBeModified_item.uLUPTNLInformation_ToBeSetup_List.list, uLUPTNLInformation_ToBeSetup_Item);
    }

    /* OPTIONAL */
    /* ULConfiguration */
    if (0) {
       drbs_toBeModified_item.uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
    }

    /* ADD */
    drbs_toBeModified_item_ies->value.choice.DRBs_ToBeModified_Item = drbs_toBeModified_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeModified_List.list,
                   drbs_toBeModified_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c14. SRBs_ToBeReleased_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeReleased_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_ToBeReleased_ItemIEs_t *srbs_toBeReleased_item_ies;
    srbs_toBeReleased_item_ies = (F1AP_SRBs_ToBeReleased_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_ToBeReleased_ItemIEs_t));
    //memset((void *)&srbs_toBeReleased_item_ies, 0, sizeof(F1AP_SRBs_ToBeReleased_ItemIEs_t));
    srbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_Item;
    srbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeReleased_item_ies->value.present = F1AP_SRBs_ToBeReleased_ItemIEs__value_PR_SRBs_ToBeReleased_Item;

    /* 9.1 SRBs_ToBeReleased_Item */
    F1AP_SRBs_ToBeReleased_Item_t srbs_toBeReleased_item;
    memset((void *)&srbs_toBeReleased_item, 0, sizeof(F1AP_SRBs_ToBeReleased_Item_t));

    /* - sRBID */
    srbs_toBeReleased_item.sRBID = 2L;

    /* ADD */
    srbs_toBeReleased_item_ies->value.choice.SRBs_ToBeReleased_Item = srbs_toBeReleased_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_ToBeReleased_List.list,
                    srbs_toBeReleased_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c15. DRBs_ToBeReleased_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeReleased_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_ToBeReleased_ItemIEs_t *drbs_toBeReleased_item_ies;
    drbs_toBeReleased_item_ies = (F1AP_DRBs_ToBeReleased_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeReleased_ItemIEs_t));
    drbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_Item;
    drbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeReleased_item_ies->value.present = F1AP_DRBs_ToBeReleased_ItemIEs__value_PR_DRBs_ToBeReleased_Item;

    /* 14.1 SRBs_ToBeReleased_Item */
    F1AP_DRBs_ToBeReleased_Item_t drbs_toBeReleased_item;
    memset((void *)&drbs_toBeReleased_item, 0, sizeof(F1AP_DRBs_ToBeReleased_Item_t));

    /* dRBID */
    drbs_toBeReleased_item.dRBID = 30L;

    /* ADD */
    drbs_toBeReleased_item_ies->value.choice.DRBs_ToBeReleased_Item = drbs_toBeReleased_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeReleased_List.list,
                    drbs_toBeReleased_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

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
