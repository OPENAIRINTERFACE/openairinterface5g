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

/*! \file f1ap_du_ue_context_management.c
 * \brief F1AP UE Context Management, DU side
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
#include "f1ap_du_ue_context_management.h"

#include "rrc_extern.h"
#include "rrc_eNB_UE_context.h"

// undefine C_RNTI from
// openair1/PHY/LTE_TRANSPORT/transport_common.h which
// replaces in ie->value.choice.C_RNTI, causing
// a compile error

#undef C_RNTI 

extern f1ap_setup_req_t *f1ap_du_data;
extern f1ap_cudu_inst_t f1ap_du_inst[MAX_eNB];
extern RAN_CONTEXT_t RC;

int DU_handle_UE_CONTEXT_SETUP_REQUEST(instance_t       instance,
                                       uint32_t         assoc_id,
                                       uint32_t         stream,
                                       F1AP_F1AP_PDU_t *pdu)
{
  MessageDef                      *msg_p; // message to RRC
  F1AP_UEContextSetupRequest_t    *container;
  F1AP_UEContextSetupRequestIEs_t *ie;
  int i;

  DevAssert(pdu);

  msg_p = itti_alloc_new_message(TASK_DU_F1, F1AP_UE_CONTEXT_SETUP_REQ);
	f1ap_ue_context_setup_req_t *f1ap_ue_context_setup_req;
	f1ap_ue_context_setup_req = &F1AP_UE_CONTEXT_SETUP_REQ(msg_p);

  container = &pdu->choice.initiatingMessage->value.choice.UEContextSetupRequest;

  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  f1ap_ue_context_setup_req->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

  /* optional */
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, false);
  if (ie) {
    f1ap_ue_context_setup_req->gNB_DU_ue_id = malloc(sizeof(uint32_t));
    if (f1ap_ue_context_setup_req->gNB_DU_ue_id)
      *f1ap_ue_context_setup_req->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  } else {
    f1ap_ue_context_setup_req->gNB_DU_ue_id = NULL;
  }

  /* SpCell_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SpCell_ID, true);
  PLMNID_TO_MCC_MNC(&ie->value.choice.NRCGI.pLMN_Identity,
										f1ap_ue_context_setup_req->mcc,
										f1ap_ue_context_setup_req->mnc,
                    f1ap_ue_context_setup_req->mnc_digit_length);
  BIT_STRING_TO_NR_CELL_IDENTITY(&ie->value.choice.NRCGI.nRCellIdentity, f1ap_ue_context_setup_req->nr_cellid);


  /* ServCellIndex */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_ServCellndex, true);
  f1ap_ue_context_setup_req->servCellIndex = ie->value.choice.ServCellIndex;

  /* optional */
  /* CellULConfigured */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SpCellULConfigured, false);  // SpCellULConfigured
  if (ie) {
    /* correct here */
    f1ap_ue_context_setup_req->cellULConfigured = malloc(sizeof(uint32_t));
    if (f1ap_ue_context_setup_req->cellULConfigured)
      *f1ap_ue_context_setup_req->cellULConfigured = ie->value.choice.CellULConfigured;
  } else {
    f1ap_ue_context_setup_req->cellULConfigured = NULL;
  }

  /* CUtoDURRCInformation */


  /* Candidate_SpCell_List */


  /* optional */
  /* DRXCycle */

  /* optional */
  /* ResourceCoordinationTransferContainer */


  /* SCell_ToBeSetup_List */

  /* SRBs_ToBeSetup_List */

  /* DRBs_ToBeSetup_List */

  /* Decode DRBs_ToBeSetup_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_List, true);
  f1ap_ue_context_setup_req->drbs_to_be_setup_length = ie->value.choice.DRBs_ToBeSetup_List.list.count;
  f1ap_ue_context_setup_req->drbs_to_be_setup = calloc(f1ap_ue_context_setup_req->drbs_to_be_setup_length,
                                                       sizeof(f1ap_drb_to_be_setup_t));
  AssertFatal(f1ap_ue_context_setup_req->drbs_to_be_setup,
              "could not allocate memory for f1ap_ue_context_setup_req->drbs_to_be_setup\n");
  for (i = 0; i < f1ap_ue_context_setup_req->drbs_to_be_setup_length; ++i) {
    f1ap_drb_to_be_setup_t *drb_p = &f1ap_ue_context_setup_req->drbs_to_be_setup[i];
    F1AP_DRBs_ToBeSetup_Item_t *drbs_tobesetup_item_p;
    drbs_tobesetup_item_p = &((F1AP_DRBs_ToBeSetup_ItemIEs_t *)ie->value.choice.DRBs_ToBeSetup_List.list.array[i])->value.choice.DRBs_ToBeSetup_Item;

    drb_p->drb_id = drbs_tobesetup_item_p->dRBID;

    /* TODO in the following, assume only one UP UL TNL is present.
     * this matches/assumes OAI CU implementation, can be up to 2! */
    drb_p->up_ul_tnl_length = 1;
    AssertFatal(drbs_tobesetup_item_p->uLUPTNLInformation_ToBeSetup_List.list.count > 0,
                "no UL UP TNL Information in DRBs to be Setup list\n");
    F1AP_ULUPTNLInformation_ToBeSetup_Item_t *ul_up_tnl_info_p = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)drbs_tobesetup_item_p->uLUPTNLInformation_ToBeSetup_List.list.array[0];
    F1AP_GTPTunnel_t *ul_up_tnl0 = ul_up_tnl_info_p->uLUPTNLInformation.choice.gTPTunnel;
    BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&ul_up_tnl0->transportLayerAddress, drb_p->up_ul_tnl[0].tl_address);
    OCTET_STRING_TO_INT32(&ul_up_tnl0->gTP_TEID, drb_p->up_ul_tnl[0].gtp_teid);

    switch (drbs_tobesetup_item_p->rLCMode) {
    case F1AP_RLCMode_rlc_am:
      drb_p->rlc_mode = RLC_MODE_AM;
      break;
    default:
      drb_p->rlc_mode = RLC_MODE_TM;
      break;
    }
  }

  AssertFatal(0, "check configuration, send to appropriate handler\n");

  return 0;
}

