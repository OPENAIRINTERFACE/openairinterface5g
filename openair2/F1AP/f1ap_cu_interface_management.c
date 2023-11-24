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

/*! \file f1ap_cu_interface_management.c
 * \brief f1ap interface management for CU
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
#include "f1ap_cu_interface_management.h"

int CU_send_RESET(sctp_assoc_t assoc_id, F1AP_Reset_t *Reset)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_RESET_ACKKNOWLEDGE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_RESET(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_RESET_ACKNOWLEDGE(sctp_assoc_t assoc_id, F1AP_ResetAcknowledge_t *ResetAcknowledge)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
    Error Indication
*/
int CU_handle_ERROR_INDICATION(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_ERROR_INDICATION(sctp_assoc_t assoc_id, F1AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
    F1 Setup
*/
int CU_handle_F1_SETUP_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "CU_handle_F1_SETUP_REQUEST\n");
  F1AP_F1SetupRequest_t              *container;
  F1AP_F1SetupRequestIEs_t           *ie;
  int i = 0;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.F1SetupRequest;

  /* F1 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_W(F1AP, "[SCTP %d] Received f1 setup request on stream != 0 (%d)\n",
          assoc_id, stream);
  }

  MessageDef *message_p = itti_alloc_new_message(TASK_CU_F1, 0, F1AP_SETUP_REQ);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_setup_req_t *req = &F1AP_SETUP_REQ(message_p);

  /* Transaction ID*/
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_TransactionID, true);
  req->transaction_id = ie->value.choice.TransactionID;
  LOG_D(F1AP, "req->transaction_id %lu \n", req->transaction_id);

  /* gNB_DU_id */
  // this function exits if the ie is mandatory
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_ID, true);
  asn_INTEGER2ulong(&ie->value.choice.GNB_DU_ID, &req->gNB_DU_id);
  LOG_D(F1AP, "req->gNB_DU_id %lu \n", req->gNB_DU_id);
  /* gNB_DU_name */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_Name, false);
  req->gNB_DU_name = NULL;
  if (ie != NULL) {
    req->gNB_DU_name = calloc(ie->value.choice.GNB_DU_Name.size + 1, sizeof(char));
    memcpy(req->gNB_DU_name, ie->value.choice.GNB_DU_Name.buf, ie->value.choice.GNB_DU_Name.size);
    LOG_D(F1AP, "req->gNB_DU_name %s \n", req->gNB_DU_name);
  }
  /* GNB_DU_Served_Cells_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_Served_Cells_List, true);
  req->num_cells_available = ie->value.choice.GNB_DU_Served_Cells_List.list.count;
  LOG_D(F1AP, "req->num_cells_available %d \n", req->num_cells_available);

  for (i=0; i<req->num_cells_available; i++) {
    F1AP_GNB_DU_Served_Cells_Item_t *served_cells_item = &(((F1AP_GNB_DU_Served_Cells_ItemIEs_t *)
							    ie->value.choice.GNB_DU_Served_Cells_List.list.array[i])->
							   value.choice.GNB_DU_Served_Cells_Item);
    F1AP_Served_Cell_Information_t *servedCellInformation= &served_cells_item->served_Cell_Information;
    /* tac */
    if (servedCellInformation->fiveGS_TAC) {
      req->cell[i].info.tac = malloc(sizeof(*req->cell[i].info.tac));
      AssertFatal(req->cell[i].info.tac != NULL, "out of memory\n");
      OCTET_STRING_TO_INT16(servedCellInformation->fiveGS_TAC, *req->cell[i].info.tac);
      LOG_D(F1AP, "req->tac[%d] %d \n", i, *req->cell[i].info.tac);
    }
    
    /* - nRCGI */
    TBCD_TO_MCC_MNC(&(servedCellInformation->nRCGI.pLMN_Identity),
                    req->cell[i].info.plmn.mcc,
                    req->cell[i].info.plmn.mnc,
                    req->cell[i].info.plmn.mnc_digit_length);
    // NR cellID
    BIT_STRING_TO_NR_CELL_IDENTITY(&servedCellInformation->nRCGI.nRCellIdentity,
                                   req->cell[i].info.nr_cellid);
    LOG_D(F1AP, "[SCTP %d] Received nRCGI: MCC %d, MNC %d, CELL_ID %llu\n", assoc_id,
          req->cell[i].info.plmn.mcc,
          req->cell[i].info.plmn.mnc,
          (long long unsigned int)req->cell[i].info.nr_cellid);
    /* - nRPCI */
    req->cell[i].info.nr_pci = servedCellInformation->nRPCI;
    LOG_D(F1AP, "req->nr_pci[%d] %d \n", i, req->cell[i].info.nr_pci);

    // FDD Cells
    if (servedCellInformation->nR_Mode_Info.present==F1AP_NR_Mode_Info_PR_fDD) {
      req->cell[i].info.mode = F1AP_MODE_FDD;
      f1ap_fdd_info_t *FDDs = &req->cell[i].info.fdd;
      F1AP_FDD_Info_t * fDD_Info=servedCellInformation->nR_Mode_Info.choice.fDD;
      FDDs->ul_freqinfo.arfcn = fDD_Info->uL_NRFreqInfo.nRARFCN;
      AssertFatal(fDD_Info->uL_NRFreqInfo.freqBandListNr.list.count == 1, "cannot handle more than one frequency band\n");
      for (int f=0; f < fDD_Info->uL_NRFreqInfo.freqBandListNr.list.count; f++) {
        F1AP_FreqBandNrItem_t * FreqItem=fDD_Info->uL_NRFreqInfo.freqBandListNr.list.array[f];
        FDDs->ul_freqinfo.band = FreqItem->freqBandIndicatorNr;
        AssertFatal(FreqItem->supportedSULBandList.list.count == 0, "cannot handle SUL bands!\n");
      }
      FDDs->dl_freqinfo.arfcn = fDD_Info->dL_NRFreqInfo.nRARFCN;
      int dlBands=fDD_Info->dL_NRFreqInfo.freqBandListNr.list.count;
      AssertFatal(dlBands == 1, "cannot handled more than one frequency band\n");
      for (int dlB=0; dlB < dlBands; dlB++) {
	F1AP_FreqBandNrItem_t * FreqItem=fDD_Info->dL_NRFreqInfo.freqBandListNr.list.array[dlB];
        FDDs->dl_freqinfo.band = FreqItem->freqBandIndicatorNr;
        int num_available_supported_SULBands = FreqItem->supportedSULBandList.list.count;
        AssertFatal(num_available_supported_SULBands == 0, "cannot handle SUL bands!\n");
      }
      FDDs->ul_tbw.scs = fDD_Info->uL_Transmission_Bandwidth.nRSCS;
      FDDs->ul_tbw.nrb = nrb_lut[fDD_Info->uL_Transmission_Bandwidth.nRNRB];
      FDDs->dl_tbw.scs = fDD_Info->dL_Transmission_Bandwidth.nRSCS;
      FDDs->dl_tbw.nrb = nrb_lut[fDD_Info->dL_Transmission_Bandwidth.nRNRB];
    } else if (servedCellInformation->nR_Mode_Info.present==F1AP_NR_Mode_Info_PR_tDD) {
      req->cell[i].info.mode = F1AP_MODE_TDD;
      f1ap_tdd_info_t *TDDs = &req->cell[i].info.tdd;
      F1AP_TDD_Info_t *tDD_Info = servedCellInformation->nR_Mode_Info.choice.tDD;
      TDDs->freqinfo.arfcn = tDD_Info->nRFreqInfo.nRARFCN;
      AssertFatal(tDD_Info->nRFreqInfo.freqBandListNr.list.count == 1, "cannot handle more than one frequency band\n");
      for (int f=0; f < tDD_Info->nRFreqInfo.freqBandListNr.list.count; f++) {
        struct F1AP_FreqBandNrItem * FreqItem=tDD_Info->nRFreqInfo.freqBandListNr.list.array[f];
        TDDs->freqinfo.band = FreqItem->freqBandIndicatorNr;
        int num_available_supported_SULBands = FreqItem->supportedSULBandList.list.count;
        AssertFatal(num_available_supported_SULBands == 0, "cannot hanlde SUL bands!\n");
      }
      TDDs->tbw.scs = tDD_Info->transmission_Bandwidth.nRSCS;
      TDDs->tbw.nrb = nrb_lut[tDD_Info->transmission_Bandwidth.nRNRB];
    } else {
      AssertFatal(false, "unknown NR Mode info %d\n", servedCellInformation->nR_Mode_Info.present);
    }

    struct F1AP_GNB_DU_System_Information * DUsi=served_cells_item->gNB_DU_System_Information;
    if (DUsi != NULL) {
      // System Information
      req->cell[i].sys_info = calloc(1, sizeof(*req->cell[i].sys_info));
      AssertFatal(req->cell[i].sys_info != NULL, "out of memory\n");
      f1ap_gnb_du_system_info_t *sys_info = req->cell[i].sys_info;
      /* mib */
      sys_info->mib = calloc(DUsi->mIB_message.size, sizeof(char));
      memcpy(sys_info->mib, DUsi->mIB_message.buf, DUsi->mIB_message.size);
      sys_info->mib_length = DUsi->mIB_message.size;
      /* sib1 */
      sys_info->sib1 = calloc(DUsi->sIB1_message.size, sizeof(char));
      memcpy(sys_info->sib1, DUsi->sIB1_message.buf, DUsi->sIB1_message.size);
      sys_info->sib1_length = DUsi->sIB1_message.size;
    }
  }

   /* Handle RRC Version */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_GNB_DU_RRC_Version, true);
  // Latest RRC Version: "This IE is not used in this release."
  // BIT_STRING_to_uint8(&ie->value.choice.RRC_Version.latest_RRC_Version);
  if (ie->value.choice.RRC_Version.iE_Extensions) {
    F1AP_ProtocolExtensionContainer_10696P228_t *ext =
        (F1AP_ProtocolExtensionContainer_10696P228_t *)ie->value.choice.RRC_Version.iE_Extensions;
    if (ext->list.count > 0) {
      F1AP_RRC_Version_ExtIEs_t *rrcext = ext->list.array[0];
      OCTET_STRING_t *os = &rrcext->extensionValue.choice.OCTET_STRING_SIZE_3_;
      DevAssert(os->size == 3);
      for (int i = 0; i < 3; ++i)
        req->rrc_ver[i] = os->buf[i];
    }
  }
  itti_send_msg_to_task(TASK_RRC_GNB, GNB_MODULE_ID_TO_INSTANCE(instance), message_p);
  
  return 0;
}

