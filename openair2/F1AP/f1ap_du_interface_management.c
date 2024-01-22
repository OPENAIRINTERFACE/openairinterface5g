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

/*! \file f1ap_du_interface_management.c
 * \brief f1ap interface management for DU
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
#include "f1ap_du_interface_management.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"
#include "assertions.h"

#include "GNB_APP/gnb_paramdef.h"

int to_NRNRB(int nrb) {
  for (int i=0; i<sizeofArray(nrb_lut); i++)
    if (nrb_lut[i] == nrb)
      return i;

  if(!RC.nrrrc)
    return 0;

  AssertFatal(1==0,"nrb %d is not in the list of possible NRNRB\n",nrb);
}

int DU_handle_RESET(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_send_RESET_ACKKNOWLEDGE(sctp_assoc_t assoc_id, F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_send_RESET(sctp_assoc_t assoc_id, F1AP_Reset_t *Reset)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_handle_RESET_ACKNOWLEDGE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
    Error Indication
*/

int DU_send_ERROR_INDICATION(sctp_assoc_t assoc_id, F1AP_F1AP_PDU_t *pdu_p)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_handle_ERROR_INDICATION(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
    F1 Setup
*/

// SETUP REQUEST
int DU_send_F1_SETUP_REQUEST(sctp_assoc_t assoc_id, f1ap_setup_req_t *setup_req)
{
  F1AP_F1AP_PDU_t       pdu= {0};
  uint8_t  *buffer;
  uint32_t  len;
  /* Create */
  /* 0. pdu Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, initMsg);
  initMsg->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  initMsg->criticality   = F1AP_Criticality_reject;
  initMsg->value.present = F1AP_InitiatingMessage__value_PR_F1SetupRequest;
  F1AP_F1SetupRequest_t *f1Setup = &initMsg->value.choice.F1SetupRequest;
  /* mandatory */
  /* c1. Transaction ID (integer value) */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieC1);
  ieC1->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ieC1->criticality               = F1AP_Criticality_reject;
  ieC1->value.present             = F1AP_F1SetupRequestIEs__value_PR_TransactionID;
  ieC1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);
  /* mandatory */
  /* c2. GNB_DU_ID (integer value) */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieC2);
  ieC2->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_ID;
  ieC2->criticality               = F1AP_Criticality_reject;
  ieC2->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_ID;
  asn_int642INTEGER(&ieC2->value.choice.GNB_DU_ID, setup_req->gNB_DU_id);

  /* optional */
  /* c3. GNB_DU_Name */
  if (setup_req->gNB_DU_name != NULL) {
    asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieC3);
    ieC3->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_Name;
    ieC3->criticality               = F1AP_Criticality_ignore;
    ieC3->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Name;
    char *gNB_DU_name=setup_req->gNB_DU_name;
    OCTET_STRING_fromBuf(&ieC3->value.choice.GNB_DU_Name, gNB_DU_name, strlen(gNB_DU_name));
  }

  /* mandatory */
  /* c4. served cells list */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieCells);
  ieCells->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_Served_Cells_List;
  ieCells->criticality               = F1AP_Criticality_reject;
  ieCells->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Served_Cells_List;
  int num_cells_available = setup_req->num_cells_available;
  LOG_D(F1AP, "num_cells_available = %d \n", num_cells_available);

  for (int i=0; i<num_cells_available; i++) {
    /* mandatory */
    /* 4.1 served cells item */
    f1ap_served_cell_info_t *cell = &setup_req->cell[i].info;
    asn1cSequenceAdd(ieCells->value.choice.GNB_DU_Served_Cells_List.list,
                     F1AP_GNB_DU_Served_Cells_ItemIEs_t, duServedCell);
    duServedCell->id = F1AP_ProtocolIE_ID_id_GNB_DU_Served_Cells_Item;
    duServedCell->criticality = F1AP_Criticality_reject;
    duServedCell->value.present = F1AP_GNB_DU_Served_Cells_ItemIEs__value_PR_GNB_DU_Served_Cells_Item;
    F1AP_GNB_DU_Served_Cells_Item_t  *gnb_du_served_cells_item=&duServedCell->value.choice.GNB_DU_Served_Cells_Item;
    /* 4.1.1 served cell Information */
    F1AP_Served_Cell_Information_t *served_cell_information= &gnb_du_served_cells_item->served_Cell_Information;
    addnRCGI(served_cell_information->nRCGI, cell);
    /* - nRPCI */
    served_cell_information->nRPCI = cell->nr_pci;  // int 0..1007
    /* - fiveGS_TAC */
    if (cell->tac != NULL) {
      uint32_t tac=htonl(*cell->tac);
      asn1cCalloc(served_cell_information->fiveGS_TAC, netOrder);
      OCTET_STRING_fromBuf(netOrder, ((char *)&tac)+1, 3);
    }

    /* - Configured_EPS_TAC */
    if(0) {
      served_cell_information->configured_EPS_TAC = (F1AP_Configured_EPS_TAC_t *)calloc(1, sizeof(F1AP_Configured_EPS_TAC_t));
      OCTET_STRING_fromBuf(served_cell_information->configured_EPS_TAC, "2", 2);
    }

    /* servedPLMN information */
    asn1cSequenceAdd(served_cell_information->servedPLMNs.list, F1AP_ServedPLMNs_Item_t,servedPLMN_item);
    MCC_MNC_TO_PLMNID(cell->plmn.mcc, cell->plmn.mnc, cell->plmn.mnc_digit_length, &servedPLMN_item->pLMN_Identity);
    // // /* - CHOICE NR-MODE-Info */
    F1AP_NR_Mode_Info_t *nR_Mode_Info= &served_cell_information->nR_Mode_Info;
    F1AP_ProtocolExtensionContainer_10696P34_t *p = calloc(1, sizeof(*p));
    servedPLMN_item->iE_Extensions = (struct F1AP_ProtocolExtensionContainer *) p;
    asn1cSequenceAdd(p->list, F1AP_ServedPLMNs_ItemExtIEs_t , served_plmns_itemExtIEs);
    served_plmns_itemExtIEs->criticality = F1AP_Criticality_ignore;
    served_plmns_itemExtIEs->id = F1AP_ProtocolIE_ID_id_TAISliceSupportList;
    served_plmns_itemExtIEs->extensionValue.present = F1AP_ServedPLMNs_ItemExtIEs__extensionValue_PR_SliceSupportList;
    F1AP_SliceSupportList_t *slice_support_list = &served_plmns_itemExtIEs->extensionValue.choice.SliceSupportList;

    /* get list of sst/sd from configuration file */
    paramdef_t SNSSAIParams[] = GNBSNSSAIPARAMS_DESC;
    paramlist_def_t SNSSAIParamList = {GNB_CONFIG_STRING_SNSSAI_LIST, NULL, 0};
    char sstr[100];
    /* TODO: be sure that %d in the line below is at the right place */
    sprintf(sstr, "%s.[%d].%s.[0]", GNB_CONFIG_STRING_GNB_LIST, i, GNB_CONFIG_STRING_PLMN_LIST);
    config_getlist(config_get_if(), &SNSSAIParamList, SNSSAIParams, sizeofArray(SNSSAIParams), sstr);
    AssertFatal(SNSSAIParamList.numelt > 0, "no slice configuration found (snssaiList in the configuration file)\n");
    AssertFatal(SNSSAIParamList.numelt <= 1024, "maximum size for slice support list is 1024, see F1AP 38.473 9.3.1.37\n");
    for (int s = 0; s < SNSSAIParamList.numelt; s++) {
      uint32_t sst;
      uint32_t sd;
      bool has_sd;
      sst = *SNSSAIParamList.paramarray[s][GNB_SLICE_SERVICE_TYPE_IDX].uptr;
      has_sd = *SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr != 0xffffff;
      asn1cSequenceAdd(slice_support_list->list, F1AP_SliceSupportItem_t, slice);
      INT8_TO_OCTET_STRING(sst, &slice->sNSSAI.sST);
      if (has_sd) {
        sd = *SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr;
        asn1cCalloc(slice->sNSSAI.sD, tmp);
        INT24_TO_OCTET_STRING(sd, tmp);
      }
    }

    if (setup_req->cell[i].info.mode == F1AP_MODE_FDD) { // FDD
      const f1ap_fdd_info_t *fdd = &setup_req->cell[i].info.fdd;
      nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_fDD;
      asn1cCalloc(nR_Mode_Info->choice.fDD, fDD_Info);
      /* FDD.1 UL NRFreqInfo */
      /* FDD.1.1 UL NRFreqInfo ARFCN */
      fDD_Info->uL_NRFreqInfo.nRARFCN = fdd->ul_freqinfo.arfcn; // Integer

      /* FDD.1.2 F1AP_SUL_Information */

      /* FDD.1.3 freqBandListNr */
      int fdd_ul_num_available_freq_Bands = 1;
      for (int fdd_ul_j=0; fdd_ul_j<fdd_ul_num_available_freq_Bands; fdd_ul_j++) {
        asn1cSequenceAdd(fDD_Info->uL_NRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
        /* FDD.1.3.1 freqBandIndicatorNr*/
        nr_freqBandNrItem->freqBandIndicatorNr = fdd->ul_freqinfo.band;

        /* FDD.1.3.2 supportedSULBandList*/
      } // for FDD : UL freq_Bands

      /* FDD.2 DL NRFreqInfo */
      /* FDD.2.1 DL NRFreqInfo ARFCN */
      fDD_Info->dL_NRFreqInfo.nRARFCN = fdd->dl_freqinfo.arfcn; // Integer

      /* FDD.2.2 F1AP_SUL_Information */

      /* FDD.2.3 freqBandListNr */
      int fdd_dl_num_available_freq_Bands = 1;
      for (int fdd_dl_j=0; fdd_dl_j<fdd_dl_num_available_freq_Bands; fdd_dl_j++) {
        asn1cSequenceAdd(fDD_Info->dL_NRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
        /* FDD.2.3.1 freqBandIndicatorNr*/
        nr_freqBandNrItem->freqBandIndicatorNr = fdd->dl_freqinfo.band;

        /* FDD.2.3.2 supportedSULBandList*/
      } // for FDD : DL freq_Bands

      /* FDD.3 UL Transmission Bandwidth */
      fDD_Info->uL_Transmission_Bandwidth.nRSCS = fdd->ul_tbw.scs;
      fDD_Info->uL_Transmission_Bandwidth.nRNRB = to_NRNRB(fdd->ul_tbw.nrb);
      /* FDD.4 DL Transmission Bandwidth */
      fDD_Info->dL_Transmission_Bandwidth.nRSCS = fdd->dl_tbw.scs;
      fDD_Info->dL_Transmission_Bandwidth.nRNRB = to_NRNRB(fdd->dl_tbw.nrb);
    } else if (setup_req->cell[i].info.mode == F1AP_MODE_TDD) { // TDD
      const f1ap_tdd_info_t *tdd = &setup_req->cell[i].info.tdd;
      nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_tDD;
      asn1cCalloc(nR_Mode_Info->choice.tDD, tDD_Info);
      /* TDD.1 nRFreqInfo */
      /* TDD.1.1 nRFreqInfo ARFCN */
      tDD_Info->nRFreqInfo.nRARFCN = tdd->freqinfo.arfcn; // Integer

      /* TDD.1.2 F1AP_SUL_Information */

      /* TDD.1.3 freqBandListNr */
      int tdd_num_available_freq_Bands = 1;
      for (int j=0; j<tdd_num_available_freq_Bands; j++) {
        asn1cSequenceAdd(tDD_Info->nRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
        /* TDD.1.3.1 freqBandIndicatorNr*/
        nr_freqBandNrItem->freqBandIndicatorNr = tdd->freqinfo.band;
        /* TDD.1.3.2 supportedSULBandList*/
      } // for TDD : freq_Bands

      /* TDD.2 transmission_Bandwidth */
      tDD_Info->transmission_Bandwidth.nRSCS = tdd->tbw.scs;
      tDD_Info->transmission_Bandwidth.nRNRB = to_NRNRB(tdd->tbw.nrb);
    } else {
      AssertFatal(false, "unknown mode %d\n", setup_req->cell[i].info.mode);
    }

    /* - measurementTimingConfiguration */
    char *measurementTimingConfiguration = cell->measurement_timing_information; // sept. 2018
    OCTET_STRING_fromBuf(&served_cell_information->measurementTimingConfiguration,
                         measurementTimingConfiguration,
                         strlen(measurementTimingConfiguration));

    /* 4.1.2 gNB-DU System Information */
    if (setup_req->cell[i].sys_info != NULL) {
      asn1cCalloc(gnb_du_served_cells_item->gNB_DU_System_Information, gNB_DU_System_Information);
      const f1ap_gnb_du_system_info_t *sys_info = setup_req->cell[i].sys_info;
      AssertFatal(sys_info->mib != NULL, "MIB must be present in DU sys info\n");
      OCTET_STRING_fromBuf(&gNB_DU_System_Information->mIB_message, (const char *)sys_info->mib, sys_info->mib_length);
      AssertFatal(sys_info->sib1 != NULL, "SIB1 must be present in DU sys info\n");
      OCTET_STRING_fromBuf(&gNB_DU_System_Information->sIB1_message, (const char *)sys_info->sib1, sys_info->sib1_length);
    }
  }

  /* mandatory */
  /* c5. RRC VERSION */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_GNB_DU_RRC_Version;
  ie2->criticality = F1AP_Criticality_reject;
  ie2->value.present = F1AP_F1SetupRequestIEs__value_PR_RRC_Version;
  // RRC Version: "This IE is not used in this release."
  // we put one bit for each byte in rrc_ver that is != 0
  uint8_t bits = 0;
  for (int i = 0; i < 3; ++i)
    bits |= (setup_req->rrc_ver[i] != 0) << i;
  BIT_STRING_t *bs = &ie2->value.choice.RRC_Version.latest_RRC_Version;
  bs->buf = calloc(1, sizeof(char));
  AssertFatal(bs->buf != NULL, "out of memory\n");
  bs->buf[0] = bits;
  bs->size = 1;
  bs->bits_unused = 5;

  F1AP_ProtocolExtensionContainer_10696P228_t *p = calloc(1, sizeof(F1AP_ProtocolExtensionContainer_10696P228_t));
  asn1cSequenceAdd(p->list, F1AP_RRC_Version_ExtIEs_t, rrcv_ext);
  rrcv_ext->id = F1AP_ProtocolIE_ID_id_latest_RRC_Version_Enhanced;
  rrcv_ext->criticality = F1AP_Criticality_ignore;
  rrcv_ext->extensionValue.present = F1AP_RRC_Version_ExtIEs__extensionValue_PR_OCTET_STRING_SIZE_3_;
  OCTET_STRING_t *os = &rrcv_ext->extensionValue.choice.OCTET_STRING_SIZE_3_;
  os->size = 3;
  os->buf = malloc(3 * sizeof(*os->buf));
  AssertFatal(os->buf != NULL, "out of memory\n");
  for (int i = 0; i < 3; ++i)
    os->buf[i] = setup_req->rrc_ver[i];
  ie2->value.choice.RRC_Version.iE_Extensions = (struct F1AP_ProtocolExtensionContainer*)p;
  
  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup request\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_handle_F1_SETUP_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "DU_handle_F1_SETUP_RESPONSE\n");
  AssertFatal(pdu->present == F1AP_F1AP_PDU_PR_successfulOutcome,
              "pdu->present != F1AP_F1AP_PDU_PR_successfulOutcome\n");
  AssertFatal(pdu->choice.successfulOutcome->procedureCode  == F1AP_ProcedureCode_id_F1Setup,
              "pdu->choice.successfulOutcome->procedureCode != F1AP_ProcedureCode_id_F1Setup\n");
  AssertFatal(pdu->choice.successfulOutcome->criticality  == F1AP_Criticality_reject,
              "pdu->choice.successfulOutcome->criticality != F1AP_Criticality_reject\n");
  AssertFatal(pdu->choice.successfulOutcome->value.present  == F1AP_SuccessfulOutcome__value_PR_F1SetupResponse,
              "pdu->choice.successfulOutcome->value.present != F1AP_SuccessfulOutcome__value_PR_F1SetupResponse\n");
  F1AP_F1SetupResponse_t    *in = &pdu->choice.successfulOutcome->value.choice.F1SetupResponse;
  F1AP_F1SetupResponseIEs_t *ie;
  int TransactionId = -1;
  int num_cells_to_activate = 0;
  f1ap_setup_resp_t resp = {0};

  for (int i=0; i < in->protocolIEs.list.count; i++) {
    ie = in->protocolIEs.list.array[i];

    switch (ie->id) {
      case F1AP_ProtocolIE_ID_id_TransactionID:
        AssertFatal(ie->criticality == F1AP_Criticality_reject,
                    "ie->criticality != F1AP_Criticality_reject\n");
        AssertFatal(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_TransactionID,
                    "ie->value.present != F1AP_F1SetupResponseIEs__value_PR_TransactionID\n");
        TransactionId=ie->value.choice.TransactionID;
        LOG_D(F1AP, "F1AP: F1Setup-Resp: TransactionId %d\n",
              TransactionId);
        break;

      case F1AP_ProtocolIE_ID_id_gNB_CU_Name:
        AssertFatal(ie->criticality == F1AP_Criticality_ignore,
                    "ie->criticality != F1AP_Criticality_ignore\n");
        AssertFatal(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_GNB_CU_Name,
                    "ie->value.present != F1AP_F1SetupResponseIEs__value_PR_GNB_CU_Name\n");
        resp.gNB_CU_name = malloc(ie->value.choice.GNB_CU_Name.size+1);
        memcpy(resp.gNB_CU_name, ie->value.choice.GNB_CU_Name.buf, ie->value.choice.GNB_CU_Name.size);
        resp.gNB_CU_name[ie->value.choice.GNB_CU_Name.size] = '\0';
        LOG_D(F1AP, "F1AP: F1Setup-Resp: gNB_CU_name %s\n", resp.gNB_CU_name);
        break;

      case F1AP_ProtocolIE_ID_id_GNB_CU_RRC_Version:
        AssertFatal(ie->criticality == F1AP_Criticality_reject,
                    "ie->criticality != F1AP_Criticality_reject\n");
        AssertFatal(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_RRC_Version,
                    "ie->value.present != F1AP_F1SetupResponseIEs__value_PR_RRC_Version\n");
        // RRC Version: "This IE is not used in this release."
        if(ie->value.choice.RRC_Version.iE_Extensions) {
          F1AP_ProtocolExtensionContainer_10696P228_t *ext =
              (F1AP_ProtocolExtensionContainer_10696P228_t *)ie->value.choice.RRC_Version.iE_Extensions;
          if(ext->list.count > 0){
            F1AP_RRC_Version_ExtIEs_t *rrcext = ext->list.array[0];
            OCTET_STRING_t *os = &rrcext->extensionValue.choice.OCTET_STRING_SIZE_3_;
            for (int i = 0; i < 3; i++)
              resp.rrc_ver[i] = os->buf[i];
          }
        }
        break;

      case F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List: {
        AssertFatal(ie->criticality == F1AP_Criticality_reject,
                    "ie->criticality != F1AP_Criticality_reject\n");
        AssertFatal(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_Cells_to_be_Activated_List,
                    "ie->value.present != F1AP_F1SetupResponseIEs__value_PR_Cells_to_be_Activated_List\n");
        num_cells_to_activate = ie->value.choice.Cells_to_be_Activated_List.list.count;
        LOG_D(F1AP, "F1AP: Activating %d cells\n",num_cells_to_activate);

        for (int i=0; i<num_cells_to_activate; i++) {
          F1AP_Cells_to_be_Activated_List_ItemIEs_t *cells_to_be_activated_list_item_ies = (F1AP_Cells_to_be_Activated_List_ItemIEs_t *) ie->value.choice.Cells_to_be_Activated_List.list.array[i];
          AssertFatal(cells_to_be_activated_list_item_ies->id == F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item,
                      "cells_to_be_activated_list_item_ies->id != F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item");
          AssertFatal(cells_to_be_activated_list_item_ies->criticality == F1AP_Criticality_reject,
                      "cells_to_be_activated_list_item_ies->criticality == F1AP_Criticality_reject");
          AssertFatal(cells_to_be_activated_list_item_ies->value.present == F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item,
                      "cells_to_be_activated_list_item_ies->value.present == F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item");
          F1AP_Cells_to_be_Activated_List_Item_t *cell =
              &cells_to_be_activated_list_item_ies->value.choice.Cells_to_be_Activated_List_Item;
          TBCD_TO_MCC_MNC(&cell->nRCGI.pLMN_Identity,
                          resp.cells_to_activate[i].plmn.mcc,
                          resp.cells_to_activate[i].plmn.mnc,
                          resp.cells_to_activate[i].plmn.mnc_digit_length);
          LOG_D(F1AP, "nr_cellId : %x %x %x %x %x\n",
                cell->nRCGI.nRCellIdentity.buf[0],
                cell->nRCGI.nRCellIdentity.buf[1],
                cell->nRCGI.nRCellIdentity.buf[2],
                cell->nRCGI.nRCellIdentity.buf[3],
                cell->nRCGI.nRCellIdentity.buf[4]);
          BIT_STRING_TO_NR_CELL_IDENTITY(&cell->nRCGI.nRCellIdentity, resp.cells_to_activate[i].nr_cellid);
          if (cell->nRPCI != NULL)
            resp.cells_to_activate[i].nrpci = *cell->nRPCI;
          F1AP_ProtocolExtensionContainer_10696P112_t *ext = (F1AP_ProtocolExtensionContainer_10696P112_t *)cell->iE_Extensions;

          if (ext==NULL)
            continue;

          for (int cnt=0; cnt<ext->list.count; cnt++) {
            F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *cells_to_be_activated_list_itemExtIEs=(F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *)ext->list.array[cnt];

            switch (cells_to_be_activated_list_itemExtIEs->id) {
              /*
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_NOTHING:
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_AvailablePLMNList,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_ExtendedAvailablePLMN_List,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_IAB_Info_IAB_donor_CU,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_AvailableSNPN_ID_List
              */
              case F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation: {
                resp.cells_to_activate[i].nrpci = (cell->nRPCI != NULL) ? *cell->nRPCI : 0;
                F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation = (F1AP_GNB_CUSystemInformation_t *)&cells_to_be_activated_list_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation;
                resp.cells_to_activate[i].num_SI = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count;
                AssertFatal(ext->list.count==1,"At least one SI message should be there, and only 1 for now!\n");
                LOG_D(F1AP,
                      "F1AP: Cell %d MCC %d MNC %d NRCellid %lx num_si %d\n",
                      i,
                      resp.cells_to_activate[i].plmn.mcc,
                      resp.cells_to_activate[i].plmn.mnc,
                      resp.cells_to_activate[i].nr_cellid,
                      resp.cells_to_activate[i].num_SI);

                for (int si = 0; si < gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count; si++) {
                  F1AP_SibtypetobeupdatedListItem_t *sib_item = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.array[si];
                  size_t size = sib_item->sIBmessage.size;
                  resp.cells_to_activate[i].SI_container_length[si] = size;
                  LOG_D(F1AP, "F1AP: SI_container_length[%d][%ld] %ld bytes\n", i, sib_item->sIBtype, size);
                  resp.cells_to_activate[i].SI_container[si] = malloc(resp.cells_to_activate[i].SI_container_length[si]);
                  memcpy(resp.cells_to_activate[i].SI_container[si], sib_item->sIBmessage.buf, size);
                  resp.cells_to_activate[i].SI_type[si]=sib_item->sIBtype;
                }

                break;
              }

              case F1AP_ProtocolIE_ID_id_AvailablePLMNList:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_ExtendedAvailablePLMN_List:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_IAB_Info_IAB_donor_CU:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_AvailableSNPN_ID_List:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              default:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id %d unknown\n",(int)cells_to_be_activated_list_itemExtIEs->id);
                break;
            }
          } // for (cnt=...
        } // for (cells_to_activate...

        break;
      } // case F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List

      default:
        AssertFatal(1==0,"F1AP_ProtocolIE_ID_id %d unknown\n", (int)ie->id);
        break;
    } // switch ie
  } // for IE

  AssertFatal(TransactionId!=-1,"TransactionId was not sent\n");
  LOG_D(F1AP,"F1AP: num_cells_to_activate %d\n",num_cells_to_activate);
  resp.num_cells_to_activate = num_cells_to_activate;

  // tmp
  for (int i=0; i<num_cells_to_activate; i++)
    AssertFatal(resp.cells_to_activate[i].num_SI == 0, "System Information handling not implemented");

  LOG_D(F1AP, "Sending F1AP_SETUP_RESP ITTI message\n");
  f1_setup_response(&resp);
  return 0;
}

// SETUP FAILURE
int DU_handle_F1_SETUP_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_F1SetupFailure_t    *out;
  F1AP_F1SetupFailureIEs_t *ie;
  f1ap_setup_failure_t fail = {0};
  out = &pdu->choice.unsuccessfulOutcome->value.choice.F1SetupFailure;
  /* Transaction ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupFailureIEs_t, ie, out, F1AP_ProtocolIE_ID_id_TransactionID, true);
  /* Cause */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupFailureIEs_t, ie, out,
                             F1AP_ProtocolIE_ID_id_Cause, true);

  if(0) {
    /* TimeToWait */
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupFailureIEs_t, ie, out,
                               F1AP_ProtocolIE_ID_id_TimeToWait, true);
  }

  f1_setup_failure(&fail);
  return 0;
}