//void DU_send_UE_CONTEXT_SETUP_RESPONSE(F1AP_UEContextSetupResponse_t *UEContextSetupResponse) {
int DU_send_UE_CONTEXT_SETUP_RESPONSE(instance_t instance) {
  F1AP_F1AP_PDU_t                  pdu;
  F1AP_UEContextSetupResponse_t    *out;
  F1AP_UEContextSetupResponseIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0, j = 0;

  rnti_t    rntiP;  // note: need get value

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (F1AP_SuccessfulOutcome_t *)calloc(1, sizeof(F1AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
  pdu.choice.successfulOutcome->criticality   = F1AP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextSetupResponse;
  out = &pdu.choice.successfulOutcome->value.choice.UEContextSetupResponse;

  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. DUtoCURRCInformation */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCInformation;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DUtoCURRCInformation;
  {
    /* cellGroupConfig */
    OCTET_STRING_fromBuf(&ie->value.choice.DUtoCURRCInformation.cellGroupConfig, "asdsa",
                         strlen("asdsa"));
    /* OPTIONAL */
    /* measGapConfig */
    if (0) {
      ie->value.choice.DUtoCURRCInformation.measGapConfig = (F1AP_MeasGapConfig_t *)calloc(1, sizeof(F1AP_MeasGapConfig_t));
      OCTET_STRING_fromBuf( ie->value.choice.DUtoCURRCInformation.measGapConfig, "asdsa",
                           strlen("asdsa"));
    }

    /* OPTIONAL */
    /* requestedP_MaxFR1 */
    if (0) {
      ie->value.choice.DUtoCURRCInformation.requestedP_MaxFR1 = (OCTET_STRING_t *)calloc(1, sizeof(OCTET_STRING_t));
      OCTET_STRING_fromBuf( ie->value.choice.DUtoCURRCInformation.requestedP_MaxFR1, "asdsa",
                           strlen("asdsa"));
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c4. C_RNTI */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_C_RNTI;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_C_RNTI;
    C_RNTI_TO_BIT_STRING(rntiP, &ie->value.choice.C_RNTI);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c5. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa",
                         strlen("asdsa"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c6. FullConfiguration */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_FullConfiguration;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_FullConfiguration;
    ie->value.choice.FullConfiguration = F1AP_FullConfiguration_full;   //enum
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c7. DRBs_Setup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_Setup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DRBs_Setup_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_DRBs_Setup_ItemIEs_t *drbs_setup_item_ies;
     drbs_setup_item_ies = (F1AP_DRBs_Setup_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_Setup_ItemIEs_t));
     drbs_setup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_Setup_Item;
     drbs_setup_item_ies->criticality   = F1AP_Criticality_ignore;
     drbs_setup_item_ies->value.present = F1AP_SRBs_FailedToBeSetup_ItemIEs__value_PR_SRBs_FailedToBeSetup_Item;

     /* 7.1 DRBs_Setup_Item */
     F1AP_DRBs_Setup_Item_t drbs_setup_item;
     memset((void *)&drbs_setup_item, 0, sizeof(F1AP_DRBs_Setup_Item_t));

     /* dRBID */
     drbs_setup_item.dRBID = 12;
     
     /* OPTIONAL */
     /* lCID */
     //drbs_setup_item.lCID = (F1AP_LCID_t *)calloc(1, sizeof(F1AP_LCID_t));
     //drbs_setup_item.lCID = 1L;

     for (j=0;
       j<1;
       j++) {

       F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dLUPTNLInformation_ToBeSetup_Item;
       dLUPTNLInformation_ToBeSetup_Item = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_DLUPTNLInformation_ToBeSetup_Item_t));
       dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
       
       /* gTPTunnel */
       F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));
       {
         /* transportLayerAddress */
         TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

         /* gTP_TEID */
         OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                               strlen("1204"));
         dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel = gTPTunnel;
       }
       /* ADD */
       ASN_SEQUENCE_ADD(&drbs_setup_item.dLUPTNLInformation_ToBeSetup_List.list,
                        dLUPTNLInformation_ToBeSetup_Item);
     } // for j

     /* ADD */
     drbs_setup_item_ies->value.choice.DRBs_Setup_Item = drbs_setup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_Setup_List.list,
                     drbs_setup_item_ies);
  } // for i
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c8. SRBs_FailedToBeSetup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_SRBs_FailedToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
      //
      F1AP_SRBs_FailedToBeSetup_ItemIEs_t *srbs_failedToBeSetup_item_ies;
      srbs_failedToBeSetup_item_ies = (F1AP_SRBs_FailedToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_FailedToBeSetup_ItemIEs_t));
      srbs_failedToBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_Item;
      srbs_failedToBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
      srbs_failedToBeSetup_item_ies->value.present = F1AP_SRBs_FailedToBeSetup_ItemIEs__value_PR_SRBs_FailedToBeSetup_Item;

      /* 8.1 SRBs_Setup_Item */
      F1AP_SRBs_FailedToBeSetup_Item_t srbs_failedToBeSetup_item;
      memset((void *)&srbs_failedToBeSetup_item, 0, sizeof(F1AP_SRBs_FailedToBeSetup_Item_t));

      /* sRBID */
      srbs_failedToBeSetup_item.sRBID = 13L;

      /* cause */
      srbs_failedToBeSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));

      // dummy value
      srbs_failedToBeSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;

      switch(srbs_failedToBeSetup_item.cause->present)
      {
        case F1AP_Cause_PR_radioNetwork:
          srbs_failedToBeSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unspecified;
          break;
        case F1AP_Cause_PR_transport:
          srbs_failedToBeSetup_item.cause->choice.transport = F1AP_CauseTransport_unspecified;
          break;
        case F1AP_Cause_PR_protocol:
          srbs_failedToBeSetup_item.cause->choice.protocol = F1AP_CauseProtocol_unspecified;
          break;
        case F1AP_Cause_PR_misc:
          srbs_failedToBeSetup_item.cause->choice.misc = F1AP_CauseMisc_unspecified;
          break;
        case F1AP_Cause_PR_NOTHING:
        default:
          break;
      } // switch

      /* ADD */
      srbs_failedToBeSetup_item_ies->value.choice.SRBs_FailedToBeSetup_Item = srbs_failedToBeSetup_item;
      ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_FailedToBeSetup_List.list,
                       srbs_failedToBeSetup_item_ies);
  } // for i
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /*  */
  /* c9. DRBs_FailedToBeSetup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DRBs_FailedToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
      //
      F1AP_DRBs_FailedToBeSetup_ItemIEs_t *drbs_failedToBeSetup_item_ies;
      drbs_failedToBeSetup_item_ies = (F1AP_DRBs_FailedToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_FailedToBeSetup_ItemIEs_t));
      drbs_failedToBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_Item;
      drbs_failedToBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
      drbs_failedToBeSetup_item_ies->value.present = F1AP_DRBs_FailedToBeSetup_ItemIEs__value_PR_DRBs_FailedToBeSetup_Item;

      /* 9.1 DRBs_Setup_Item */
      F1AP_DRBs_FailedToBeSetup_Item_t drbs_failedToBeSetup_item;
      memset((void *)&drbs_failedToBeSetup_item, 0, sizeof(F1AP_DRBs_FailedToBeSetup_Item_t));

      /* dRBID */
      drbs_failedToBeSetup_item.dRBID = 14;

      /* cause */
      drbs_failedToBeSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));

      // dummy value
      drbs_failedToBeSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;

      switch(drbs_failedToBeSetup_item.cause->present)
      {
        case F1AP_Cause_PR_radioNetwork:
          drbs_failedToBeSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unspecified;
          break;
        case F1AP_Cause_PR_transport:
          drbs_failedToBeSetup_item.cause->choice.transport = F1AP_CauseTransport_unspecified;
          break;
        case F1AP_Cause_PR_protocol:
          drbs_failedToBeSetup_item.cause->choice.protocol = F1AP_CauseProtocol_unspecified;
          break;
        case F1AP_Cause_PR_misc:
          drbs_failedToBeSetup_item.cause->choice.misc = F1AP_CauseMisc_unspecified;
          break;
        case F1AP_Cause_PR_NOTHING:
        default:
          break;
      } // switch

      /* ADD */
      drbs_failedToBeSetup_item_ies->value.choice.DRBs_FailedToBeSetup_Item = drbs_failedToBeSetup_item;
      ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_FailedToBeSetup_List.list,
                     drbs_failedToBeSetup_item_ies);
  } // for i
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c10. SCell_FailedtoSetup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_SCell_FailedtoSetup_List;

  for (i=0;
       i<1;
       i++) {
      //
      F1AP_SCell_FailedtoSetup_ItemIEs_t *sCell_FailedtoSetup_item_ies;
      sCell_FailedtoSetup_item_ies = (F1AP_SCell_FailedtoSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_FailedtoSetup_ItemIEs_t));
      sCell_FailedtoSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_Item;
      sCell_FailedtoSetup_item_ies->criticality   = F1AP_Criticality_ignore;
      sCell_FailedtoSetup_item_ies->value.present = F1AP_SCell_FailedtoSetup_ItemIEs__value_PR_SCell_FailedtoSetup_Item;

      /* 10.1 DRBs_Setup_Item */
      F1AP_SCell_FailedtoSetup_Item_t sCell_FailedtoSetup_item;
      memset((void *)&sCell_FailedtoSetup_item, 0, sizeof(F1AP_SCell_FailedtoSetup_Item_t));

      /* sCell_ID */
      F1AP_NRCGI_t nRCGI;  // issue here
      MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i], &nRCGI.pLMN_Identity);

      NR_CELL_ID_TO_BIT_STRING(f1ap_du_data->nr_cellid[0], &nRCGI.nRCellIdentity);

      sCell_FailedtoSetup_item.sCell_ID = nRCGI;

      /* cause */
      sCell_FailedtoSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));

      // dummy value
      sCell_FailedtoSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;

      switch(sCell_FailedtoSetup_item.cause->present)
      {
        case F1AP_Cause_PR_radioNetwork:
          sCell_FailedtoSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unspecified;
          break;
        case F1AP_Cause_PR_transport:
          sCell_FailedtoSetup_item.cause->choice.transport = F1AP_CauseTransport_unspecified;
          break;
        case F1AP_Cause_PR_protocol:
          sCell_FailedtoSetup_item.cause->choice.protocol = F1AP_CauseProtocol_unspecified;
          break;
        case F1AP_Cause_PR_misc:
          sCell_FailedtoSetup_item.cause->choice.misc = F1AP_CauseMisc_unspecified;
          break;
        case F1AP_Cause_PR_NOTHING:
        default:
          break;
      } // switch

      /* ADD */
      sCell_FailedtoSetup_item_ies->value.choice.SCell_FailedtoSetup_Item = sCell_FailedtoSetup_item;
      ASN_SEQUENCE_ADD(&ie->value.choice.SCell_FailedtoSetup_List.list,
                      sCell_FailedtoSetup_item_ies);
  } // for i
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
 

  /* Optional */
  /* c11. InactivityMonitoringResponse */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                                        = F1AP_ProtocolIE_ID_id_InactivityMonitoringResponse;
    ie->criticality                               = F1AP_Criticality_ignore;
    ie->value.present                             = F1AP_UEContextSetupResponseIEs__value_PR_InactivityMonitoringResponse;
    ie->value.choice.InactivityMonitoringResponse = F1AP_InactivityMonitoringResponse_not_supported;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }


  /* Optional */
  /* c12. CriticalityDiagnostics */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_CriticalityDiagnostics;
    
    ie->value.choice.CriticalityDiagnostics.procedureCode = (F1AP_ProcedureCode_t *)calloc(1, sizeof(F1AP_ProcedureCode_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
    
    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)calloc(1, sizeof(F1AP_TriggeringMessage_t));
    *ie->value.choice.CriticalityDiagnostics.triggeringMessage = F1AP_TriggeringMessage_initiating_message;
    
    ie->value.choice.CriticalityDiagnostics.procedureCriticality = (F1AP_Criticality_t *)calloc(1, sizeof(F1AP_Criticality_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCriticality = F1AP_Criticality_reject;
    
    ie->value.choice.CriticalityDiagnostics.transactionID = (F1AP_TransactionID_t *)calloc(1, sizeof(F1AP_TransactionID_t));
    *ie->value.choice.CriticalityDiagnostics.transactionID = 0;
    
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }


  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup request\n");
    return -1;
  }

  return 0;
}