int CU_send_F1_SETUP_RESPONSE(sctp_assoc_t assoc_id, f1ap_setup_resp_t *f1ap_setup_resp)
{
  F1AP_F1AP_PDU_t           pdu= {0};
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_F1SetupResponse;
  F1AP_F1SetupResponse_t    *out = &pdu.choice.successfulOutcome->value.choice.F1SetupResponse;
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie1);
  ie1->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie1->criticality               = F1AP_Criticality_reject;
  ie1->value.present             = F1AP_F1SetupResponseIEs__value_PR_TransactionID;
  ie1->value.choice.TransactionID = f1ap_setup_resp->transaction_id;

  /* optional */
  /* c2. GNB_CU_Name */
  if (f1ap_setup_resp->gNB_CU_name != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie2);
    ie2->id                        = F1AP_ProtocolIE_ID_id_gNB_CU_Name;
    ie2->criticality               = F1AP_Criticality_ignore;
    ie2->value.present             = F1AP_F1SetupResponseIEs__value_PR_GNB_CU_Name;
    OCTET_STRING_fromBuf(&ie2->value.choice.GNB_CU_Name, f1ap_setup_resp->gNB_CU_name,
                         strlen(f1ap_setup_resp->gNB_CU_name));
  }

  /* mandatory */
  /* c3. cells to be Activated list */
  int num_cells_to_activate = f1ap_setup_resp->num_cells_to_activate;
  LOG_D(F1AP, "num_cells_to_activate = %d \n", num_cells_to_activate);

  if (num_cells_to_activate >0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie3);
    ie3->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List;
    ie3->criticality               = F1AP_Criticality_reject;
    ie3->value.present             = F1AP_F1SetupResponseIEs__value_PR_Cells_to_be_Activated_List;

    for (int i=0; i<num_cells_to_activate;  i++) {
      asn1cSequenceAdd(ie3->value.choice.Cells_to_be_Activated_List.list,
                       F1AP_Cells_to_be_Activated_List_ItemIEs_t, cells_to_be_activated_ies);
      cells_to_be_activated_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
      cells_to_be_activated_ies->criticality = F1AP_Criticality_reject;
      cells_to_be_activated_ies->value.present = F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item;
      /* 3.1 cells to be Activated list item */
      F1AP_Cells_to_be_Activated_List_Item_t *cells_to_be_activated_item=
        &cells_to_be_activated_ies->value.choice.Cells_to_be_Activated_List_Item;
      /* - nRCGI */
      addnRCGI(cells_to_be_activated_item->nRCGI, f1ap_setup_resp->cells_to_activate+i);

      /* optional */
      /* - nRPCI */
      if (1) {
        cells_to_be_activated_item->nRPCI = (F1AP_NRPCI_t *)calloc(1, sizeof(F1AP_NRPCI_t));
        *cells_to_be_activated_item->nRPCI = f1ap_setup_resp->cells_to_activate[i].nrpci;  // int 0..1007
      }

      /* optional */
      /* - gNB-CU System Information */
      if (f1ap_setup_resp->cells_to_activate[i].num_SI > 0) {
        /* 3.1.2 gNB-CUSystem Information */
        F1AP_ProtocolExtensionContainer_10696P112_t *p = calloc(1, sizeof(* p));
        cells_to_be_activated_item->iE_Extensions = (struct F1AP_ProtocolExtensionContainer *) p;
        asn1cSequenceAdd(p->list, F1AP_Cells_to_be_Activated_List_ItemExtIEs_t, cells_to_be_activated_itemExtIEs);
        cells_to_be_activated_itemExtIEs->id                     = F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation;
        cells_to_be_activated_itemExtIEs->criticality            = F1AP_Criticality_reject;
        cells_to_be_activated_itemExtIEs->extensionValue.present = F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation;
        F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation =
          &cells_to_be_activated_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation;

        // for (int sIBtype=2;sIBtype<33;sIBtype++) { //21 ? 33 ?
        for (int sIBtype=2; sIBtype<21; sIBtype++) {
          if (f1ap_setup_resp->cells_to_activate[i].SI_container[sIBtype]!=NULL) {
            AssertFatal(sIBtype < 6 || sIBtype == 9, "Illegal SI type %d\n",sIBtype);
            asn1cSequenceAdd(gNB_CUSystemInformation->sibtypetobeupdatedlist.list, F1AP_SibtypetobeupdatedListItem_t, sib_item);
            sib_item->sIBtype = sIBtype;
            OCTET_STRING_fromBuf(&sib_item->sIBmessage,
                                 (const char *)f1ap_setup_resp->cells_to_activate[i].SI_container[sIBtype],
                                 f1ap_setup_resp->cells_to_activate[i].SI_container_length[sIBtype]);
            LOG_D(F1AP, "f1ap_setup_resp->SI_container_length[%d][%d] = %d \n", i,sIBtype,f1ap_setup_resp->cells_to_activate[i].SI_container_length[sIBtype]);
          }
        }
      }
    }
  }

  /* c5. RRC VERSION */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie4);
  ie4->id = F1AP_ProtocolIE_ID_id_GNB_CU_RRC_Version;
  ie4->criticality = F1AP_Criticality_reject;
  ie4->value.present = F1AP_F1SetupResponseIEs__value_PR_RRC_Version;
  // RRC Version: "This IE is not used in this release."
  // we put one bit for each byte in rrc_ver that is != 0
  uint8_t bits = 0;
  for (int i = 0; i < 3; ++i)
    bits |= (f1ap_setup_resp->rrc_ver[i] != 0) << i;
  BIT_STRING_t *bs = &ie4->value.choice.RRC_Version.latest_RRC_Version;
  bs->buf = calloc(1, sizeof(char));
  AssertFatal(bs->buf != NULL, "out of memory\n");
  bs->buf[0] = bits;
  bs->size = 1;
  bs->bits_unused = 5;

  F1AP_ProtocolExtensionContainer_10696P228_t *p = (F1AP_ProtocolExtensionContainer_10696P228_t*)calloc(1, sizeof(F1AP_ProtocolExtensionContainer_10696P228_t));    
  asn1cSequenceAdd(p->list, F1AP_RRC_Version_ExtIEs_t, rrcv_ext);
  rrcv_ext->id = F1AP_ProtocolIE_ID_id_latest_RRC_Version_Enhanced;
  rrcv_ext->criticality = F1AP_Criticality_ignore;
  rrcv_ext->extensionValue.present = F1AP_RRC_Version_ExtIEs__extensionValue_PR_OCTET_STRING_SIZE_3_;
  OCTET_STRING_t *os = &rrcv_ext->extensionValue.choice.OCTET_STRING_SIZE_3_;
  os->size = 3;
  os->buf = malloc(3 * sizeof(*os->buf));
  AssertFatal(os->buf != NULL, "out of memory\n");
  for (int i = 0; i < 3; ++i)
    os->buf[i] = f1ap_setup_resp->rrc_ver[i];
  ie4->value.choice.RRC_Version.iE_Extensions = (struct F1AP_ProtocolExtensionContainer *)p;

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup response\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_send_F1_SETUP_FAILURE(sctp_assoc_t assoc_id, const f1ap_setup_failure_t *fail)
{
  LOG_D(F1AP, "CU_send_F1_SETUP_FAILURE\n");
  F1AP_F1AP_PDU_t           pdu= {0};
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  asn1cCalloc(pdu.choice.unsuccessfulOutcome, UnsuccessfulOutcome);
  pdu.present = F1AP_F1AP_PDU_PR_unsuccessfulOutcome;
  UnsuccessfulOutcome->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  UnsuccessfulOutcome->criticality   = F1AP_Criticality_reject;
  UnsuccessfulOutcome->value.present = F1AP_UnsuccessfulOutcome__value_PR_F1SetupFailure;
  F1AP_F1SetupFailure_t *out = &pdu.choice.unsuccessfulOutcome->value.choice.F1SetupFailure;
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie1);
  ie1->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie1->criticality               = F1AP_Criticality_reject;
  ie1->value.present             = F1AP_F1SetupFailureIEs__value_PR_TransactionID;
  ie1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);
  /* mandatory */
  /* c2. Cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie2);
  ie2->id                        = F1AP_ProtocolIE_ID_id_Cause;
  ie2->criticality               = F1AP_Criticality_ignore;
  ie2->value.present             = F1AP_F1SetupFailureIEs__value_PR_Cause;
  ie2->value.choice.Cause.present = F1AP_Cause_PR_radioNetwork;
  ie2->value.choice.Cause.choice.radioNetwork = F1AP_CauseRadioNetwork_unspecified;

  /* optional */
  /* c3. TimeToWait */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie3);
    ie3->id                        = F1AP_ProtocolIE_ID_id_TimeToWait;
    ie3->criticality               = F1AP_Criticality_ignore;
    ie3->value.present             = F1AP_F1SetupFailureIEs__value_PR_TimeToWait;
    ie3->value.choice.TimeToWait = F1AP_TimeToWait_v10s;
  }

  /* optional */
  /* c4. CriticalityDiagnostics*/
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie4);
    ie4->id                        = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie4->criticality               = F1AP_Criticality_ignore;
    ie4->value.present             = F1AP_F1SetupFailureIEs__value_PR_CriticalityDiagnostics;
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.procedureCode,
                   F1AP_ProcedureCode_id_UEContextSetup);
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.triggeringMessage,
                   F1AP_TriggeringMessage_initiating_message);
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.procedureCriticality,
                   F1AP_Criticality_reject);
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.transactionID,
                   0);
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup failure\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