/*
    gNB-DU Configuration Update
*/

int DU_send_gNB_DU_CONFIGURATION_UPDATE(sctp_assoc_t assoc_id, f1ap_setup_req_t *f1ap_setup_req)
{
  F1AP_F1AP_PDU_t                     pdu= {0};
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, initMsg);
  initMsg->procedureCode = F1AP_ProcedureCode_id_gNBDUConfigurationUpdate;
  initMsg->criticality   = F1AP_Criticality_reject;
  initMsg->value.present = F1AP_InitiatingMessage__value_PR_GNBDUConfigurationUpdate;
  F1AP_GNBDUConfigurationUpdate_t      *out = &pdu.choice.initiatingMessage->value.choice.GNBDUConfigurationUpdate;
  /* mandatory */
  /* c1. Transaction ID (integer value) */
  asn1cSequenceAdd(out, F1AP_GNBDUConfigurationUpdateIEs_t, ie1);
  ie1->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie1->criticality               = F1AP_Criticality_reject;
  ie1->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_TransactionID;
  ie1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);
  /* mandatory */
  /* c2. Served_Cells_To_Add */
  asn1cSequenceAdd(out, F1AP_GNBDUConfigurationUpdateIEs_t, ie2);
  ie2->id                        = F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_List;
  ie2->criticality               = F1AP_Criticality_reject;
  ie2->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Add_List;

  for (int j=0;   j<1; j++) {
    //
    asn1cSequenceAdd(ie2->value.choice.Served_Cells_To_Add_List.list, F1AP_Served_Cells_To_Add_ItemIEs_t, served_cells_to_add_item_ies);
    served_cells_to_add_item_ies->id            = F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_Item;
    served_cells_to_add_item_ies->criticality   = F1AP_Criticality_reject;
    served_cells_to_add_item_ies->value.present = F1AP_Served_Cells_To_Add_ItemIEs__value_PR_Served_Cells_To_Add_Item;
    F1AP_Served_Cells_To_Add_Item_t *served_cells_to_add_item= &served_cells_to_add_item_ies->value.choice.Served_Cells_To_Add_Item;
    F1AP_Served_Cell_Information_t  *served_cell_information=&served_cells_to_add_item->served_Cell_Information;
    /* - nRCGI */
    addnRCGI(served_cell_information->nRCGI, &f1ap_setup_req->cell[j].info);
    /* - nRPCI */
    /* 2.1.1 serverd cell Information */
    f1ap_served_cell_info_t *cell = &f1ap_setup_req->cell[j].info;
    served_cell_information->nRPCI = cell->nr_pci;  // int 0..1007
    /* - fiveGS_TAC */
    if (cell->tac != NULL) {
      uint32_t tac = htonl(*cell->tac);
      served_cell_information->fiveGS_TAC = calloc(1, sizeof(*served_cell_information->fiveGS_TAC));
      OCTET_STRING_fromBuf(served_cell_information->fiveGS_TAC, ((char *)&tac)+1, 3);
    }

    /* - Configured_EPS_TAC */
    if(1) {
      served_cell_information->configured_EPS_TAC = (F1AP_Configured_EPS_TAC_t *)calloc(1, sizeof(F1AP_Configured_EPS_TAC_t));
      OCTET_STRING_fromBuf(served_cell_information->configured_EPS_TAC,"2", 2);
    }

    asn1cSequenceAdd(served_cell_information->servedPLMNs.list, F1AP_ServedPLMNs_Item_t, servedPLMN_item);
    MCC_MNC_TO_PLMNID(cell->plmn.mcc, cell->plmn.mnc, cell->plmn.mnc_digit_length, &servedPLMN_item->pLMN_Identity);
    // // /* - CHOICE NR-MODE-Info */
    F1AP_NR_Mode_Info_t *nR_Mode_Info=&served_cell_information->nR_Mode_Info;
    LOG_E(F1AP,"Here hardcoded values instead of values from configuration file\n");

    if (cell->mode == F1AP_MODE_FDD) {
      nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_fDD;
      /* > FDD >> FDD Info */
      asn1cCalloc(nR_Mode_Info->choice.fDD, fDD_Info);
      /* >>> UL NRFreqInfo */
      fDD_Info->uL_NRFreqInfo.nRARFCN = 999L;
      asn1cSequenceAdd(fDD_Info->uL_NRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, ul_freqBandNrItem);
      ul_freqBandNrItem->freqBandIndicatorNr = 888L;
      asn1cSequenceAdd(ul_freqBandNrItem->supportedSULBandList.list, F1AP_SupportedSULFreqBandItem_t, ul_supportedSULFreqBandItem);
      ul_supportedSULFreqBandItem->freqBandIndicatorNr = 777L;
      /* >>> DL NRFreqInfo */
      fDD_Info->dL_NRFreqInfo.nRARFCN = 666L;
      asn1cSequenceAdd(fDD_Info->dL_NRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, dl_freqBandNrItem);
      dl_freqBandNrItem->freqBandIndicatorNr = 555L;
      asn1cSequenceAdd(dl_freqBandNrItem->supportedSULBandList.list, F1AP_SupportedSULFreqBandItem_t, dl_supportedSULFreqBandItem);
      dl_supportedSULFreqBandItem->freqBandIndicatorNr = 444L;
      /* >>> UL Transmission Bandwidth */
      fDD_Info->uL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
      fDD_Info->uL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
      /* >>> DL Transmission Bandwidth */
      fDD_Info->dL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
      fDD_Info->dL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
    } else if (cell->mode == F1AP_MODE_TDD) { // TDD
      nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_tDD;
      /* > TDD >> TDD Info */
      asn1cCalloc(nR_Mode_Info->choice.tDD, tDD_Info);
      /* >>> ARFCN */
      tDD_Info->nRFreqInfo.nRARFCN = 999L; // Integer
      asn1cSequenceAdd(tDD_Info->nRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
      nr_freqBandNrItem->freqBandIndicatorNr = 555L;
      asn1cSequenceAdd(nr_freqBandNrItem->supportedSULBandList.list, F1AP_SupportedSULFreqBandItem_t, nr_supportedSULFreqBandItem);
      nr_supportedSULFreqBandItem->freqBandIndicatorNr = 444L;
      tDD_Info->transmission_Bandwidth.nRSCS= F1AP_NRSCS_scs15;
      tDD_Info->transmission_Bandwidth.nRNRB= F1AP_NRNRB_nrb11;
    } else {
      AssertFatal(false, "illegal mode %d\n", cell->mode);
    }

    /* - measurementTimingConfiguration */
    char *measurementTimingConfiguration = "0"; // sept. 2018
    OCTET_STRING_fromBuf(&served_cell_information->measurementTimingConfiguration,
                         measurementTimingConfiguration,
                         strlen(measurementTimingConfiguration));
    /* 2.1.2 gNB-DU System Information */
    asn1cCalloc(served_cells_to_add_item->gNB_DU_System_Information, gNB_DU_System_Information);
    OCTET_STRING_fromBuf(&gNB_DU_System_Information->mIB_message,  // sept. 2018
                         "1",
                         sizeof("1"));
    OCTET_STRING_fromBuf(&gNB_DU_System_Information->sIB1_message,  // sept. 2018
                         "1",
                         sizeof("1"));
  }

  /* mandatory */
  /* c3. Served_Cells_To_Modify */
  asn1cSequenceAdd(out, F1AP_GNBDUConfigurationUpdateIEs_t, ie3);
  ie3->id                        = F1AP_ProtocolIE_ID_id_Served_Cells_To_Modify_List;
  ie3->criticality               = F1AP_Criticality_reject;
  ie3->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Modify_List;

  for (int i=0; i<1; i++) {
    //
    f1ap_served_cell_info_t *cell = &f1ap_setup_req->cell[i].info;
    asn1cSequenceAdd(ie3->value.choice.Served_Cells_To_Modify_List.list, F1AP_Served_Cells_To_Modify_ItemIEs_t, served_cells_to_modify_item_ies);
    served_cells_to_modify_item_ies->id            = F1AP_ProtocolIE_ID_id_Served_Cells_To_Modify_Item;
    served_cells_to_modify_item_ies->criticality   = F1AP_Criticality_reject;
    served_cells_to_modify_item_ies->value.present = F1AP_Served_Cells_To_Modify_ItemIEs__value_PR_Served_Cells_To_Modify_Item;
    F1AP_Served_Cells_To_Modify_Item_t *served_cells_to_modify_item=&served_cells_to_modify_item_ies->value.choice.Served_Cells_To_Modify_Item;
    /* 3.1 oldNRCGI */
    //addnRGCI(served_cells_to_modify_item->oldNRCGI, f1ap_setup_req->cell[i]);
    /* 3.2.1 serverd cell Information */
    F1AP_Served_Cell_Information_t *served_cell_information= &served_cells_to_modify_item->served_Cell_Information;
    /* - nRCGI */
    //addnRGCI(served_cell_information->nRCGI,f1ap_setup_req->cell[i]);
    /* - nRPCI */
    served_cell_information->nRPCI = cell->nr_pci;  // int 0..1007
    /* - fiveGS_TAC */
    asn1cCalloc(served_cell_information->fiveGS_TAC, tac );
    OCTET_STRING_fromBuf(tac,
                         (const char *) &cell->tac,
                         3);

    /* - Configured_EPS_TAC */
    if(1) {
      asn1cCalloc(served_cell_information->configured_EPS_TAC, tmp2);
      OCTET_STRING_fromBuf(tmp2,
                           "2",
                           2);
    }

    asn1cSequenceAdd(served_cell_information->servedPLMNs.list, F1AP_ServedPLMNs_Item_t, servedPLMN_item);
    MCC_MNC_TO_PLMNID(cell->plmn.mcc, cell->plmn.mnc, cell->plmn.mnc_digit_length, &servedPLMN_item->pLMN_Identity);
    // // /* - CHOICE NR-MODE-Info */
    F1AP_NR_Mode_Info_t *nR_Mode_Info= &served_cell_information->nR_Mode_Info;

    if (cell->mode == F1AP_MODE_FDD) {
      nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_fDD;
      /* > FDD >> FDD Info */
      asn1cCalloc(nR_Mode_Info->choice.fDD, fDD_Info);
      /* >>> UL NRFreqInfo */
      fDD_Info->uL_NRFreqInfo.nRARFCN = 999L;
      asn1cSequenceAdd(fDD_Info->uL_NRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, ul_freqBandNrItem);
      ul_freqBandNrItem->freqBandIndicatorNr = 888L;
      asn1cSequenceAdd(ul_freqBandNrItem->supportedSULBandList.list, F1AP_SupportedSULFreqBandItem_t, ul_supportedSULFreqBandItem);
      ul_supportedSULFreqBandItem->freqBandIndicatorNr = 777L;
      /* >>> DL NRFreqInfo */
      fDD_Info->dL_NRFreqInfo.nRARFCN = 666L;
      asn1cSequenceAdd(dl_freqBandNrItem->supportedSULBandList.list, F1AP_FreqBandNrItem_t, dl_freqBandNrItem);
      dl_freqBandNrItem->freqBandIndicatorNr = 555L;
      F1AP_SupportedSULFreqBandItem_t dl_supportedSULFreqBandItem;
      memset((void *)&dl_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
      dl_supportedSULFreqBandItem.freqBandIndicatorNr = 444L;
      /* >>> UL Transmission Bandwidth */
      fDD_Info->uL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
      fDD_Info->uL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
      /* >>> DL Transmission Bandwidth */
      fDD_Info->dL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
      fDD_Info->dL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
    } else if (cell->mode == F1AP_MODE_TDD) { // TDD
      nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_tDD;
      /* > TDD >> TDD Info */
      asn1cCalloc(nR_Mode_Info->choice.tDD, tDD_Info);
      /* >>> ARFCN */
      tDD_Info->nRFreqInfo.nRARFCN = 999L; // Integer
      asn1cSequenceAdd(tDD_Info->nRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
      nr_freqBandNrItem->freqBandIndicatorNr = 555L;
      asn1cSequenceAdd(nr_freqBandNrItem->supportedSULBandList.list, F1AP_SupportedSULFreqBandItem_t, nr_supportedSULFreqBandItem);
      nr_supportedSULFreqBandItem->freqBandIndicatorNr = 444L;
      tDD_Info->transmission_Bandwidth.nRSCS= F1AP_NRSCS_scs15;
      tDD_Info->transmission_Bandwidth.nRNRB= F1AP_NRNRB_nrb11;
    } else {
      AssertFatal(false, "unknown mode %d\n", cell->mode);
    }

    /* - measurementTimingConfiguration */
    char *measurementTimingConfiguration = "0"; // sept. 2018
    OCTET_STRING_fromBuf(&served_cell_information->measurementTimingConfiguration,
                         measurementTimingConfiguration,
                         strlen(measurementTimingConfiguration));
    /* 3.2.2 gNB-DU System Information */
    asn1cCalloc( served_cells_to_modify_item->gNB_DU_System_Information, gNB_DU_System_Information);
    OCTET_STRING_fromBuf(&gNB_DU_System_Information->mIB_message,  // sept. 2018
                         "1",
                         sizeof("1"));
    OCTET_STRING_fromBuf(&gNB_DU_System_Information->sIB1_message,  // sept. 2018
                         "1",
                         sizeof("1"));
  }

  /* mandatory */
  /* c4. Served_Cells_To_Delete */
  asn1cSequenceAdd(out, F1AP_GNBDUConfigurationUpdateIEs_t, ie4);
  ie4->id                        = F1AP_ProtocolIE_ID_id_Served_Cells_To_Delete_List;
  ie4->criticality               = F1AP_Criticality_reject;
  ie4->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Delete_List;

  for (int i=0; i<1; i++) {
    //
    asn1cSequenceAdd(ie4->value.choice.Served_Cells_To_Delete_List.list, F1AP_Served_Cells_To_Delete_ItemIEs_t, served_cells_to_delete_item_ies);
    served_cells_to_delete_item_ies->id            = F1AP_ProtocolIE_ID_id_Served_Cells_To_Delete_Item;
    served_cells_to_delete_item_ies->criticality   = F1AP_Criticality_reject;
    served_cells_to_delete_item_ies->value.present = F1AP_Served_Cells_To_Delete_ItemIEs__value_PR_Served_Cells_To_Delete_Item;
    F1AP_Served_Cells_To_Delete_Item_t *served_cells_to_delete_item=&served_cells_to_delete_item_ies->value.choice.Served_Cells_To_Delete_Item;
    /* 3.1 oldNRCGI */
    addnRCGI(served_cells_to_delete_item->oldNRCGI, &f1ap_setup_req->cell[i].info);
  }

  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 gNB-DU CONFIGURATION UPDATE\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  return 0;
}

int DU_handle_gNB_DU_CONFIGURATION_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_handle_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                                      sctp_assoc_t assoc_id,
                                                      uint32_t stream,
                                                      F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int DU_handle_gNB_CU_CONFIGURATION_UPDATE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "DU_handle_gNB_CU_CONFIGURATION_UPDATE\n");
  AssertFatal(pdu->present == F1AP_F1AP_PDU_PR_initiatingMessage,
              "pdu->present != F1AP_F1AP_PDU_PR_initiatingMessage\n");
  AssertFatal(pdu->choice.initiatingMessage->procedureCode  == F1AP_ProcedureCode_id_gNBCUConfigurationUpdate,
              "pdu->choice.initiatingMessage->procedureCode != F1AP_ProcedureCode_id_gNBCUConfigurationUpdate\n");
  AssertFatal(pdu->choice.initiatingMessage->criticality  == F1AP_Criticality_reject,
              "pdu->choice.initiatingMessage->criticality != F1AP_Criticality_reject\n");
  AssertFatal(pdu->choice.initiatingMessage->value.present  == F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate,
              "pdu->choice.initiatingMessage->value.present != F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate\n");
  F1AP_GNBCUConfigurationUpdate_t *in = &pdu->choice.initiatingMessage->value.choice.GNBCUConfigurationUpdate;
  F1AP_GNBCUConfigurationUpdateIEs_t *ie;
  int TransactionId = -1;
  int num_cells_to_activate = 0;
  F1AP_Cells_to_be_Activated_List_Item_t *cell;
  MessageDef *msg_p = itti_alloc_new_message (TASK_DU_F1, 0, F1AP_GNB_CU_CONFIGURATION_UPDATE);
  LOG_D(F1AP, "F1AP: gNB_CU_Configuration_Update: protocolIEs.list.count %d\n",
        in->protocolIEs.list.count);

  for (int i=0; i < in->protocolIEs.list.count; i++) {
    ie = in->protocolIEs.list.array[i];

    switch (ie->id) {
      case F1AP_ProtocolIE_ID_id_TransactionID:
        AssertFatal(ie->criticality == F1AP_Criticality_reject,
                    "ie->criticality != F1AP_Criticality_reject\n");
        AssertFatal(ie->value.present == F1AP_GNBCUConfigurationUpdateIEs__value_PR_TransactionID,
                    "ie->value.present != F1AP_GNBCUConfigurationUpdateIEs__value_PR_TransactionID\n");
        TransactionId=ie->value.choice.TransactionID;
        LOG_D(F1AP, "F1AP: GNB-CU-ConfigurationUpdate: TransactionId %d\n",
              TransactionId);
        break;

      case F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List: {
        AssertFatal(ie->criticality == F1AP_Criticality_reject,
                    "ie->criticality != F1AP_Criticality_reject\n");
        AssertFatal(ie->value.present == F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Activated_List,
                    "ie->value.present != F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Activated_List\n");
        num_cells_to_activate = ie->value.choice.Cells_to_be_Activated_List.list.count;
        LOG_D(F1AP, "F1AP: Activating %d cells\n",num_cells_to_activate);

        for (int i=0; i<num_cells_to_activate; i++) {
          F1AP_Cells_to_be_Activated_List_ItemIEs_t *cells_to_be_activated_list_item_ies = (F1AP_Cells_to_be_Activated_List_ItemIEs_t *) ie->value.choice.Cells_to_be_Activated_List.list.array[i];
          AssertFatal(cells_to_be_activated_list_item_ies->id == F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item,
                      "cells_to_be_activated_list_item_ies->id != F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item");
          AssertFatal(cells_to_be_activated_list_item_ies->criticality == F1AP_Criticality_reject,
                      "cells_to_be_activated_list_item_ies->criticality == F1AP_Criticality_reject");
          AssertFatal(cells_to_be_activated_list_item_ies->value.present == F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item,
                      "cells_to_be_activated_list_item_ies->value.present == F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item");
          cell = &cells_to_be_activated_list_item_ies->value.choice.Cells_to_be_Activated_List_Item;
          TBCD_TO_MCC_MNC(&cell->nRCGI.pLMN_Identity,
                          F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mcc,
                          F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mnc,
                          F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mnc_digit_length);
          LOG_D(F1AP, "nr_cellId : %x %x %x %x %x\n",
                cell->nRCGI.nRCellIdentity.buf[0],
                cell->nRCGI.nRCellIdentity.buf[1],
                cell->nRCGI.nRCellIdentity.buf[2],
                cell->nRCGI.nRCellIdentity.buf[3],
                cell->nRCGI.nRCellIdentity.buf[4]);
          BIT_STRING_TO_NR_CELL_IDENTITY(&cell->nRCGI.nRCellIdentity,
                                         F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].nr_cellid);
          F1AP_ProtocolExtensionContainer_10696P112_t *ext = (F1AP_ProtocolExtensionContainer_10696P112_t *)cell->iE_Extensions;

          if (ext==NULL)
            continue;

          for (int cnt=0; cnt<ext->list.count; cnt++) {
            F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *cells_to_be_activated_list_itemExtIEs=(F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *)ext->list.array[cnt];

            switch (cells_to_be_activated_list_itemExtIEs->id) {
              /*
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_NOTHING:
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_AvailablePLMNList,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_ExtendedAvailablePLMN_List,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_IAB_Info_IAB_donor_CU,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_AvailableSNPN_ID_List
              */
              case F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation: {
                F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].nrpci = (cell->nRPCI != NULL) ? *cell->nRPCI : 0;
                F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation = (F1AP_GNB_CUSystemInformation_t *)&cells_to_be_activated_list_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation;
                F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].num_SI = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count;
                AssertFatal(ext->list.count==1,"At least one SI message should be there, and only 1 for now!\n");
                LOG_D(F1AP,
                      "F1AP: Cell %d MCC %d MNC %d NRCellid %lx num_si %d\n",
                      i,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mcc,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mnc,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].nr_cellid,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].num_SI);

                for (int si = 0; si < gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count; si++) {
                  F1AP_SibtypetobeupdatedListItem_t *sib_item = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.array[si];
                  size_t size = sib_item->sIBmessage.size;
                  F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].SI_container_length[si] = size;
                  LOG_D(F1AP, "F1AP: SI_container_length[%d][%ld] %ld bytes\n", i, sib_item->sIBtype, size);
                  F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].SI_container[si] = malloc(F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].SI_container_length[si]);
                  memcpy((void *)F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].SI_container[si],
                         (void *)sib_item->sIBmessage.buf,
                         size);
                  F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].SI_type[si]=sib_item->sIBtype;
                }

                break;
              }

              case F1AP_ProtocolIE_ID_id_AvailablePLMNList:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_ExtendedAvailablePLMN_List:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_IAB_Info_IAB_donor_CU:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_AvailableSNPN_ID_List:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              default:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id %d unknown\n",(int)cells_to_be_activated_list_itemExtIEs->id);
                break;
            }
          } // for (cnt=...
        } // for (cells_to_activate...

        break;
      } // case F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List

      default:
        AssertFatal(1==0,"F1AP_ProtocolIE_ID_id %d unknown\n", (int)ie->id);
        break;
    } // switch ie
  } // for IE

  AssertFatal(TransactionId!=-1,"TransactionId was not sent\n");
  LOG_D(F1AP,"F1AP: num_cells_to_activate %d\n",num_cells_to_activate);
  F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).num_cells_to_activate = num_cells_to_activate;
  LOG_D(F1AP, "Sending F1AP_GNB_CU_CONFIGURATION_UPDATE ITTI message \n");
  itti_send_msg_to_task(TASK_GNB_APP, GNB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);
  return 0;
}

