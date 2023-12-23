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
#include "f1ap_itti_messaging.h"
#include "f1ap_du_ue_context_management.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"

#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"

bool DURecvCb(protocol_ctxt_t *ctxt_pP,
              const srb_flag_t srb_flagP,
              const rb_id_t rb_idP,
              const mui_t muiP,
              const confirm_t confirmP,
              const sdu_size_t sdu_buffer_sizeP,
              unsigned char *const sdu_buffer_pP,
              const pdcp_transmission_mode_t modeP,
              const uint32_t *sourceL2Id,
              const uint32_t *destinationL2Id)
{
  // The buffer comes from the stack in gtp-u thread, we have a make a separate buffer to enqueue in a inter-thread message queue
  uint8_t *sdu = malloc16(sdu_buffer_sizeP);
  memcpy(sdu, sdu_buffer_pP, sdu_buffer_sizeP);
  du_rlc_data_req(ctxt_pP, srb_flagP, false, rb_idP, muiP, confirmP, sdu_buffer_sizeP, sdu);
  return true;
}

int DU_handle_UE_CONTEXT_SETUP_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_UEContextSetupRequest_t    *container;
  int i;
  DevAssert(pdu);
  f1ap_ue_context_setup_t ue_context_setup = {0};
  f1ap_ue_context_setup_t *f1ap_ue_context_setup_req = &ue_context_setup;
  container = &pdu->choice.initiatingMessage->value.choice.UEContextSetupRequest;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_UEContextSetupRequestIEs_t *ieCU;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieCU, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  f1ap_ue_context_setup_req->gNB_CU_ue_id = ieCU->value.choice.GNB_CU_UE_F1AP_ID;
  /* optional */
  /* GNB_DU_UE_F1AP_ID */
  F1AP_UEContextSetupRequestIEs_t *ieDU_UE;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieDU_UE, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  f1ap_ue_context_setup_req->gNB_DU_ue_id = ieDU_UE->value.choice.GNB_DU_UE_F1AP_ID;

  /* SpCell_ID */
  F1AP_UEContextSetupRequestIEs_t *ieNet;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieNet, container,
                             F1AP_ProtocolIE_ID_id_SpCell_ID, true);
  PLMNID_TO_MCC_MNC(&ieNet->value.choice.NRCGI.pLMN_Identity,
                    f1ap_ue_context_setup_req->plmn.mcc,
                    f1ap_ue_context_setup_req->plmn.mnc,
                    f1ap_ue_context_setup_req->plmn.mnc_digit_length);
  BIT_STRING_TO_NR_CELL_IDENTITY(&ieNet->value.choice.NRCGI.nRCellIdentity, f1ap_ue_context_setup_req->nr_cellid);
  /* ServCellIndex */
  F1AP_UEContextSetupRequestIEs_t *ieCell;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieCell, container,
                             F1AP_ProtocolIE_ID_id_ServCellIndex, true);
  f1ap_ue_context_setup_req->servCellIndex = ieCell->value.choice.ServCellIndex;
  /* optional */
  /* CellULConfigured */
  F1AP_UEContextSetupRequestIEs_t *ieULCell;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieULCell, container,
                             F1AP_ProtocolIE_ID_id_SpCellULConfigured, false);  // SpCellULConfigured

  if (ieULCell) {
    /* correct here */
    f1ap_ue_context_setup_req->cellULConfigured = malloc(sizeof(uint32_t));

    if (f1ap_ue_context_setup_req->cellULConfigured)
      *f1ap_ue_context_setup_req->cellULConfigured = ieULCell->value.choice.CellULConfigured;
  } else {
    f1ap_ue_context_setup_req->cellULConfigured = NULL;
  }

  F1AP_UEContextSetupRequestIEs_t *ieCuRrcInfo;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieCuRrcInfo, container,
      F1AP_ProtocolIE_ID_id_CUtoDURRCInformation, false);
  if(ieCuRrcInfo!=NULL){
    f1ap_ue_context_setup_req->cu_to_du_rrc_information = (cu_to_du_rrc_information_t *)calloc(1,sizeof(cu_to_du_rrc_information_t));
    if(ieCuRrcInfo->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList!=NULL){
      f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList = (uint8_t *)calloc(1,ieCuRrcInfo->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList->size);
      memcpy(f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList, ieCuRrcInfo->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList->buf, ieCuRrcInfo->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList->size);
      f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList_length = ieCuRrcInfo->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList->size;
      LOG_D(F1AP, "Size f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList_length: %d \n", f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList_length);
    }
  }

  /* DRB */
  F1AP_UEContextSetupRequestIEs_t *ieDrb;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieDrb, container,
                             F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_List, false);

  if(ieDrb!=NULL) {
    f1ap_ue_context_setup_req->drbs_to_be_setup_length = ieDrb->value.choice.DRBs_ToBeSetup_List.list.count;
    f1ap_ue_context_setup_req->drbs_to_be_setup = calloc(f1ap_ue_context_setup_req->drbs_to_be_setup_length,
        sizeof(f1ap_drb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_req->drbs_to_be_setup,
                "could not allocate memory for f1ap_ue_context_setup_req->drbs_to_be_setup\n");

    for (i = 0; i < f1ap_ue_context_setup_req->drbs_to_be_setup_length; ++i) {
      f1ap_drb_to_be_setup_t *drb_p = &f1ap_ue_context_setup_req->drbs_to_be_setup[i];
      F1AP_DRBs_ToBeSetup_Item_t *drbs_tobesetup_item_p =
        &((F1AP_DRBs_ToBeSetup_ItemIEs_t *)ieDrb->value.choice.DRBs_ToBeSetup_List.list.array[i])->value.choice.DRBs_ToBeSetup_Item;
      drb_p->drb_id = drbs_tobesetup_item_p->dRBID;
      /* TODO in the following, assume only one UP UL TNL is present.
       * this matches/assumes OAI CU implementation, can be up to 2! */
      drb_p->up_ul_tnl_length = 1;
      AssertFatal(drbs_tobesetup_item_p->uLUPTNLInformation_ToBeSetup_List.list.count > 0,
                  "no UL UP TNL Information in DRBs to be Setup list\n");
      F1AP_ULUPTNLInformation_ToBeSetup_Item_t *ul_up_tnl_info_p = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)drbs_tobesetup_item_p->uLUPTNLInformation_ToBeSetup_List.list.array[0];
      F1AP_GTPTunnel_t *ul_up_tnl0 = ul_up_tnl_info_p->uLUPTNLInformation.choice.gTPTunnel;
      BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&ul_up_tnl0->transportLayerAddress, drb_p->up_ul_tnl[0].tl_address);
      OCTET_STRING_TO_UINT32(&ul_up_tnl0->gTP_TEID, drb_p->up_ul_tnl[0].teid);
      // 3GPP assumes GTP-U is on port 2152, but OAI is configurable
      drb_p->up_ul_tnl[0].port = getCxt(instance)->net_config.CUport;

      switch (drbs_tobesetup_item_p->rLCMode) {
        case F1AP_RLCMode_rlc_am:
          drb_p->rlc_mode = RLC_MODE_AM;
          break;

        default:
          drb_p->rlc_mode = RLC_MODE_TM;
          break;
      }
    }
  }

  /* SRB */
  F1AP_UEContextSetupRequestIEs_t *ieSrb;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieSrb, container,
                             F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_List, false);

  if(ieSrb != NULL) {
    f1ap_ue_context_setup_req->srbs_to_be_setup_length = ieSrb->value.choice.SRBs_ToBeSetup_List.list.count;
    f1ap_ue_context_setup_req->srbs_to_be_setup = calloc(f1ap_ue_context_setup_req->srbs_to_be_setup_length,
        sizeof(f1ap_srb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_req->srbs_to_be_setup,
                "could not allocate memory for f1ap_ue_context_setup_req->srbs_to_be_setup\n");

    for (i = 0; i < f1ap_ue_context_setup_req->srbs_to_be_setup_length; ++i) {
      f1ap_srb_to_be_setup_t *srb_p = &f1ap_ue_context_setup_req->srbs_to_be_setup[i];
      F1AP_SRBs_ToBeSetup_Item_t *srbs_tobesetup_item_p;
      srbs_tobesetup_item_p = &((F1AP_SRBs_ToBeSetup_ItemIEs_t *)ieSrb->value.choice.SRBs_ToBeSetup_List.list.array[i])->value.choice.SRBs_ToBeSetup_Item;
      srb_p->srb_id = srbs_tobesetup_item_p->sRBID;
    }
  }

  /* RRCContainer */
  F1AP_UEContextSetupRequestIEs_t *ieRRC;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupRequestIEs_t, ieRRC, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, false);

  if (ieRRC) {
    /* correct here */
    if ( ieRRC->value.choice.RRCContainer.size )  {
      f1ap_ue_context_setup_req->rrc_container = malloc(ieRRC->value.choice.RRCContainer.size);
      memcpy(f1ap_ue_context_setup_req->rrc_container,
             ieRRC->value.choice.RRCContainer.buf, ieRRC->value.choice.RRCContainer.size);
      f1ap_ue_context_setup_req->rrc_container_length = ieRRC->value.choice.RRCContainer.size;
      // AssertFatal(0, "check configuration, send to appropriate handler\n");
    } else {
      LOG_E(F1AP, " RRCContainer in UEContextSetupRequestIEs size id 0\n");
    }
  } else {
    LOG_W(F1AP, "can't find RRCContainer in UEContextSetupRequestIEs by id %ld \n", F1AP_ProtocolIE_ID_id_RRCContainer);
  }

  ue_context_setup_request(f1ap_ue_context_setup_req);
  return 0;
}