/*
    gNB-DU Configuration Update
*/

int CU_handle_gNB_DU_CONFIGURATION_UPDATE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_gNB_DU_CONFIGURATION_FAILURE(sctp_assoc_t assoc_id,
    F1AP_GNBDUConfigurationUpdateFailure_t *GNBDUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(sctp_assoc_t assoc_id,
    F1AP_GNBDUConfigurationUpdateAcknowledge_t *GNBDUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
    gNB-CU Configuration Update
*/

int CU_send_gNB_CU_CONFIGURATION_UPDATE(sctp_assoc_t assoc_id, f1ap_gnb_cu_configuration_update_t *f1ap_gnb_cu_configuration_update)
{
  F1AP_F1AP_PDU_t                    pdu= {0};
  uint8_t  *buffer;
  uint32_t  len;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, initMsg);
  initMsg->procedureCode = F1AP_ProcedureCode_id_gNBCUConfigurationUpdate;
  initMsg->criticality   = F1AP_Criticality_reject;
  initMsg->value.present = F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate;
  F1AP_GNBCUConfigurationUpdate_t *cfgUpdate = &pdu.choice.initiatingMessage->value.choice.GNBCUConfigurationUpdate;
  /* mandatory */
  /* c1. Transaction ID (integer value) */
  asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC1);
  ieC1->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ieC1->criticality               = F1AP_Criticality_reject;
  ieC1->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_TransactionID;
  ieC1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);

  // mandatory
  // c2. Cells_to_be_Activated_List
  if (f1ap_gnb_cu_configuration_update->num_cells_to_activate > 0) {
    asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC3);
    ieC3->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List;
    ieC3->criticality               = F1AP_Criticality_reject;
    ieC3->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Activated_List;

    for (int i=0; i<f1ap_gnb_cu_configuration_update->num_cells_to_activate; i++) {
      asn1cSequenceAdd(ieC3->value.choice.Cells_to_be_Activated_List.list,F1AP_Cells_to_be_Activated_List_ItemIEs_t,
                       cells_to_be_activated_ies);
      cells_to_be_activated_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
      cells_to_be_activated_ies->criticality = F1AP_Criticality_reject;
      cells_to_be_activated_ies->value.present = F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item;
      // 2.1 cells to be Activated list item
      F1AP_Cells_to_be_Activated_List_Item_t *cells_to_be_activated_list_item=
        &cells_to_be_activated_ies->value.choice.Cells_to_be_Activated_List_Item;
      // - nRCGI
      addnRCGI(cells_to_be_activated_list_item->nRCGI, f1ap_gnb_cu_configuration_update->cells_to_activate+i);
      // optional
      // -nRPCI
      asn1cCalloc(cells_to_be_activated_list_item->nRPCI, tmp);
      *tmp = f1ap_gnb_cu_configuration_update->cells_to_activate[i].nrpci;  // int 0..1007
      // optional
      // 3.1.2 gNB-CUSystem Information
      F1AP_ProtocolExtensionContainer_10696P112_t *p = calloc(1,sizeof(*p));
      cells_to_be_activated_list_item->iE_Extensions = (struct F1AP_ProtocolExtensionContainer *) p;
      //F1AP_ProtocolExtensionContainer_154P112_t
      asn1cSequenceAdd(p->list, F1AP_Cells_to_be_Activated_List_ItemExtIEs_t,  cells_to_be_activated_itemExtIEs);
      cells_to_be_activated_itemExtIEs->id                     = F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation;
      cells_to_be_activated_itemExtIEs->criticality            = F1AP_Criticality_reject;
      cells_to_be_activated_itemExtIEs->extensionValue.present = F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation;

      if (f1ap_gnb_cu_configuration_update->cells_to_activate[i].num_SI > 0) {
        F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation =
          &cells_to_be_activated_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation;
        //LOG_I(F1AP, "%s() SI %d size %d: ", __func__, i, f1ap_setup_resp->SI_container_length[i][0]);
        //for (int n = 0; n < f1ap_setup_resp->SI_container_length[i][0]; n++)
        //  printf("%02x ", f1ap_setup_resp->SI_container[i][0][n]);
        //printf("\n");

        // for (int sIBtype=2;sIBtype<33;sIBtype++) { //21 ? 33 ?
        for (int sIBtype=2; sIBtype<21; sIBtype++) {
          if (f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_container[sIBtype]!=NULL) {
            AssertFatal(sIBtype < 6 || sIBtype == 9, "Illegal SI type %d\n",sIBtype);
            asn1cSequenceAdd(gNB_CUSystemInformation->sibtypetobeupdatedlist.list, F1AP_SibtypetobeupdatedListItem_t, sib_item);
            sib_item->sIBtype = sIBtype;
            OCTET_STRING_fromBuf(&sib_item->sIBmessage,
                                 (const char *)f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_container[sIBtype],
                                 f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_container_length[sIBtype]);
            LOG_D(F1AP, "f1ap_setup_resp->SI_container_length[%d][%d] = %d \n", i,sIBtype,
                  f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_container_length[sIBtype]);
          }
        }
      }
    }
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 gNB-CU CONFIGURATION UPDATE\n");
    return -1;
  }

  LOG_DUMPMSG(F1AP, LOG_DUMP_CHAR, buffer, len, "F1AP gNB-CU CONFIGURATION UPDATE : ");
  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_handle_gNB_CU_CONFIGURATION_UPDATE_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                                      sctp_assoc_t assoc_id,
                                                      uint32_t stream,
                                                      F1AP_F1AP_PDU_t *pdu)
{
  LOG_I(F1AP,"Cell Configuration ok (assoc_id %d)\n",assoc_id);
  return(0);
}

int CU_handle_gNB_DU_RESOURCE_COORDINATION_REQUEST(instance_t instance,
                                                   sctp_assoc_t assoc_id,
                                                   uint32_t stream,
                                                   F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(0, "Not implemented yet\n");
}

int CU_send_gNB_DU_RESOURCE_COORDINATION_RESPONSE(sctp_assoc_t assoc_id,
    F1AP_GNBDUResourceCoordinationResponse_t *GNBDUResourceCoordinationResponse) {
  AssertFatal(0, "Not implemented yet\n");
}