int DU_send_gNB_CU_CONFIGURATION_UPDATE_FAILURE(sctp_assoc_t assoc_id,
    f1ap_gnb_cu_configuration_update_failure_t *GNBCUConfigurationUpdateFailure) {
  AssertFatal(1==0,"received gNB CU CONFIGURATION UPDATE FAILURE with cause %d\n",
              GNBCUConfigurationUpdateFailure->cause);
}

int DU_send_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(
    sctp_assoc_t assoc_id,
    f1ap_gnb_cu_configuration_update_acknowledge_t *GNBCUConfigurationUpdateAcknowledge)
{
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->num_cells_failed_to_be_activated == 0,
              "%d cells failed to activate\n",
              GNBCUConfigurationUpdateAcknowledge->num_cells_failed_to_be_activated);
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_to_setup == 0,
              "%d TNLAssociations to setup, handle this ...\n",
              GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_to_setup);
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_failed == 0,
              "%d TNLAssociations failed\n",
              GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_failed);
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->noofDedicatedSIDeliveryNeededUEs == 0,
              "%d DedicatedSIDeliveryNeededUEs\n",
              GNBCUConfigurationUpdateAcknowledge->noofDedicatedSIDeliveryNeededUEs);
  F1AP_F1AP_PDU_t           pdu= {0};
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. pdu Type */
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_gNBCUConfigurationUpdate;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_GNBCUConfigurationUpdateAcknowledge;
  F1AP_GNBCUConfigurationUpdateAcknowledge_t *out = &tmp->value.choice.GNBCUConfigurationUpdateAcknowledge;
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_GNBCUConfigurationUpdateAcknowledgeIEs_t, ie);
  ie->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateAcknowledgeIEs__value_PR_TransactionID;
  ie->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode GNB-DU-Configuration-Update-Acknowledge\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_send_gNB_DU_RESOURCE_COORDINATION_REQUEST(sctp_assoc_t assoc_id,
                                                 F1AP_GNBDUResourceCoordinationRequest_t *GNBDUResourceCoordinationRequest)
{
  AssertFatal(0, "Not implemented yet\n");
}

int DU_handle_gNB_DU_RESOURCE_COORDINATION_RESPONSE(instance_t instance,
                                                    sctp_assoc_t assoc_id,
                                                    uint32_t stream,
                                                    F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(0, "Not implemented yet\n");
}