int DU_send_UE_CONTEXT_SETUP_RESPONSE(sctp_assoc_t assoc_id, f1ap_ue_context_setup_t *resp)
{
  F1AP_F1AP_PDU_t                  pdu= {0};
  F1AP_UEContextSetupResponse_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextSetupResponse;
  out = &tmp->value.choice.UEContextSetupResponse;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = resp->gNB_CU_ue_id;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = resp->gNB_DU_ue_id;

  /* mandatory */
  /* c3. DUtoCURRCInformation */
  if(resp->du_to_cu_rrc_information){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie3);
    ie3->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCInformation;
    ie3->criticality                    = F1AP_Criticality_reject;
    ie3->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DUtoCURRCInformation;
    if(resp->du_to_cu_rrc_information->cellGroupConfig!=NULL){
      /* cellGroupConfig */
      OCTET_STRING_fromBuf(&ie3->value.choice.DUtoCURRCInformation.cellGroupConfig, (const char *)resp->du_to_cu_rrc_information->cellGroupConfig,
        resp->du_to_cu_rrc_information->cellGroupConfig_length);
    }

    /* OPTIONAL */
    /* measGapConfig */
    if (resp->du_to_cu_rrc_information->measGapConfig!=NULL) {
      OCTET_STRING_fromBuf(ie3->value.choice.DUtoCURRCInformation.measGapConfig, (const char *)resp->du_to_cu_rrc_information->measGapConfig,
        resp->du_to_cu_rrc_information->measGapConfig_length);
    }

    /* OPTIONAL */
    /* requestedP_MaxFR1 */
    if (resp->du_to_cu_rrc_information->requestedP_MaxFR1!=NULL) {
      OCTET_STRING_fromBuf(ie3->value.choice.DUtoCURRCInformation.requestedP_MaxFR1, (const char *)resp->du_to_cu_rrc_information->requestedP_MaxFR1,
          resp->du_to_cu_rrc_information->requestedP_MaxFR1_length);
    }
  }

  /* optional */
  /* c4. C_RNTI */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie4);
    ie4->id                             = F1AP_ProtocolIE_ID_id_C_RNTI;
    ie4->criticality                    = F1AP_Criticality_ignore;
    ie4->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_C_RNTI;
    //C_RNTI_TO_BIT_STRING(rntiP, &ie->value.choice.C_RNTI);
    ie4->value.choice.C_RNTI=0;
    AssertFatal(false, "not implemented\n");
    LOG_E(F1AP,"RNTI to code!\n");
  }

  /* optional */
  /* c5. ResourceCoordinationTransferContainer */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie5->criticality                    = F1AP_Criticality_ignore;
    ie5->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie5->value.choice.ResourceCoordinationTransferContainer, "asdsa",
                         strlen("asdsa"));
  }

  /* optional */
  /* c6. FullConfiguration */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie6);
    ie6->id                             = F1AP_ProtocolIE_ID_id_FullConfiguration;
    ie6->criticality                    = F1AP_Criticality_ignore;
    ie6->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_FullConfiguration;
    ie6->value.choice.FullConfiguration = F1AP_FullConfiguration_full;   //enum
  }

  /* mandatory */
  /* c7. DRBs_Setup_List */
  if(resp->drbs_to_be_setup_length > 0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie7);
    ie7->id                             = F1AP_ProtocolIE_ID_id_DRBs_Setup_List;
    ie7->criticality                    = F1AP_Criticality_ignore;
    ie7->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DRBs_Setup_List;
    for (int i=0;  i< resp->drbs_to_be_setup_length; i++) {
      //
      asn1cSequenceAdd(ie7->value.choice.DRBs_Setup_List.list,
          F1AP_DRBs_Setup_ItemIEs_t, drbs_setup_item_ies);
      drbs_setup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_Setup_Item;
      drbs_setup_item_ies->criticality   = F1AP_Criticality_ignore;
      drbs_setup_item_ies->value.present = F1AP_DRBs_Setup_ItemIEs__value_PR_DRBs_Setup_Item;
      /* 7.1 DRBs_Setup_Item */
      /* ADD */
      F1AP_DRBs_Setup_Item_t *drbs_setup_item=&drbs_setup_item_ies->value.choice.DRBs_Setup_Item;
      /* dRBID */
      drbs_setup_item->dRBID = resp->drbs_to_be_setup[i].drb_id;

      /* OPTIONAL */
      /* lCID */
      //drbs_setup_item.lCID = (F1AP_LCID_t *)calloc(1, sizeof(F1AP_LCID_t));
      //drbs_setup_item.lCID = 1L;

      for (int j=0;  j<resp->drbs_to_be_setup[i].up_dl_tnl_length; j++) {
        /* ADD */
        asn1cSequenceAdd(drbs_setup_item->dLUPTNLInformation_ToBeSetup_List.list,
                       F1AP_DLUPTNLInformation_ToBeSetup_Item_t, dLUPTNLInformation_ToBeSetup_Item);
        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        /* gTPTunnel */
        asn1cCalloc(dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel,gTPTunnel);
        /* transportLayerAddress */
        struct sockaddr_in addr= {0};
        inet_pton(AF_INET, getCxt(0)->net_config.DU_f1_ip_address.ipv4_address, &addr.sin_addr.s_addr);
        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(addr.sin_addr.s_addr,
            &gTPTunnel->transportLayerAddress);
        /* gTP_TEID */
        INT32_TO_OCTET_STRING(resp->drbs_to_be_setup[i].up_dl_tnl[j].teid, &gTPTunnel->gTP_TEID);
      } // for j
    } // for i

  }

  /* mandatory */
  /* c8. SRBs_FailedToBeSetup_List */
  if(0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie8);
    ie8->id                             = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_List;
    ie8->criticality                    = F1AP_Criticality_ignore;
    ie8->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_SRBs_FailedToBeSetup_List;

    for (int i=0;  i<1; i++) {
      //
      asn1cSequenceAdd(ie8->value.choice.SRBs_FailedToBeSetup_List.list,
        F1AP_SRBs_FailedToBeSetup_ItemIEs_t, srbs_failedToBeSetup_item_ies);
      srbs_failedToBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_Item;
      srbs_failedToBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
      srbs_failedToBeSetup_item_ies->value.present = F1AP_SRBs_FailedToBeSetup_ItemIEs__value_PR_SRBs_FailedToBeSetup_Item;
      /* 8.1 SRBs_Setup_Item */
      F1AP_SRBs_FailedToBeSetup_Item_t *srbs_failedToBeSetup_item=
        &srbs_failedToBeSetup_item_ies->value.choice.SRBs_FailedToBeSetup_Item;
      /* sRBID */
      srbs_failedToBeSetup_item->sRBID = 13L;
      /* cause */
      asn1cCalloc(srbs_failedToBeSetup_item->cause, tmp);
      // dummy value
      tmp->present = F1AP_Cause_PR_radioNetwork;

      switch(tmp->present) {
        case F1AP_Cause_PR_radioNetwork:
          tmp->choice.radioNetwork = F1AP_CauseRadioNetwork_unspecified;
          break;

        case F1AP_Cause_PR_transport:
          tmp->choice.transport = F1AP_CauseTransport_unspecified;
          break;

        case F1AP_Cause_PR_protocol:
          tmp->choice.protocol = F1AP_CauseProtocol_unspecified;
          break;

        case F1AP_Cause_PR_misc:
          tmp->choice.misc = F1AP_CauseMisc_unspecified;
          break;

        case F1AP_Cause_PR_NOTHING:
        default:
          break;
      } // switch
    } // for i
  }

  /*  */
  /* c9. DRBs_FailedToBeSetup_List */
  if(0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie9);
    ie9->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_List;
    ie9->criticality                    = F1AP_Criticality_ignore;
    ie9->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DRBs_FailedToBeSetup_List;

    for (int i=0;  i<1; i++) {
      asn1cSequenceAdd(ie9->value.choice.DRBs_FailedToBeSetup_List.list,
                       F1AP_DRBs_FailedToBeSetup_ItemIEs_t, drbs_failedToBeSetup_item_ies);
      drbs_failedToBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_Item;
      drbs_failedToBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
      drbs_failedToBeSetup_item_ies->value.present = F1AP_DRBs_FailedToBeSetup_ItemIEs__value_PR_DRBs_FailedToBeSetup_Item;
      /* 9.1 DRBs_Setup_Item */
      /* ADD */
      F1AP_DRBs_FailedToBeSetup_Item_t *drbs_failedToBeSetup_item=
        &drbs_failedToBeSetup_item_ies->value.choice.DRBs_FailedToBeSetup_Item;
      /* dRBID */
      drbs_failedToBeSetup_item->dRBID = 14;
      /* cause */
      asn1cCalloc(drbs_failedToBeSetup_item->cause,tmp);
      // dummy value
      tmp->present = F1AP_Cause_PR_radioNetwork;

      switch(tmp->present) {
        case F1AP_Cause_PR_radioNetwork:
          tmp->choice.radioNetwork = F1AP_CauseRadioNetwork_unspecified;
          break;

        case F1AP_Cause_PR_transport:
          tmp->choice.transport = F1AP_CauseTransport_unspecified;
          break;

        case F1AP_Cause_PR_protocol:
          tmp->choice.protocol = F1AP_CauseProtocol_unspecified;
          break;

        case F1AP_Cause_PR_misc:
          tmp->choice.misc = F1AP_CauseMisc_unspecified;
          break;

        case F1AP_Cause_PR_NOTHING:
        default:
          break;
      } // switch
    } // for i
  }

  // /*  */
  /* c10. SCell_FailedtoSetup_List */
  if(0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie10);
    ie10->id                             = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_List;
    ie10->criticality                    = F1AP_Criticality_ignore;
    ie10->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_SCell_FailedtoSetup_List;

    for (int i=0;   i<1; i++) {
      asn1cSequenceAdd(ie10->value.choice.SCell_FailedtoSetup_List.list,
                       F1AP_SCell_FailedtoSetup_ItemIEs_t, sCell_FailedtoSetup_item_ies);
      sCell_FailedtoSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_Item;
      sCell_FailedtoSetup_item_ies->criticality   = F1AP_Criticality_ignore;
      sCell_FailedtoSetup_item_ies->value.present = F1AP_SCell_FailedtoSetup_ItemIEs__value_PR_SCell_FailedtoSetup_Item;
      /* 10.1 DRBs_Setup_Item */
      F1AP_SCell_FailedtoSetup_Item_t *sCell_FailedtoSetup_item=
        &sCell_FailedtoSetup_item_ies->value.choice.SCell_FailedtoSetup_Item;
      /* sCell_ID */
      AssertFatal(false, "handle correct CellID\n");
      f1ap_served_cell_info_t cellID = {0};
      addnRCGI(sCell_FailedtoSetup_item->sCell_ID, &cellID);
      /* cause */
      asn1cCalloc(sCell_FailedtoSetup_item->cause, tmp);
      // dummy value
      tmp->present = F1AP_Cause_PR_radioNetwork;

      switch(tmp->present) {
        case F1AP_Cause_PR_radioNetwork:
          tmp->choice.radioNetwork = F1AP_CauseRadioNetwork_unspecified;
          break;

        case F1AP_Cause_PR_transport:
          tmp->choice.transport = F1AP_CauseTransport_unspecified;
          break;

        case F1AP_Cause_PR_protocol:
          tmp->choice.protocol = F1AP_CauseProtocol_unspecified;
          break;

        case F1AP_Cause_PR_misc:
          tmp->choice.misc = F1AP_CauseMisc_unspecified;
          break;

        case F1AP_Cause_PR_NOTHING:
        default:
          break;
      } // switch
    } // for i
  }

  /* mandatory */
  /* c11. SRBs_Setup_List */
  if(resp->srbs_to_be_setup_length > 0){
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie11);
  ie11->id                             = F1AP_ProtocolIE_ID_id_SRBs_Setup_List;
  ie11->criticality                    = F1AP_Criticality_ignore;
  ie11->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_SRBs_Setup_List;

  for (int i=0;  i< resp->srbs_to_be_setup_length; i++) {//
    asn1cSequenceAdd(ie11->value.choice.SRBs_Setup_List.list,
                     F1AP_SRBs_Setup_ItemIEs_t, srbs_setup_item_ies);
    srbs_setup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_Setup_Item;
    srbs_setup_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_setup_item_ies->value.present = F1AP_SRBs_Setup_ItemIEs__value_PR_SRBs_Setup_Item;
    /* 11.1 SRBs_Setup_Item */
    /* ADD */
    F1AP_SRBs_Setup_Item_t *srbs_setup_item=&srbs_setup_item_ies->value.choice.SRBs_Setup_Item;
    /* sRBID */
    srbs_setup_item->sRBID = resp->srbs_to_be_setup[i].srb_id;
  }
  }

  /* Optional */
  /* c12. InactivityMonitoringResponse */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie11);
    ie11->id                                        = F1AP_ProtocolIE_ID_id_InactivityMonitoringResponse;
    ie11->criticality                               = F1AP_Criticality_ignore;
    ie11->value.present                             = F1AP_UEContextSetupResponseIEs__value_PR_InactivityMonitoringResponse;
    ie11->value.choice.InactivityMonitoringResponse = F1AP_InactivityMonitoringResponse_not_supported;
  }

  /* Optional */
  /* c13. CriticalityDiagnostics */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupResponseIEs_t, ie12);
    ie12->id                             = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie12->criticality                    = F1AP_Criticality_ignore;
    ie12->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_CriticalityDiagnostics;
    asn1cCallocOne(ie12->value.choice.CriticalityDiagnostics.procedureCode,
                   F1AP_ProcedureCode_id_UEContextSetup);
    asn1cCallocOne(ie12->value.choice.CriticalityDiagnostics.triggeringMessage,
                   F1AP_TriggeringMessage_initiating_message);
    asn1cCallocOne(ie12->value.choice.CriticalityDiagnostics.procedureCriticality,
                   F1AP_Criticality_reject);
    asn1cCallocOne(ie12->value.choice.CriticalityDiagnostics.transactionID,
                   0);
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE CONTEXT SETUP RESPONSE\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_send_UE_CONTEXT_SETUP_FAILURE(sctp_assoc_t assoc_id)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_send_UE_CONTEXT_RELEASE_REQUEST(sctp_assoc_t assoc_id, f1ap_ue_context_release_req_t *req)
{
  F1AP_F1AP_PDU_t                   pdu = {0};
  F1AP_UEContextReleaseRequest_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextReleaseRequest;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextReleaseRequest;
  out = &tmp->value.choice.UEContextReleaseRequest;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseRequestIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextReleaseRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = req->gNB_CU_ue_id;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseRequestIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextReleaseRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = req->gNB_DU_ue_id;
  /* mandatory */
  /* c3. Cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseRequestIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_Cause;
  ie3->criticality                    = F1AP_Criticality_ignore;
  ie3->value.present                  = F1AP_UEContextReleaseRequestIEs__value_PR_Cause;

  switch (req->cause) {
    case F1AP_CAUSE_RADIO_NETWORK:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_radioNetwork;
      ie3->value.choice.Cause.choice.radioNetwork = req->cause_value;
      break;

    case F1AP_CAUSE_TRANSPORT:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_transport;
      ie3->value.choice.Cause.choice.transport = req->cause_value;
      break;

    case F1AP_CAUSE_PROTOCOL:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_protocol;
      ie3->value.choice.Cause.choice.protocol = req->cause_value;
      break;

    case F1AP_CAUSE_MISC:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_misc;
      ie3->value.choice.Cause.choice.misc = req->cause_value;
      break;

    case F1AP_CAUSE_NOTHING:
    default:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_NOTHING;
      break;
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release request\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_handle_UE_CONTEXT_RELEASE_COMMAND(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_UEContextReleaseCommand_t *container;
  F1AP_UEContextReleaseCommandIEs_t *ie;

  f1ap_ue_context_release_req_t ue_context_release = {0};
  f1ap_ue_context_release_req_t *f1ap_ue_context_release_cmd = &ue_context_release;

  DevAssert(pdu);
  container = &pdu->choice.initiatingMessage->value.choice.UEContextReleaseCommand;
  // GNB_CU_UE_F1AP_ID
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  f1ap_ue_context_release_cmd->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  // GNB_DU_UE_F1AP_ID
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  f1ap_ue_context_release_cmd->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  // We don't need the Cause
  // Optional RRC Container: if present, send to UE
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, false);

  if (ie != NULL) {
    f1ap_ue_context_release_cmd->rrc_container = malloc(ie->value.choice.RRCContainer.size);
    AssertFatal(f1ap_ue_context_release_cmd->rrc_container != NULL, "out of memory\n");
    memcpy(f1ap_ue_context_release_cmd->rrc_container, ie->value.choice.RRCContainer.buf, ie->value.choice.RRCContainer.size);
    f1ap_ue_context_release_cmd->rrc_container_length = ie->value.choice.RRCContainer.size;

    // conditionally have SRB ID if there is RRC container
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container, F1AP_ProtocolIE_ID_id_SRBID, true);
    f1ap_ue_context_release_cmd->srb_id = ie->value.choice.SRBID;
  }

  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCommandIEs_t, ie, container,
      F1AP_ProtocolIE_ID_id_Cause, true);
  switch (ie->value.choice.Cause.present){
  case  F1AP_Cause_PR_radioNetwork:
    f1ap_ue_context_release_cmd->cause = F1AP_CAUSE_RADIO_NETWORK;
    break;
  case F1AP_Cause_PR_transport:
    f1ap_ue_context_release_cmd->cause = F1AP_CAUSE_TRANSPORT;
    break;
  case F1AP_Cause_PR_protocol:
    f1ap_ue_context_release_cmd->cause = F1AP_CAUSE_PROTOCOL;
    break;
  case F1AP_Cause_PR_misc:
    f1ap_ue_context_release_cmd->cause = F1AP_CAUSE_MISC;
    break;
  default:
    LOG_W(F1AP, "Unknown cause for UE context release command\n");
    break;

  }

  ue_context_release_command(f1ap_ue_context_release_cmd);
  return 0;
}

int DU_send_UE_CONTEXT_RELEASE_COMPLETE(sctp_assoc_t assoc_id, f1ap_ue_context_release_complete_t *complete)
{
  F1AP_F1AP_PDU_t                     pdu= {0};
  F1AP_UEContextReleaseComplete_t    *out;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextRelease;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextReleaseComplete;
  out = &tmp->value.choice.UEContextReleaseComplete;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCompleteIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextReleaseCompleteIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = complete->gNB_CU_ue_id;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list,F1AP_UEContextReleaseCompleteIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextReleaseCompleteIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = complete->gNB_DU_ue_id;
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
  //        asn1cSeqAdd(&ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics->list,
  //                    criticalityDiagnostics_ie_item);
  //    }
  //  }
  //  asn1cSeqAdd(&out->protocolIEs.list, ie);
  //}
  /* encode */
  uint8_t  *buffer;
  uint32_t  len;

  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release complete\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

static instance_t du_create_gtpu_instance_to_cu(char *CUaddr, uint16_t CUport, char *DUaddr, uint16_t DUport)
{
  openAddr_t tmp = {0};
  strncpy(tmp.originHost, DUaddr, sizeof(tmp.originHost)-1);
  strncpy(tmp.destinationHost, CUaddr, sizeof(tmp.destinationHost)-1);
  sprintf(tmp.originService, "%d", DUport);
  sprintf(tmp.destinationService, "%d", CUport);
  return gtpv1Init(tmp);
}

int DU_handle_UE_CONTEXT_MODIFICATION_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_UEContextModificationRequest_t    *container;
  int i;
  f1ap_ue_context_modif_req_t ue_context_modification = {0};
  f1ap_ue_context_modif_req_t *f1ap_ue_context_modification_req = &ue_context_modification;
  container = &pdu->choice.initiatingMessage->value.choice.UEContextModificationRequest;

  /* mandatory */
  /* GNB_CU_UE_F1AP_ID */
  F1AP_UEContextModificationRequestIEs_t *ieCU;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t, ieCU, container,
      F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  f1ap_ue_context_modification_req->gNB_CU_ue_id = ieCU->value.choice.GNB_CU_UE_F1AP_ID;

  /* mandatory */
  /* GNB_DU_UE_F1AP_ID */
  F1AP_UEContextModificationRequestIEs_t *ieDU_UE;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t, ieDU_UE, container,
      F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  f1ap_ue_context_modification_req->gNB_DU_ue_id = ieDU_UE->value.choice.GNB_DU_UE_F1AP_ID;

  /* SRB */
  F1AP_UEContextModificationRequestIEs_t *ieSrb;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t, ieSrb, container,
      F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_List, false);

  if(ieSrb != NULL) {
    f1ap_ue_context_modification_req->srbs_to_be_setup_length = ieSrb->value.choice.SRBs_ToBeSetupMod_List.list.count;
    f1ap_ue_context_modification_req->srbs_to_be_setup = calloc(f1ap_ue_context_modification_req->srbs_to_be_setup_length,
        sizeof(f1ap_srb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_modification_req->srbs_to_be_setup,
        "could not allocate memory for f1ap_ue_context_setup_req->srbs_to_be_setup\n");

    for (i = 0; i < f1ap_ue_context_modification_req->srbs_to_be_setup_length; ++i) {
      f1ap_srb_to_be_setup_t *srb_p = &f1ap_ue_context_modification_req->srbs_to_be_setup[i];
      F1AP_SRBs_ToBeSetupMod_Item_t *srbs_tobesetupmod_item_p;
      srbs_tobesetupmod_item_p = &((F1AP_SRBs_ToBeSetupMod_ItemIEs_t *)ieSrb->value.choice.SRBs_ToBeSetupMod_List.list.array[i])->value.choice.SRBs_ToBeSetupMod_Item;
      srb_p->srb_id = srbs_tobesetupmod_item_p->sRBID;
    }
  }

  /* DRB */
  F1AP_UEContextModificationRequestIEs_t *ieDrb;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t, ieDrb, container,
                   F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_List, false);

  if(ieDrb!=NULL) {
    f1ap_ue_context_modification_req->drbs_to_be_setup_length = ieDrb->value.choice.DRBs_ToBeSetupMod_List.list.count;
    f1ap_ue_context_modification_req->drbs_to_be_setup = calloc(f1ap_ue_context_modification_req->drbs_to_be_setup_length,
                               sizeof(f1ap_drb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_modification_req->drbs_to_be_setup,
          "could not allocate memory for f1ap_ue_context_setup_req->drbs_to_be_setup\n");

    for (i = 0; i < f1ap_ue_context_modification_req->drbs_to_be_setup_length; ++i) {
      f1ap_drb_to_be_setup_t *drb_p = &f1ap_ue_context_modification_req->drbs_to_be_setup[i];
      F1AP_DRBs_ToBeSetupMod_Item_t *drbs_tobesetupmod_item_p =
          &((F1AP_DRBs_ToBeSetupMod_ItemIEs_t *)ieDrb->value.choice.DRBs_ToBeSetupMod_List.list.array[i])->value.choice.DRBs_ToBeSetupMod_Item;
      drb_p->drb_id = drbs_tobesetupmod_item_p->dRBID;
      /* TODO in the following, assume only one UP UL TNL is present.
       * this matches/assumes OAI CU implementation, can be up to 2! */
      drb_p->up_ul_tnl_length = 1;
      AssertFatal(drbs_tobesetupmod_item_p->uLUPTNLInformation_ToBeSetup_List.list.count > 0,
          "no UL UP TNL Information in DRBs to be Setup list\n");
      F1AP_ULUPTNLInformation_ToBeSetup_Item_t *ul_up_tnl_info_p = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)drbs_tobesetupmod_item_p->uLUPTNLInformation_ToBeSetup_List.list.array[0];
      F1AP_GTPTunnel_t *ul_up_tnl0 = ul_up_tnl_info_p->uLUPTNLInformation.choice.gTPTunnel;
      BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&ul_up_tnl0->transportLayerAddress, drb_p->up_ul_tnl[0].tl_address);
      OCTET_STRING_TO_UINT32(&ul_up_tnl0->gTP_TEID, drb_p->up_ul_tnl[0].teid);
       // 3GPP assumes GTP-U is on port 2152, but OAI is configurable
      drb_p->up_ul_tnl[0].port = getCxt(instance)->net_config.CUport;

      extern instance_t DUuniqInstance;
      if (DUuniqInstance == 0) {
        char gtp_tunnel_ip_address[32];
        snprintf(gtp_tunnel_ip_address,
                 sizeof(gtp_tunnel_ip_address),
                 "%d.%d.%d.%d",
                 drb_p->up_ul_tnl[0].tl_address & 0xff,
                 (drb_p->up_ul_tnl[0].tl_address >> 8) & 0xff,
                 (drb_p->up_ul_tnl[0].tl_address >> 16) & 0xff,
                 (drb_p->up_ul_tnl[0].tl_address >> 24) & 0xff);
        getCxt(instance)->gtpInst = du_create_gtpu_instance_to_cu(gtp_tunnel_ip_address,
                                                                  getCxt(instance)->net_config.CUport,
                                                                  getCxt(instance)->net_config.DU_f1_ip_address.ipv4_address,
                                                                  getCxt(instance)->net_config.DUport);
        AssertFatal(getCxt(instance)->gtpInst > 0, "Failed to create CU F1-U UDP listener");
        // Fixme: fully inconsistent instances management
        // dirty global var is a bad fix
        extern instance_t legacyInstanceMapping;
        legacyInstanceMapping = DUuniqInstance = getCxt(instance)->gtpInst;
      }

      switch (drbs_tobesetupmod_item_p->rLCMode) {
      case F1AP_RLCMode_rlc_am:
        drb_p->rlc_mode = RLC_MODE_AM;
        break;

      default:
        drb_p->rlc_mode = RLC_MODE_TM;
        break;
      }

      if (drbs_tobesetupmod_item_p->qoSInformation.present == F1AP_QoSInformation_PR_eUTRANQoS) {
        AssertFatal(false, "Decode of eUTRANQoS is not implemented yet");
      } // EUTRAN QoS Information
      else {
        /* 12.1.2 DRB_Information */
        if (drbs_tobesetupmod_item_p->qoSInformation.present == F1AP_QoSInformation_PR_choice_extension) {
          F1AP_QoSInformation_ExtIEs_t *ie =
              (F1AP_QoSInformation_ExtIEs_t *)drbs_tobesetupmod_item_p->qoSInformation.choice.choice_extension;
          if (ie->id == F1AP_ProtocolIE_ID_id_DRB_Information && ie->criticality == F1AP_Criticality_reject
              && ie->value.present == F1AP_QoSInformation_ExtIEs__value_PR_DRB_Information) {
            F1AP_DRB_Information_t *dRB_Info = &ie->value.choice.DRB_Information;
            f1ap_drb_information_t *drb_info = &f1ap_ue_context_modification_req->drbs_to_be_setup->drb_info;

            /* 12.1.2.1 dRB_QoS */
            {
              /* QoS-Flow-Level-QoS-Parameters */
              f1ap_qos_flow_level_qos_parameters_t *drb_qos = &drb_info->drb_qos;
              F1AP_QoSFlowLevelQoSParameters_t *dRB_QoS = &dRB_Info->dRB_QoS;
              {
                /* QoS Characteristics*/
                f1ap_qos_characteristics_t *drb_qos_char = &drb_qos->qos_characteristics;
                F1AP_QoS_Characteristics_t *dRB_QoS_Char = &dRB_QoS->qoS_Characteristics;

                if (dRB_QoS_Char->present == F1AP_QoS_Characteristics_PR_non_Dynamic_5QI) {
                  drb_qos_char->qos_type = non_dynamic;
                  drb_qos_char->non_dynamic.fiveqi = dRB_QoS_Char->choice.non_Dynamic_5QI->fiveQI;
                  drb_qos_char->non_dynamic.qos_priority_level = (dRB_QoS_Char->choice.non_Dynamic_5QI->qoSPriorityLevel != NULL)
                                                                     ? *dRB_QoS_Char->choice.non_Dynamic_5QI->qoSPriorityLevel
                                                                     : -1;
                } else {
                  drb_qos_char->qos_type = dynamic;
                  drb_qos_char->dynamic.fiveqi =
                      (dRB_QoS_Char->choice.dynamic_5QI->fiveQI != NULL) ? *dRB_QoS_Char->choice.dynamic_5QI->fiveQI : -1;
                  drb_qos_char->dynamic.qos_priority_level = dRB_QoS_Char->choice.dynamic_5QI->qoSPriorityLevel;
                  drb_qos_char->dynamic.packet_delay_budget = dRB_QoS_Char->choice.dynamic_5QI->packetDelayBudget;
                  drb_qos_char->dynamic.packet_error_rate.per_scalar = dRB_QoS_Char->choice.dynamic_5QI->packetErrorRate.pER_Scalar;
                  drb_qos_char->dynamic.packet_error_rate.per_exponent =
                      dRB_QoS_Char->choice.dynamic_5QI->packetErrorRate.pER_Exponent;
                }
              }

              /* nGRANallocationRetentionPriority */
              drb_qos->alloc_reten_priority.priority_level = dRB_QoS->nGRANallocationRetentionPriority.priorityLevel;
              drb_qos->alloc_reten_priority.preemption_vulnerability =
                  dRB_QoS->nGRANallocationRetentionPriority.pre_emptionVulnerability;
              drb_qos->alloc_reten_priority.preemption_capability =
                  dRB_QoS->nGRANallocationRetentionPriority.pre_emptionVulnerability;
            } // dRB_QoS

            // 12.1.2.4 flows_Mapped_To_DRB_List
            drb_info->flows_to_be_setup_length = dRB_Info->flows_Mapped_To_DRB_List.list.count;
            drb_info->flows_mapped_to_drb = calloc(drb_info->flows_to_be_setup_length, sizeof(f1ap_flows_mapped_to_drb_t));
            AssertFatal(drb_info->flows_mapped_to_drb, "could not allocate memory for drb_p->drb_info.flows_mapped_to_drb\n");

            for (int k = 0; k < drb_p->drb_info.flows_to_be_setup_length; k++) {
              f1ap_flows_mapped_to_drb_t *flows_mapped_to_drb = drb_info->flows_mapped_to_drb + k;
              F1AP_Flows_Mapped_To_DRB_Item_t *flows_Mapped_To_Drb = dRB_Info->flows_Mapped_To_DRB_List.list.array[0] + k;

              flows_mapped_to_drb->qfi = flows_Mapped_To_Drb->qoSFlowIdentifier;

              /* QoS-Flow-Level-QoS-Parameters */
              {
                f1ap_qos_flow_level_qos_parameters_t *flow_qos = &flows_mapped_to_drb->qos_params;
                F1AP_QoSFlowLevelQoSParameters_t *Flow_QoS = &flows_Mapped_To_Drb->qoSFlowLevelQoSParameters;

                /* QoS Characteristics*/
                {
                  f1ap_qos_characteristics_t *flow_qos_char = &flow_qos->qos_characteristics;
                  F1AP_QoS_Characteristics_t *Flow_QoS_Char = &Flow_QoS->qoS_Characteristics;

                  if (Flow_QoS_Char->present == F1AP_QoS_Characteristics_PR_non_Dynamic_5QI) {
                    flow_qos_char->qos_type = non_dynamic;
                    flow_qos_char->non_dynamic.fiveqi = Flow_QoS_Char->choice.non_Dynamic_5QI->fiveQI;
                    flow_qos_char->non_dynamic.qos_priority_level =
                        (Flow_QoS_Char->choice.non_Dynamic_5QI->qoSPriorityLevel != NULL)
                            ? *Flow_QoS_Char->choice.non_Dynamic_5QI->qoSPriorityLevel
                            : -1;
                  } else {
                    flow_qos_char->qos_type = dynamic;
                    flow_qos_char->dynamic.fiveqi =
                        (Flow_QoS_Char->choice.dynamic_5QI->fiveQI != NULL) ? *Flow_QoS_Char->choice.dynamic_5QI->fiveQI : -1;
                    flow_qos_char->dynamic.qos_priority_level = Flow_QoS_Char->choice.dynamic_5QI->qoSPriorityLevel;
                    flow_qos_char->dynamic.packet_delay_budget = Flow_QoS_Char->choice.dynamic_5QI->packetDelayBudget;
                    flow_qos_char->dynamic.packet_error_rate.per_scalar =
                        Flow_QoS_Char->choice.dynamic_5QI->packetErrorRate.pER_Scalar;
                    flow_qos_char->dynamic.packet_error_rate.per_exponent =
                        Flow_QoS_Char->choice.dynamic_5QI->packetErrorRate.pER_Exponent;
                  }
                }

                /* nGRANallocationRetentionPriority */
                flow_qos->alloc_reten_priority.priority_level = Flow_QoS->nGRANallocationRetentionPriority.priorityLevel;
                flow_qos->alloc_reten_priority.preemption_vulnerability =
                    Flow_QoS->nGRANallocationRetentionPriority.pre_emptionVulnerability;
                flow_qos->alloc_reten_priority.preemption_capability =
                    Flow_QoS->nGRANallocationRetentionPriority.pre_emptionVulnerability;
              }
            }

            /* S-NSSAI */
            OCTET_STRING_TO_INT8(&dRB_Info->sNSSAI.sST, drb_p->nssai.sst);
            if (dRB_Info->sNSSAI.sD != NULL)
              memcpy((uint8_t *)&drb_p->nssai.sd, dRB_Info->sNSSAI.sD->buf, 3);
            else
              drb_p->nssai.sd = 0xffffff;
          }
        }
      }
    }
  }

  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t, ieDrb, container,
                   F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_List, false);
  if (ieDrb != NULL) {
    f1ap_ue_context_modification_req->drbs_to_be_released_length = ieDrb->value.choice.DRBs_ToBeReleased_List.list.count;
    f1ap_ue_context_modification_req->drbs_to_be_released =
        calloc(f1ap_ue_context_modification_req->drbs_to_be_released_length, sizeof(f1ap_drb_to_be_released_t));
    AssertFatal(f1ap_ue_context_modification_req->drbs_to_be_released,
                "could not allocate memory for f1ap_ue_context_released_req->drbs_to_be_released\n");
    for (i = 0; i < f1ap_ue_context_modification_req->drbs_to_be_released_length; ++i) {
      F1AP_DRBs_ToBeReleased_ItemIEs_t *tbrel = (F1AP_DRBs_ToBeReleased_ItemIEs_t *)ieDrb->value.choice.DRBs_ToBeReleased_List.list.array[i];
      DevAssert(tbrel->id == F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_Item);
      DevAssert(tbrel->value.present == F1AP_DRBs_ToBeReleased_ItemIEs__value_PR_DRBs_ToBeReleased_Item);
      f1ap_ue_context_modification_req->drbs_to_be_released[i].rb_id = tbrel->value.choice.DRBs_ToBeReleased_Item.dRBID;
      newGtpuDeleteOneTunnel(0, f1ap_ue_context_modification_req->gNB_DU_ue_id, f1ap_ue_context_modification_req->drbs_to_be_released[i].rb_id);
    }
  }

  /* RRC Reconfiguration Complete indicator */
  F1AP_UEContextModificationRequestIEs_t *ieReconf;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t, ieReconf, container,
      F1AP_ProtocolIE_ID_id_RRCReconfigurationCompleteIndicator, false);

  if(ieReconf) {
    switch(ieReconf->value.choice.RRCReconfigurationCompleteIndicator){
    case F1AP_RRCReconfigurationCompleteIndicator_true:
      f1ap_ue_context_modification_req->ReconfigComplOutcome = RRCreconf_success;
      break;

    case F1AP_RRCReconfigurationCompleteIndicator_failure:
      f1ap_ue_context_modification_req->ReconfigComplOutcome = RRCreconf_failure;
      break;
    }
  }

  /* RRCContainer */
  F1AP_UEContextModificationRequestIEs_t *ieRRC;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t, ieRRC, container,
      F1AP_ProtocolIE_ID_id_RRCContainer, false);

  if (ieRRC) {
    /* correct here */
    if ( ieRRC->value.choice.RRCContainer.size )  {
      f1ap_ue_context_modification_req->rrc_container = malloc(ieRRC->value.choice.RRCContainer.size);
      memcpy(f1ap_ue_context_modification_req->rrc_container,
          ieRRC->value.choice.RRCContainer.buf, ieRRC->value.choice.RRCContainer.size);
      f1ap_ue_context_modification_req->rrc_container_length = ieRRC->value.choice.RRCContainer.size;
    } else {
      LOG_E(F1AP, " RRCContainer in UEContextModificationRequestIEs size id 0\n");
    }
  } else {
    LOG_D(F1AP, "can't find RRCContainer in UEContextModificationRequestIEs by id %ld \n", F1AP_ProtocolIE_ID_id_RRCContainer);
  }

  /* CU2DU Information */
  F1AP_UEContextModificationRequestIEs_t *ieCU2DU = NULL;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequestIEs_t,
                             ieCU2DU,
                             container,
                             F1AP_ProtocolIE_ID_id_CUtoDURRCInformation,
                             false);
  if (ieCU2DU != NULL) {
    f1ap_ue_context_modification_req->cu_to_du_rrc_information =
        calloc(1, sizeof(*f1ap_ue_context_modification_req->cu_to_du_rrc_information));
    AssertFatal(f1ap_ue_context_modification_req->cu_to_du_rrc_information != NULL, "out of memory\n");
    cu_to_du_rrc_information_t *cu2du = f1ap_ue_context_modification_req->cu_to_du_rrc_information;
    const F1AP_CUtoDURRCInformation_t *cu2duie = &ieCU2DU->value.choice.CUtoDURRCInformation;
    if (cu2duie->cG_ConfigInfo != NULL)
      LOG_W(F1AP, "UE RNTI %04x: ignoring cg_ConfigInfo, not implemented\n", f1ap_ue_context_modification_req->gNB_DU_ue_id);
    if (cu2duie->measConfig != NULL)
      LOG_W(F1AP, "UE RNTI %04x: ignoring measConfig, not implemented\n", f1ap_ue_context_modification_req->gNB_DU_ue_id);
    if (cu2duie->uE_CapabilityRAT_ContainerList != NULL) {
      F1AP_UE_CapabilityRAT_ContainerList_t *uecap = cu2duie->uE_CapabilityRAT_ContainerList;
      cu2du->uE_CapabilityRAT_ContainerList = calloc(uecap->size, sizeof(*cu2du->uE_CapabilityRAT_ContainerList));
      AssertFatal(cu2du->uE_CapabilityRAT_ContainerList != NULL, "out of memory\n");
      cu2du->uE_CapabilityRAT_ContainerList_length = uecap->size;
      memcpy(cu2du->uE_CapabilityRAT_ContainerList, uecap->buf, uecap->size);
    }
  }

  ue_context_modification_request(f1ap_ue_context_modification_req);
  return 0;
}

int DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(sctp_assoc_t assoc_id, f1ap_ue_context_modif_resp_t *resp)
{
  F1AP_F1AP_PDU_t                        pdu= {0};
  F1AP_UEContextModificationResponse_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome,tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModification;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextModificationResponse;
  out = &tmp->value.choice.UEContextModificationResponse;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = resp->gNB_CU_ue_id;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = resp->gNB_DU_ue_id;

  /* optional */
  /* c3. ResourceCoordinationTransferContainer */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie3);
    ie3->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie3->criticality                    = F1AP_Criticality_ignore;
    ie3->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie3->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
  }

  /* optional */
  /* c4. DUtoCURRCInformation */
  if(resp->du_to_cu_rrc_information!=NULL){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie4);
    ie4->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCInformation;
    ie4->criticality                    = F1AP_Criticality_reject;
    ie4->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DUtoCURRCInformation;
    if(resp->du_to_cu_rrc_information->cellGroupConfig!=NULL){
      /* cellGroupConfig */
      OCTET_STRING_fromBuf(&ie4->value.choice.DUtoCURRCInformation.cellGroupConfig, (const char *)resp->du_to_cu_rrc_information->cellGroupConfig,
          resp->du_to_cu_rrc_information->cellGroupConfig_length);
    }

    /* OPTIONAL */
    /* measGapConfig */
    if (resp->du_to_cu_rrc_information->measGapConfig!=NULL) {
      OCTET_STRING_fromBuf(ie4->value.choice.DUtoCURRCInformation.measGapConfig, (const char *)resp->du_to_cu_rrc_information->measGapConfig,
        resp->du_to_cu_rrc_information->measGapConfig_length);
    }

    /* OPTIONAL */
    /* requestedP_MaxFR1 */
    if (resp->du_to_cu_rrc_information->requestedP_MaxFR1!=NULL) {
      OCTET_STRING_fromBuf(ie4->value.choice.DUtoCURRCInformation.requestedP_MaxFR1, (const char *)resp->du_to_cu_rrc_information->requestedP_MaxFR1,
          resp->du_to_cu_rrc_information->requestedP_MaxFR1_length);
    }
  }


  /* optional */
  /* c5. DRBs_Setup_List */
  if(resp->drbs_to_be_setup_length > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_DRBs_SetupMod_List;
    ie5->criticality                    = F1AP_Criticality_ignore;
    ie5->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_SetupMod_List;
    for (int i=0;  i< resp->drbs_to_be_setup_length; i++) {
      asn1cSequenceAdd(ie5->value.choice.DRBs_SetupMod_List.list, F1AP_DRBs_SetupMod_ItemIEs_t, drbs_setupmod_item_ies);
      drbs_setupmod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_SetupMod_Item;
      drbs_setupmod_item_ies->criticality   = F1AP_Criticality_ignore;
      drbs_setupmod_item_ies->value.present = F1AP_DRBs_SetupMod_ItemIEs__value_PR_DRBs_SetupMod_Item;
      /* 7.1 DRBs_Setup_Item */
      /* ADD */
      F1AP_DRBs_SetupMod_Item_t *drbs_setupmod_item=&drbs_setupmod_item_ies->value.choice.DRBs_SetupMod_Item;
      /* dRBID */
      drbs_setupmod_item->dRBID = resp->drbs_to_be_setup[i].drb_id;

      for (int j=0;  j<resp->drbs_to_be_setup[i].up_dl_tnl_length; j++) {
        f1ap_drb_to_be_setup_t *drb = &resp->drbs_to_be_setup[i];
        transport_layer_addr_t tl_addr = {0};
        memcpy(tl_addr.buffer, &drb->up_ul_tnl[0].tl_address, sizeof(drb->up_ul_tnl[0].tl_address));
        tl_addr.length = sizeof(drb->up_ul_tnl[0].tl_address) * 8;
        drb->up_dl_tnl[j].teid = newGtpuCreateTunnel(getCxt(0)->gtpInst,
                                                     resp->gNB_DU_ue_id,
                                                     drb->drb_id,
                                                     drb->drb_id,
                                                     drb->up_ul_tnl[j].teid,
                                                     -1, // no qfi
                                                     tl_addr,
                                                     drb->up_ul_tnl[0].port,
                                                     DURecvCb,
                                                     NULL);

        /* ADD */
        asn1cSequenceAdd(drbs_setupmod_item->dLUPTNLInformation_ToBeSetup_List.list,
          F1AP_DLUPTNLInformation_ToBeSetup_Item_t, dLUPTNLInformation_ToBeSetup_Item);
        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        /* gTPTunnel */
        asn1cCalloc(dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel,gTPTunnel);
        /* transportLayerAddress */
        struct sockaddr_in addr= {0};
        inet_pton(AF_INET, getCxt(0)->net_config.DU_f1_ip_address.ipv4_address, &addr.sin_addr.s_addr);
        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(addr.sin_addr.s_addr,
          &gTPTunnel->transportLayerAddress);
        /* gTP_TEID */
        INT32_TO_OCTET_STRING(resp->drbs_to_be_setup[i].up_dl_tnl[j].teid, &gTPTunnel->gTP_TEID);
      } // for j
    } // for i
  }

  /* optional */
  /* c6. DRBs_Modified_List */
  if(resp->drbs_to_be_modified_length > 0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie6);
    ie6->id                             = F1AP_ProtocolIE_ID_id_DRBs_Modified_List;
    ie6->criticality                    = F1AP_Criticality_ignore;
    ie6->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_Modified_List;

    for (int i=0;  i<resp->drbs_to_be_modified_length; i++) {
      asn1cSequenceAdd(ie6->value.choice.DRBs_Modified_List.list,
        F1AP_DRBs_Modified_ItemIEs_t, drbs_modified_item_ies);
      drbs_modified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_Modified_Item;
      drbs_modified_item_ies->criticality   = F1AP_Criticality_ignore;
      drbs_modified_item_ies->value.present = F1AP_DRBs_Modified_ItemIEs__value_PR_DRBs_Modified_Item;
      /* DRBs_modified_Item */
      F1AP_DRBs_Modified_Item_t *drbs_modified_item= &drbs_modified_item_ies->value.choice.DRBs_Modified_Item;
      /* dRBID */
      drbs_modified_item->dRBID = resp->drbs_to_be_modified[i].drb_id;
      
      /* ULTunnels_Modified_List */
      for (int j=0;  j<resp->drbs_to_be_modified[i].up_dl_tnl_length;  j++) {
        /*  DLTunnels_Modified_Item */
        asn1cSequenceAdd(drbs_modified_item->dLUPTNLInformation_ToBeSetup_List.list,
          F1AP_DLUPTNLInformation_ToBeSetup_Item_t, dLUPTNLInformation_ToBeSetup_Item);
        asn1cCalloc(dLUPTNLInformation_ToBeSetup_Item, tmp);
        tmp->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        asn1cCalloc(dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel, gTPTunnel);
        /* transportLayerAddress */
        struct sockaddr_in addr= {0};
        inet_pton(AF_INET, getCxt(0)->net_config.DU_f1_ip_address.ipv4_address, &addr.sin_addr.s_addr);
        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(addr.sin_addr.s_addr, &gTPTunnel->transportLayerAddress);
        /* gTP_TEID */
        INT32_TO_OCTET_STRING(resp->drbs_to_be_modified[i].up_dl_tnl[j].teid, &gTPTunnel->gTP_TEID);
      }
    }
  }

  /* optional */
  /* c7. SRBs_FailedToBeSetupMod_List */
  if(resp->srbs_failed_to_be_setup_length > 0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie7);
    ie7->id                             = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetupMod_List;
    ie7->criticality                    = F1AP_Criticality_reject;
    ie7->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_SRBs_FailedToBeSetupMod_List;

    for (int i=0; i<resp->srbs_to_be_setup_length; i++) {
      asn1cSequenceAdd(ie7->value.choice.SRBs_FailedToBeSetupMod_List.list,
        F1AP_SRBs_FailedToBeSetupMod_ItemIEs_t, srbs_failedToBeSetupMod_item_ies);
      srbs_failedToBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetupMod_Item;
      srbs_failedToBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
      srbs_failedToBeSetupMod_item_ies->value.present = F1AP_SRBs_FailedToBeSetupMod_ItemIEs__value_PR_SRBs_FailedToBeSetupMod_Item;
      /* 9.1 SRBs_FailedToBeSetupMod_Item */
      F1AP_SRBs_FailedToBeSetupMod_Item_t *srbs_failedToBeSetupMod_item=
        &srbs_failedToBeSetupMod_item_ies->value.choice.SRBs_FailedToBeSetupMod_Item;
      /* - sRBID */
      srbs_failedToBeSetupMod_item->sRBID = resp->srbs_failed_to_be_setup[i].rb_id;
      asn1cCalloc(srbs_failedToBeSetupMod_item->cause, tmp);
      tmp->present = F1AP_Cause_PR_radioNetwork;
      tmp->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_du_ue_f1ap_id;
    }
  }

  /* optional */
  /* c8. DRBs_FailedToBeSetupMod_List */
  if(resp->drbs_failed_to_be_setup_length > 0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie8);
    ie8->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetupMod_List;
    ie8->criticality                    = F1AP_Criticality_reject;
    ie8->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_FailedToBeSetupMod_List;

    for (int i=0;   i<resp->drbs_failed_to_be_setup_length; i++) {
      asn1cSequenceAdd(ie8->value.choice.DRBs_FailedToBeSetupMod_List.list,
        F1AP_DRBs_FailedToBeSetupMod_ItemIEs_t, drbs_failedToBeSetupMod_item_ies);
      drbs_failedToBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetupMod_Item;
      drbs_failedToBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
      drbs_failedToBeSetupMod_item_ies->value.present = F1AP_DRBs_FailedToBeSetupMod_ItemIEs__value_PR_DRBs_FailedToBeSetupMod_Item;
      /* DRBs_FailedToBeSetupMod_Item */
      F1AP_DRBs_FailedToBeSetupMod_Item_t *drbs_failedToBeSetupMod_item=
        &drbs_failedToBeSetupMod_item_ies->value.choice.DRBs_FailedToBeSetupMod_Item;
      /* dRBID */
      drbs_failedToBeSetupMod_item->dRBID = resp->drbs_failed_to_be_setup[i].rb_id;
      drbs_failedToBeSetupMod_item->cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
      drbs_failedToBeSetupMod_item->cause->present = F1AP_Cause_PR_radioNetwork;
      drbs_failedToBeSetupMod_item->cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_du_ue_f1ap_id;
    }
  }

  /* optional */
  /* c9. SCell_FailedtoSetupMod_List */
  if(0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie9);
    ie9->id                             = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetupMod_List;
    ie9->criticality                    = F1AP_Criticality_ignore;
    ie9->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_SCell_FailedtoSetupMod_List;

    for (int i=0; i<1; i++) {
      asn1cSequenceAdd(ie9->value.choice.SCell_FailedtoSetupMod_List.list,
        F1AP_SCell_FailedtoSetupMod_ItemIEs_t, scell_failedtoSetupMod_item_ies);
      scell_failedtoSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetupMod_Item;
      scell_failedtoSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
      scell_failedtoSetupMod_item_ies->value.present = F1AP_SCell_FailedtoSetupMod_ItemIEs__value_PR_SCell_FailedtoSetupMod_Item;
      /* 8.1 SCell_ToBeSetup_Item */
      F1AP_SCell_FailedtoSetupMod_Item_t *scell_failedtoSetupMod_item=&scell_failedtoSetupMod_item_ies->value.choice.SCell_FailedtoSetupMod_Item;
      /* - sCell_ID */
      addnRCGI(scell_failedtoSetupMod_item->sCell_ID, &getCxt(0)->setupReq.cell[i].info);
      asn1cCalloc(scell_failedtoSetupMod_item->cause, tmp);
      tmp->present = F1AP_Cause_PR_radioNetwork;
      tmp->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_du_ue_f1ap_id;
    }
  }

  /* optional */
  /* c10. DRBs_FailedToBeModified_List */
  if(0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie10);
    ie10->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeModified_List;
    ie10->criticality                    = F1AP_Criticality_reject;
    ie10->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_FailedToBeModified_List;

    for (int i=0; i<1; i++) {
      asn1cSequenceAdd(ie10->value.choice.DRBs_FailedToBeModified_List.list,
        F1AP_DRBs_FailedToBeModified_ItemIEs_t, drbs_failedToBeModified_item_ies);
      drbs_failedToBeModified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeModified_Item;
      drbs_failedToBeModified_item_ies->criticality   = F1AP_Criticality_reject;
      drbs_failedToBeModified_item_ies->value.present = F1AP_DRBs_FailedToBeModified_ItemIEs__value_PR_DRBs_FailedToBeModified_Item;
      /* 13.1 DRBs_FailedToBeModified_Item */
      F1AP_DRBs_FailedToBeModified_Item_t *drbs_failedToBeModified_item=
        &drbs_failedToBeModified_item_ies->value.choice.DRBs_FailedToBeModified_Item ;
      /* dRBID */
      drbs_failedToBeModified_item->dRBID = 30L;
      asn1cCalloc(drbs_failedToBeModified_item->cause, tmp);
      tmp->present = F1AP_Cause_PR_radioNetwork;
      tmp->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_du_ue_f1ap_id;
    }
  }

  // /*  */
  /* c11. CriticalityDiagnostics */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie11);
    ie11->id                             = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie11->criticality                    = F1AP_Criticality_ignore;
    ie11->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_CriticalityDiagnostics;

    // dummy value
    /* optional */
    /* procedureCode */
    if (0) {
      ie11->value.choice.CriticalityDiagnostics.procedureCode = (F1AP_ProcedureCode_t *)calloc(1, sizeof(F1AP_ProcedureCode_t));
      ie11->value.choice.CriticalityDiagnostics.procedureCode = 0L;
    }

    /* optional */
    /* triggeringMessage */
    if (0) {
      ie11->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)calloc(1, sizeof(F1AP_TriggeringMessage_t));
      *ie11->value.choice.CriticalityDiagnostics.triggeringMessage = F1AP_TriggeringMessage_successful_outcome;
    }

    /* optional */
    /* procedureCriticality */
    if (0) {
      ie11->value.choice.CriticalityDiagnostics.procedureCriticality = (F1AP_Criticality_t *)calloc(1, sizeof(F1AP_Criticality_t));
      *ie11->value.choice.CriticalityDiagnostics.procedureCriticality = F1AP_Criticality_reject;
    }

    /* optional */
    /* transactionID */
    if (0) {
      ie11->value.choice.CriticalityDiagnostics.transactionID = (F1AP_TransactionID_t *)calloc(1, sizeof(F1AP_TransactionID_t));
      ie11->value.choice.CriticalityDiagnostics.transactionID = 0L;
    }

    /* optional */
    /* F1AP_CriticalityDiagnostics_IE_List */
    if (0) {
      for (int i=0; i<0; i++) {
        asn1cSequenceAdd(ie11->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics->list, F1AP_CriticalityDiagnostics_IE_Item_t, criticalityDiagnostics_ie_item);
        criticalityDiagnostics_ie_item->iECriticality = F1AP_Criticality_reject;
        criticalityDiagnostics_ie_item->iE_ID         = 0L;
        criticalityDiagnostics_ie_item->typeOfError   = F1AP_TypeOfError_not_understood;
      }
    }
  }
  /* optional */
  /* c12. SRBs_Setup_List */
  if(resp->srbs_to_be_setup_length > 0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationResponseIEs_t, ie12);
    ie12->id                             = F1AP_ProtocolIE_ID_id_SRBs_SetupMod_List;
    ie12->criticality                    = F1AP_Criticality_ignore;
    ie12->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_SRBs_SetupMod_List;

    for (int i=0;  i< resp->srbs_to_be_setup_length; i++) {//
      asn1cSequenceAdd(ie12->value.choice.SRBs_SetupMod_List.list,
        F1AP_SRBs_SetupMod_ItemIEs_t, srbs_setupmod_item_ies);
      srbs_setupmod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_SetupMod_Item;
      srbs_setupmod_item_ies->criticality   = F1AP_Criticality_ignore;
      srbs_setupmod_item_ies->value.present = F1AP_SRBs_SetupMod_ItemIEs__value_PR_SRBs_SetupMod_Item;
      /* 11.1 SRBs_Setup_Item */
      /* ADD */
      F1AP_SRBs_SetupMod_Item_t *srbs_setupmod_item=&srbs_setupmod_item_ies->value.choice.SRBs_SetupMod_Item;
      /* sRBID */
      srbs_setupmod_item->sRBID = resp->srbs_to_be_setup[i].srb_id;
      srbs_setupmod_item->lCID = resp->srbs_to_be_setup[i].srb_id;
    }
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE CONTEXT MODIFICATION RESPONSE\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_send_UE_CONTEXT_MODIFICATION_FAILURE(sctp_assoc_t assoc_id) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_send_UE_CONTEXT_MODIFICATION_REQUIRED(sctp_assoc_t assoc_id, f1ap_ue_context_modif_required_t *required)
{
  /* 0. Message Type */
  F1AP_F1AP_PDU_t pdu = {0};
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModificationRequired;
  tmp->criticality = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextModificationRequired;
  F1AP_UEContextModificationRequired_t *out = &tmp->value.choice.UEContextModificationRequired;

  /* mandatory GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequiredIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_UEContextModificationRequiredIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = required->gNB_CU_ue_id;

  /* mandatory: GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequiredIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality = F1AP_Criticality_reject;
  ie2->value.present = F1AP_UEContextModificationRequiredIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = required->gNB_DU_ue_id;

  /* optional: Resource Coordination Transfer Container */
  /* not implemented!*/

  /* optional: DU-to-CU RRC Container */
  if (required->du_to_cu_rrc_information) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequiredIEs_t, ie3);
    ie3->id = F1AP_ProtocolIE_ID_id_DUtoCURRCInformation;
    ie3->criticality = F1AP_Criticality_reject;
    ie3->value.present = F1AP_UEContextModificationRequiredIEs__value_PR_DUtoCURRCInformation;

    const du_to_cu_rrc_information_t *du2cu = required->du_to_cu_rrc_information;
    AssertFatal(du2cu->cellGroupConfig != NULL, "du2cu cellGroupConfig is mandatory!\n");

    /* mandatorycellGroupConfig */
    OCTET_STRING_fromBuf(&ie3->value.choice.DUtoCURRCInformation.cellGroupConfig,
                         (const char *)du2cu->cellGroupConfig,
                         du2cu->cellGroupConfig_length);

    /* optional: measGapConfig */
    if (du2cu->measGapConfig != NULL) {
      OCTET_STRING_fromBuf(ie3->value.choice.DUtoCURRCInformation.measGapConfig,
                           (const char *)du2cu->measGapConfig,
                           du2cu->measGapConfig_length);
    }

    /* optional: requestedP_MaxFR1 */
    if (du2cu->requestedP_MaxFR1 != NULL) {
      OCTET_STRING_fromBuf(ie3->value.choice.DUtoCURRCInformation.requestedP_MaxFR1,
                           (const char *)du2cu->requestedP_MaxFR1,
                           du2cu->requestedP_MaxFR1_length);
    }
  }

  /* optional: DRB required to be modified */
  /* not implemented */

  /* optional: SRB required to be released */
  /* not implemented */

  /* optional: DRB required to be released */
  /* not implemented */

  /* mandatory: cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequiredIEs_t, ie4);
  ie4->id = F1AP_ProtocolIE_ID_id_Cause;
  ie4->criticality = F1AP_Criticality_reject;
  ie4->value.present = F1AP_UEContextModificationRequiredIEs__value_PR_Cause;
  F1AP_Cause_t *cause = &ie4->value.choice.Cause;
  switch (required->cause) {
    case F1AP_CAUSE_RADIO_NETWORK:
      cause->present = F1AP_Cause_PR_radioNetwork;
      cause->choice.radioNetwork = required->cause_value;
      break;
    case F1AP_CAUSE_TRANSPORT:
      cause->present = F1AP_Cause_PR_transport;
      cause->choice.transport = required->cause_value;
      break;
    case F1AP_CAUSE_PROTOCOL:
      cause->present = F1AP_Cause_PR_protocol;
      cause->choice.protocol = required->cause_value;
      break;
    case F1AP_CAUSE_MISC:
      cause->present = F1AP_Cause_PR_misc;
      cause->choice.misc = required->cause_value;
      break;
    case F1AP_CAUSE_NOTHING:
    default:
      cause->present = F1AP_Cause_PR_NOTHING;
      break;
  } // switch

  /* optional: BH RLC Channel Required to be Released List */
  /* not implemented */

  /* optional: SL DRB Required to Be Modified List */
  /* not implemented */

  /* optional: SL DRB Required to be Released List */
  /* not implemented */

  /* optional: Candidate Cells To Be Cancelled List */
  /* not implemented */

  /* encode */
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release request\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_handle_UE_CONTEXT_MODIFICATION_CONFIRM(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_UEContextModificationConfirm_t *container = &pdu->choice.successfulOutcome->value.choice.UEContextModificationConfirm;
  f1ap_ue_context_modif_confirm_t confirm = {0};
  F1AP_UEContextModificationConfirmIEs_t *ie = NULL;

  /* mandatory: GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  confirm.gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

  /* mandatory: GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  confirm.gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* optional: Resource Coordination Transfer Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer,
                             false);
  AssertFatal(ie == NULL, "handling of Resource Coordination Transfer Container not implemented\n");

  /* optional: DRBS Modified List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_DRBs_Modified_List,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Modified List not implemented\n");

  /* optional: RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t, ie, container, F1AP_ProtocolIE_ID_id_RRCContainer, false);
  if (ie != NULL) {
    F1AP_RRCContainer_t *rrc_container = &ie->value.choice.RRCContainer;
    confirm.rrc_container = malloc(rrc_container->size);
    AssertFatal(confirm.rrc_container != NULL, "memory allocation failed\n");
    memcpy(confirm.rrc_container, rrc_container->buf, rrc_container->size);
    confirm.rrc_container_length = rrc_container->size;
  }

  /* optional: Criticality Diagnostics */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_CriticalityDiagnostics,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Modified List not implemented\n");

  /* optional: Execute Duplication */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_ExecuteDuplication,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Modified List not implemented\n");

  /* optional: Resource Coordination Transfer Information */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferInformation,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Modified List not implemented\n");

  /* optional: SL DRB Modified List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationConfirmIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_SLDRBs_Modified_List,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Modified List not implemented\n");

  ue_context_modification_confirm(&confirm);
  return 0;
}

int DU_handle_UE_CONTEXT_MODIFICATION_REFUSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_UEContextModificationRefuse_t *container = &pdu->choice.unsuccessfulOutcome->value.choice.UEContextModificationRefuse;
  f1ap_ue_context_modif_refuse_t refuse = {0};
  F1AP_UEContextModificationRefuseIEs_t *ie = NULL;

  /* mandatory: GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRefuseIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  refuse.gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

  /* mandatory: GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRefuseIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  refuse.gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* mandatory: Cause */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRefuseIEs_t, ie, container, F1AP_ProtocolIE_ID_id_Cause, true);
  switch (ie->value.choice.Cause.present) {
    case F1AP_Cause_PR_radioNetwork:
      refuse.cause = F1AP_CAUSE_RADIO_NETWORK;
      refuse.cause_value = ie->value.choice.Cause.choice.radioNetwork;
      break;
    case F1AP_Cause_PR_transport:
      refuse.cause = F1AP_CAUSE_TRANSPORT;
      refuse.cause_value = ie->value.choice.Cause.choice.transport;
      break;
    case F1AP_Cause_PR_protocol:
      refuse.cause = F1AP_CAUSE_PROTOCOL;
      refuse.cause_value = ie->value.choice.Cause.choice.protocol;
      break;
    case F1AP_Cause_PR_misc:
      refuse.cause = F1AP_CAUSE_MISC;
      refuse.cause_value = ie->value.choice.Cause.choice.misc;
      break;
    default:
      LOG_W(F1AP, "Unknown cause for UE Context Modification Refuse message\n");
      /* fall through */
    case F1AP_Cause_PR_NOTHING:
      refuse.cause = F1AP_CAUSE_NOTHING;
      break;
  }

  /* optional: Criticality Diagnostics */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRefuseIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_CriticalityDiagnostics,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Modified List not implemented\n");

  ue_context_modification_refuse(&refuse);
  return 0;
}