int DU_send_UE_CONTEXT_SETUP_FAILURE(instance_t instance) {
  AssertFatal(1==0,"Not implemented yet\n");
}


int DU_send_UE_CONTEXT_RELEASE_REQUEST(instance_t instance,
                                       f1ap_ue_context_release_req_t *req) {
  F1AP_F1AP_PDU_t                   pdu;
  F1AP_UEContextReleaseRequest_t    *out;
  F1AP_UEContextReleaseRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  //int       i = 0, j = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_UEContextReleaseRequest;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_UEContextReleaseRequest;
  out = &pdu.choice.initiatingMessage->value.choice.UEContextReleaseRequest;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextReleaseRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextReleaseRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(&f1ap_du_inst[instance], req->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextReleaseRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextReleaseRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(&f1ap_du_inst[instance], req->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. Cause */
  ie = (F1AP_UEContextReleaseRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_Cause;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextReleaseRequestIEs__value_PR_Cause;

  switch (req->cause)
  {
    case F1AP_CAUSE_RADIO_NETWORK:
      ie->value.choice.Cause.present = F1AP_Cause_PR_radioNetwork;
      ie->value.choice.Cause.choice.radioNetwork = req->cause_value;
      break;
    case F1AP_CAUSE_TRANSPORT:
      ie->value.choice.Cause.present = F1AP_Cause_PR_transport;
      ie->value.choice.Cause.choice.transport = req->cause_value;
      break;
    case F1AP_CAUSE_PROTOCOL:
      ie->value.choice.Cause.present = F1AP_Cause_PR_protocol;
      ie->value.choice.Cause.choice.protocol = req->cause_value;
      break;
    case F1AP_CAUSE_MISC:
      ie->value.choice.Cause.present = F1AP_Cause_PR_misc;
      ie->value.choice.Cause.choice.misc = req->cause_value;
      break;
    case F1AP_CAUSE_NOTHING:
    default:
      ie->value.choice.Cause.present = F1AP_Cause_PR_NOTHING;
      break;
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release request\n");
    return -1;
  }

  du_f1ap_itti_send_sctp_data_req(instance,
      f1ap_du_data->assoc_id,
      buffer,
      len,
      f1ap_du_data->default_sctp_stream_id);

  return 0;
}


int DU_handle_UE_CONTEXT_RELEASE_COMMAND(instance_t       instance,
                                         uint32_t         assoc_id,
                                         uint32_t         stream,
                                         F1AP_F1AP_PDU_t *pdu) {

  F1AP_UEContextReleaseCommand_t *container;
  F1AP_UEContextReleaseCommandIEs_t *ie;
  protocol_ctxt_t ctxt;

  DevAssert(pdu);

  container = &pdu->choice.initiatingMessage->value.choice.UEContextReleaseCommand;

  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  ctxt.rnti = f1ap_get_rnti_by_cu_id(&f1ap_du_inst[instance], ie->value.choice.GNB_CU_UE_F1AP_ID);
  ctxt.module_id = instance;
  ctxt.instance  = instance;
  ctxt.enb_flag  = 1;

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  const rnti_t rnti = f1ap_get_rnti_by_du_id(&f1ap_du_inst[instance],
                                             ie->value.choice.GNB_DU_UE_F1AP_ID);
  AssertFatal(ctxt.rnti == rnti,
              "RNTI obtained through DU ID (%x) is different from CU ID (%x)\n",
              rnti, ctxt.rnti);

  int UE_out_of_sync = 0;
  for (int n = 0; n < MAX_MOBILES_PER_ENB; ++n) {
    if (RC.mac[instance]->UE_list.active[n] == TRUE
        && rnti == UE_RNTI(instance, n)) {
      UE_out_of_sync = RC.mac[instance]->UE_list.UE_sched_ctrl[n].ul_out_of_sync;
      break;
    }
  }

  /* We don't need the Cause */

  /* Optional RRC Container: if present, send to UE */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, false);
  if (ie && !UE_out_of_sync) {
    /* RRC message and UE is reachable, send message */
    const sdu_size_t sdu_len = ie->value.choice.RRCContainer.size;
    mem_block_t *pdu_p = NULL;
    pdu_p = get_free_mem_block(sdu_len, __func__);
    memcpy(&pdu_p->data[0], ie->value.choice.RRCContainer.buf, sdu_len);
    rlc_op_status_t rlc_status = rlc_data_req(&ctxt
                                              , 1
                                              , MBMS_FLAG_NO
                                              , 1 // SRB 1 correct?
                                              , 0
                                              , 0
                                              , sdu_len
                                              , pdu_p
#ifdef Rel14
                                              ,NULL
                                              ,NULL
#endif
                                              );
    switch (rlc_status) {
      case RLC_OP_STATUS_OK:
        break;
      case RLC_OP_STATUS_BAD_PARAMETER:
        LOG_W(F1AP, "Data sending request over RLC failed with 'Bad Parameter' reason!\n");
        break;
      case RLC_OP_STATUS_INTERNAL_ERROR:
        LOG_W(F1AP, "Data sending request over RLC failed with 'Internal Error' reason!\n");
        break;
      case RLC_OP_STATUS_OUT_OF_RESSOURCES:
        LOG_W(F1AP, "Data sending request over RLC failed with 'Out of Resources' reason!\n");
        break;
      default:
        LOG_W(F1AP, "RLC returned an unknown status code after F1AP placed "
              "the order to send some data (Status Code:%d)\n", rlc_status);
        break;
    }
  }

  struct rrc_eNB_ue_context_s* ue_context_p;
  ue_context_p = rrc_eNB_get_ue_context(RC.rrc[ctxt.module_id], ctxt.rnti);
  if (ue_context_p && !UE_out_of_sync) {
    /* UE exists and is in sync so we start a timer before releasing the
     * connection */
    pthread_mutex_lock(&rrc_release_freelist);
    for (uint16_t release_num = 0; release_num < NUMBER_OF_UE_MAX; release_num++) {
      if (rrc_release_info.RRC_release_ctrl[release_num].flag == 0) {
        if (ue_context_p->ue_context.ue_release_timer_s1 > 0)
          rrc_release_info.RRC_release_ctrl[release_num].flag = 1;
        else
          rrc_release_info.RRC_release_ctrl[release_num].flag = 2;
        rrc_release_info.RRC_release_ctrl[release_num].rnti = ctxt.rnti;
        LOG_D(F1AP, "add rrc_release_info RNTI %x\n", ctxt.rnti);
        // TODO: how to provide the correct MUI?
        rrc_release_info.RRC_release_ctrl[release_num].rrc_eNB_mui = 0;
        rrc_release_info.num_UEs++;
        LOG_D(RRC,"Generate DLSCH Release send: index %d rnti %x mui %d flag %d \n",release_num,
              ctxt.rnti, 0, rrc_release_info.RRC_release_ctrl[release_num].flag);
        break;
      }
    }
    pthread_mutex_unlock(&rrc_release_freelist);
    ue_context_p->ue_context.ue_release_timer_s1 = 0;
  } else if (ue_context_p && UE_out_of_sync) {
    /* UE exists and is out of sync, drop the connection */
    mac_eNB_rrc_ul_failure(instance, 0, 0, 0, rnti);
  } else {
    LOG_E(F1AP, "no ue_context for RNTI %x, acknowledging release\n", rnti);
  }
  
  /* TODO send this once the connection has really been released */
  f1ap_ue_context_release_cplt_t cplt;
  cplt.rnti = ctxt.rnti;
  DU_send_UE_CONTEXT_RELEASE_COMPLETE(instance, &cplt);

  return 0;
}


int DU_send_UE_CONTEXT_RELEASE_COMPLETE(instance_t instance,
                                        f1ap_ue_context_release_cplt_t *cplt) {
  F1AP_F1AP_PDU_t                     pdu;
  F1AP_UEContextReleaseComplete_t    *out;
  F1AP_UEContextReleaseCompleteIEs_t *ie;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (F1AP_SuccessfulOutcome_t *)calloc(1, sizeof(F1AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = F1AP_ProcedureCode_id_UEContextRelease;
  pdu.choice.successfulOutcome->criticality   = F1AP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextReleaseComplete;
  out = &pdu.choice.successfulOutcome->value.choice.UEContextReleaseComplete;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextReleaseCompleteIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseCompleteIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextReleaseCompleteIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(&f1ap_du_inst[instance], cplt->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextReleaseCompleteIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseCompleteIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextReleaseCompleteIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(&f1ap_du_inst[instance], cplt->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional -> currently not used */
  /* c3. CriticalityDiagnostics */
  //if (0) {
  //  ie = (F1AP_UEContextReleaseCompleteIEs_t *)calloc(1, sizeof(F1AP_UEContextReleaseCompleteIEs_t));
  //  ie->id                             = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
  //  ie->criticality                    = F1AP_Criticality_ignore;
  //  ie->value.present                  = F1AP_UEContextReleaseCompleteIEs__value_PR_CriticalityDiagnostics;

  //  // dummy value
  //  /* optional */
  //  /* procedureCode */
  //  if (0) {
  //    ie->value.choice.CriticalityDiagnostics.procedureCode = (F1AP_ProcedureCode_t *)calloc(1, sizeof(F1AP_ProcedureCode_t));
  //    ie->value.choice.CriticalityDiagnostics.procedureCode = 0L;
  //  }

  //  /* optional */
  //  /* triggeringMessage */
  //  if (0) {
  //    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)calloc(1, sizeof(F1AP_TriggeringMessage_t));
  //    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)F1AP_TriggeringMessage_successful_outcome;
  //  }

  //  /* optional */
  //  /* procedureCriticality */
  //  if (0) {
  //    ie->value.choice.CriticalityDiagnostics.procedureCriticality = (F1AP_Criticality_t *)calloc(1, sizeof(F1AP_Criticality_t));
  //    ie->value.choice.CriticalityDiagnostics.procedureCriticality = F1AP_Criticality_reject;
  //  }

  //  /* optional */
  //  /* transactionID */
  //  if (0) {
  //    ie->value.choice.CriticalityDiagnostics.transactionID = (F1AP_TransactionID_t *)calloc(1, sizeof(F1AP_TransactionID_t));
  //    ie->value.choice.CriticalityDiagnostics.transactionID = 0L;
  //  }

  //  /* optional */
  //  /* F1AP_CriticalityDiagnostics_IE_List */
  //  if (0) {
  //    for (i=0;
  //         i<0;
  //         i++) {

  //        F1AP_CriticalityDiagnostics_IE_Item_t *criticalityDiagnostics_ie_item = (F1AP_CriticalityDiagnostics_IE_Item_t *)calloc(1, sizeof(F1AP_CriticalityDiagnostics_IE_Item_t));;
  //        criticalityDiagnostics_ie_item->iECriticality = F1AP_Criticality_reject;
  //        criticalityDiagnostics_ie_item->iE_ID         = 0L;
  //        criticalityDiagnostics_ie_item->typeOfError   = F1AP_TypeOfError_not_understood;

  //        ASN_SEQUENCE_ADD(&ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics->list,
  //                    criticalityDiagnostics_ie_item);
  //    }
  //  }
  //  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  //}

  /* encode */
  uint8_t  *buffer;
  uint32_t  len;
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release complete\n");
    return -1;
  }

  du_f1ap_itti_send_sctp_data_req(instance,
      f1ap_du_data->assoc_id,
      buffer,
      len,
      f1ap_du_data->default_sctp_stream_id);

  f1ap_remove_ue(&f1ap_du_inst[instance], cplt->rnti);
  return 0;
}


int DU_handle_UE_CONTEXT_MODIFICATION_REQUEST(instance_t       instance,
                                              uint32_t         assoc_id,
                                              uint32_t         stream,
                                              F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(F1AP_UEContextModificationResponse_t *UEContextModificationResponse) {
int DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(instance_t instance) {
  F1AP_F1AP_PDU_t                        pdu;
  F1AP_UEContextModificationResponse_t    *out;
  F1AP_UEContextModificationResponseIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (F1AP_SuccessfulOutcome_t *)calloc(1, sizeof(F1AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = F1AP_ProcedureCode_id_UEContextModification;
  pdu.choice.successfulOutcome->criticality   = F1AP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextModificationResponse;
  out = &pdu.choice.successfulOutcome->value.choice.UEContextModificationResponse;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c4. DUtoCURRCInformation */
  if (0) {
    ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCInformation;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DUtoCURRCInformation;

    OCTET_STRING_fromBuf(&ie->value.choice.DUtoCURRCInformation.cellGroupConfig, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));
    /* OPTIONAL */
    if (1) {
      ie->value.choice.DUtoCURRCInformation.measGapConfig = (F1AP_MeasGapConfig_t *)calloc(1, sizeof(F1AP_MeasGapConfig_t));
      OCTET_STRING_fromBuf( ie->value.choice.DUtoCURRCInformation.measGapConfig, "asdsa1d32sa1d31asd31as",
                           strlen("asdsa1d32sa1d31asd31as"));
      ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }
  }


  /* mandatory */
  /* c5. DRBs_SetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_SetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_SetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_SetupMod_ItemIEs_t *drbs_setupMod_item_ies;
    drbs_setupMod_item_ies = (F1AP_DRBs_SetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_SetupMod_ItemIEs_t));
    drbs_setupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_SetupMod_Item;
    drbs_setupMod_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_setupMod_item_ies->value.present = F1AP_DRBs_SetupMod_ItemIEs__value_PR_DRBs_SetupMod_Item;

    /* 10.1 DRBs_SetupMod_Item */
    F1AP_DRBs_SetupMod_Item_t drbs_setupMod_item;
    memset((void *)&drbs_setupMod_item, 0, sizeof(F1AP_DRBs_SetupMod_Item_t));

    /* dRBID */
    drbs_setupMod_item.dRBID = 30L;

    /* DLTunnels_SetupMod_List */
    int j = 0;
    int maxnoofDLUPTNLInformation = 1; // 2;
    for (j=0;
        j<maxnoofDLUPTNLInformation;
        j++) {
        /*  DLTunnels_ToBeSetup_Item */
        F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dLUPTNLInformation_ToBeSetup_Item;
        dLUPTNLInformation_ToBeSetup_Item = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_DLUPTNLInformation_ToBeSetup_Item_t));
        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

        OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                             strlen("1204"));

        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

        ASN_SEQUENCE_ADD(&drbs_setupMod_item.dLUPTNLInformation_ToBeSetup_List.list, dLUPTNLInformation_ToBeSetup_Item);
    }

    /* ADD */
    drbs_setupMod_item_ies->value.choice.DRBs_SetupMod_Item = drbs_setupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_SetupMod_List.list,
                   drbs_setupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c6. DRBs_Modified_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_Modified_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_Modified_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_Modified_ItemIEs_t *drbs_modified_item_ies;
    drbs_modified_item_ies = (F1AP_DRBs_Modified_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_Modified_ItemIEs_t));
    drbs_modified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_Modified_Item;
    drbs_modified_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_modified_item_ies->value.present = F1AP_DRBs_Modified_ItemIEs__value_PR_DRBs_Modified_Item;

    /* 13.1 SRBs_modified_Item */
    F1AP_DRBs_Modified_Item_t drbs_modified_item;
    memset((void *)&drbs_modified_item, 0, sizeof(F1AP_DRBs_Modified_Item_t));

    /* dRBID */
    drbs_modified_item.dRBID = 25L;

    /* ULTunnels_Modified_List */
    int maxnoofULTunnels = 1; // 2;
    int j = 0;
    for (j=0;
         j<maxnoofULTunnels;
         j++) {
      /*  DLTunnels_Modified_Item */
        F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dLUPTNLInformation_ToBeSetup_Item;
        dLUPTNLInformation_ToBeSetup_Item = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_DLUPTNLInformation_ToBeSetup_Item_t));
        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

        OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                              strlen("1204"));

        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

        ASN_SEQUENCE_ADD(&drbs_modified_item.dLUPTNLInformation_ToBeSetup_List.list, dLUPTNLInformation_ToBeSetup_Item);
    }

    /* ADD */
    drbs_modified_item_ies->value.choice.DRBs_Modified_Item = drbs_modified_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_Modified_List.list,
                    drbs_modified_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c7. SRBs_FailedToBeSetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_SRBs_FailedToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_FailedToBeSetupMod_ItemIEs_t *srbs_failedToBeSetupMod_item_ies;
    srbs_failedToBeSetupMod_item_ies = (F1AP_SRBs_FailedToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_FailedToBeSetupMod_ItemIEs_t));
    srbs_failedToBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetupMod_Item;
    srbs_failedToBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_failedToBeSetupMod_item_ies->value.present = F1AP_SRBs_FailedToBeSetupMod_ItemIEs__value_PR_SRBs_FailedToBeSetupMod_Item;

    /* 9.1 SRBs_FailedToBeSetupMod_Item */
    F1AP_SRBs_FailedToBeSetupMod_Item_t srbs_failedToBeSetupMod_item;
    memset((void *)&srbs_failedToBeSetupMod_item, 0, sizeof(F1AP_SRBs_FailedToBeSetupMod_Item_t));

    /* - sRBID */
    srbs_failedToBeSetupMod_item.sRBID = 50L;

    srbs_failedToBeSetupMod_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
    srbs_failedToBeSetupMod_item.cause->present = F1AP_Cause_PR_radioNetwork;
    srbs_failedToBeSetupMod_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

    /* ADD */
    srbs_failedToBeSetupMod_item_ies->value.choice.SRBs_FailedToBeSetupMod_Item = srbs_failedToBeSetupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_FailedToBeSetupMod_List.list,
                     srbs_failedToBeSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c8. DRBs_FailedToBeSetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_FailedToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_FailedToBeSetupMod_ItemIEs_t *drbs_failedToBeSetupMod_item_ies;
    drbs_failedToBeSetupMod_item_ies = (F1AP_DRBs_FailedToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_FailedToBeSetupMod_ItemIEs_t));
    drbs_failedToBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetupMod_Item;
    drbs_failedToBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_failedToBeSetupMod_item_ies->value.present = F1AP_DRBs_FailedToBeSetupMod_ItemIEs__value_PR_DRBs_FailedToBeSetupMod_Item;

    /* 10.1 DRBs_ToBeSetupMod_Item */
    F1AP_DRBs_FailedToBeSetupMod_Item_t drbs_failedToBeSetupMod_item;
    memset((void *)&drbs_failedToBeSetupMod_item, 0, sizeof(F1AP_DRBs_FailedToBeSetupMod_Item_t));

    /* dRBID */
    drbs_failedToBeSetupMod_item.dRBID = 30L;

    drbs_failedToBeSetupMod_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
    drbs_failedToBeSetupMod_item.cause->present = F1AP_Cause_PR_radioNetwork;
    drbs_failedToBeSetupMod_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

    /* ADD */
    drbs_failedToBeSetupMod_item_ies->value.choice.DRBs_FailedToBeSetupMod_Item = drbs_failedToBeSetupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_FailedToBeSetupMod_List.list,
                     drbs_failedToBeSetupMod_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c9. SCell_FailedtoSetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetupMod_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_SCell_FailedtoSetupMod_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_FailedtoSetupMod_ItemIEs_t *scell_failedtoSetupMod_item_ies;
     scell_failedtoSetupMod_item_ies = (F1AP_SCell_FailedtoSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_FailedtoSetupMod_ItemIEs_t));
     scell_failedtoSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetupMod_Item;
     scell_failedtoSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_failedtoSetupMod_item_ies->value.present = F1AP_SCell_FailedtoSetupMod_ItemIEs__value_PR_SCell_FailedtoSetupMod_Item;

     /* 8.1 SCell_ToBeSetup_Item */
     F1AP_SCell_FailedtoSetupMod_Item_t scell_failedtoSetupMod_item;
     memset((void *)&scell_failedtoSetupMod_item, 0, sizeof(F1AP_SCell_FailedtoSetupMod_Item_t));

     /* - sCell_ID */
     F1AP_NRCGI_t nRCGI;
     MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i],
                                        &nRCGI.pLMN_Identity);

     NR_CELL_ID_TO_BIT_STRING(f1ap_du_data->nr_cellid[0], &nRCGI.nRCellIdentity);

     scell_failedtoSetupMod_item.sCell_ID = nRCGI;

     scell_failedtoSetupMod_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     scell_failedtoSetupMod_item.cause->present = F1AP_Cause_PR_radioNetwork;
     scell_failedtoSetupMod_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

        /* ADD */
     scell_failedtoSetupMod_item_ies->value.choice.SCell_FailedtoSetupMod_Item = scell_failedtoSetupMod_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_FailedtoSetupMod_List.list,
                      scell_failedtoSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c10. DRBs_FailedToBeModified_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeModified_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_FailedToBeModified_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_FailedToBeModified_ItemIEs_t *drbs_failedToBeModified_item_ies;
    drbs_failedToBeModified_item_ies = (F1AP_DRBs_FailedToBeModified_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_FailedToBeModified_ItemIEs_t));
    drbs_failedToBeModified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeModified_Item;
    drbs_failedToBeModified_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_failedToBeModified_item_ies->value.present = F1AP_DRBs_FailedToBeModified_ItemIEs__value_PR_DRBs_FailedToBeModified_Item;

    /* 13.1 DRBs_FailedToBeModified_Item */
    F1AP_DRBs_FailedToBeModified_Item_t drbs_failedToBeModified_item;
    memset((void *)&drbs_failedToBeModified_item, 0, sizeof(F1AP_DRBs_FailedToBeModified_Item_t));

    /* dRBID */
    drbs_failedToBeModified_item.dRBID = 30L;

    drbs_failedToBeModified_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
    drbs_failedToBeModified_item.cause->present = F1AP_Cause_PR_radioNetwork;
    drbs_failedToBeModified_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

    /* ADD */
    drbs_failedToBeModified_item_ies->value.choice.DRBs_FailedToBeModified_Item = drbs_failedToBeModified_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_FailedToBeModified_List.list,
                     drbs_failedToBeModified_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c11. CriticalityDiagnostics */
  if (0) {
    ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_CriticalityDiagnostics;

    // dummy value
    /* optional */
    /* procedureCode */
    if (0) {
      ie->value.choice.CriticalityDiagnostics.procedureCode = (F1AP_ProcedureCode_t *)calloc(1, sizeof(F1AP_ProcedureCode_t));
      ie->value.choice.CriticalityDiagnostics.procedureCode = 0L;
    }

    /* optional */
    /* triggeringMessage */
    if (0) {
      ie->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)calloc(1, sizeof(F1AP_TriggeringMessage_t));
      ie->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)F1AP_TriggeringMessage_successful_outcome;
    }

    /* optional */
    /* procedureCriticality */
    if (0) {
      ie->value.choice.CriticalityDiagnostics.procedureCriticality = (F1AP_Criticality_t *)calloc(1, sizeof(F1AP_Criticality_t));
      ie->value.choice.CriticalityDiagnostics.procedureCriticality = F1AP_Criticality_reject;
    }

    /* optional */
    /* transactionID */
    if (0) {
      ie->value.choice.CriticalityDiagnostics.transactionID = (F1AP_TransactionID_t *)calloc(1, sizeof(F1AP_TransactionID_t));
      ie->value.choice.CriticalityDiagnostics.transactionID = 0L;
    }

    /* optional */
    /* F1AP_CriticalityDiagnostics_IE_List */
    if (0) {
      for (i=0;
           i<0;
           i++) {

          F1AP_CriticalityDiagnostics_IE_Item_t *criticalityDiagnostics_ie_item = (F1AP_CriticalityDiagnostics_IE_Item_t *)calloc(1, sizeof(F1AP_CriticalityDiagnostics_IE_Item_t));;
          criticalityDiagnostics_ie_item->iECriticality = F1AP_Criticality_reject;
          criticalityDiagnostics_ie_item->iE_ID         = 0L;
          criticalityDiagnostics_ie_item->typeOfError   = F1AP_TypeOfError_not_understood;

          ASN_SEQUENCE_ADD(&ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics->list,
                      criticalityDiagnostics_ie_item);
      }
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup request\n");
    return -1;
  }

  //du_f1ap_itti_send_sctp_data_req(instance, f1ap_setup_req->assoc_id, buffer, len, 0);
  return 0;

}

int DU_send_UE_CONTEXT_MODIFICATION_FAILURE(instance_t instance) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_send_UE_CONTEXT_MODIFICATION_REQUIRED(instance_t instance) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_handle_UE_CONTEXT_MODIFICATION_CONFIRM(instance_t       instance,
                                              uint32_t         assoc_id,
                                              uint32_t         stream,
                                              F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}
