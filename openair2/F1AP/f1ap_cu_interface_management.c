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
#include "f1ap_decoder.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_interface_management.h"

int CU_send_RESET(instance_t instance, F1AP_Reset_t *Reset) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_RESET_ACKKNOWLEDGE(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_RESET(instance_t instance,
                    uint32_t assoc_id,
                    uint32_t stream,
                    F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_RESET_ACKNOWLEDGE(instance_t instance, F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
    Error Indication
*/
int CU_handle_ERROR_INDICATION(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_ERROR_INDICATION(instance_t instance, F1AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
    F1 Setup
*/
int CU_handle_F1_SETUP_REQUEST(instance_t instance,
                               uint32_t assoc_id,
                               uint32_t stream,
                               F1AP_F1AP_PDU_t *pdu) {
  LOG_D(F1AP, "CU_handle_F1_SETUP_REQUEST\n");
  MessageDef                         *message_p;
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

  /* assoc_id */
  f1ap_setup_req_t *req=&getCxt(true, instance)->setupReq;
  req->assoc_id = assoc_id;
  /* gNB_DU_id */
  // this function exits if the ie is mandatory
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_ID, true);
  asn_INTEGER2ulong(&ie->value.choice.GNB_DU_ID, &req->gNB_DU_id);
  LOG_D(F1AP, "req->gNB_DU_id %lu \n", req->gNB_DU_id);
  /* gNB_DU_name */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_Name, true);
  req->gNB_DU_name = calloc(ie->value.choice.GNB_DU_Name.size + 1, sizeof(char));
  memcpy(req->gNB_DU_name, ie->value.choice.GNB_DU_Name.buf,
         ie->value.choice.GNB_DU_Name.size);
  LOG_D(F1AP, "req->gNB_DU_name %s \n", req->gNB_DU_name);
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
      OCTET_STRING_TO_INT16(servedCellInformation->fiveGS_TAC, req->cell[i].tac);
      LOG_D(F1AP, "req->tac[%d] %d \n", i, req->cell[i].tac);
    }
    
    /* - nRCGI */
    TBCD_TO_MCC_MNC(&(servedCellInformation->nRCGI.pLMN_Identity), req->cell[i].mcc,
                    req->cell[i].mnc,req->cell[i].mnc_digit_length);
    // NR cellID
    BIT_STRING_TO_NR_CELL_IDENTITY(&servedCellInformation->nRCGI.nRCellIdentity,
                                   req->cell[i].nr_cellid);
    LOG_D(F1AP, "[SCTP %d] Received nRCGI: MCC %d, MNC %d, CELL_ID %llu\n", assoc_id,
          req->cell[i].mcc,
          req->cell[i].mnc,
          (long long unsigned int)req->cell[i].nr_cellid);
    /* - nRPCI */
    req->cell[i].nr_pci = servedCellInformation->nRPCI;
    LOG_D(F1AP, "req->nr_pci[%d] %d \n", i, req->cell[i].nr_pci);
    
    // LTS: FIXME data model failure: we don't KNOW if we receive a 4G or a 5G cell
    // Furthermore, cell_type is not a attribute of a cell in the data structure !!!!!!!!!!
    if (RC.nrrrc && RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)]->node_type == ngran_gNB_CU)
      f1ap_req(true, instance)->cell_type=CELL_MACRO_GNB;
    else
      f1ap_req(true, instance)->cell_type=CELL_MACRO_ENB;
    
    // FDD Cells
    if (servedCellInformation->nR_Mode_Info.present==F1AP_NR_Mode_Info_PR_fDD) {
      struct fdd_s *FDDs=&req->nr_mode_info[i].fdd;
      F1AP_FDD_Info_t * fDD_Info=servedCellInformation->nR_Mode_Info.choice.fDD;
      FDDs->ul_nr_arfcn=fDD_Info->uL_NRFreqInfo.nRARFCN;
      FDDs->ul_num_frequency_bands=fDD_Info->uL_NRFreqInfo.freqBandListNr.list.count;
      for (int f=0; f < fDD_Info->uL_NRFreqInfo.freqBandListNr.list.count; f++) {
	F1AP_FreqBandNrItem_t * FreqItem=fDD_Info->uL_NRFreqInfo.freqBandListNr.list.array[f];
	FDDs->ul_nr_band[f]=FreqItem->freqBandIndicatorNr;
	int num_available_supported_SULBands=FreqItem->supportedSULBandList.list.count;
	for (int sul=0; sul < num_available_supported_SULBands; sul ++ ) {
          F1AP_SupportedSULFreqBandItem_t * SulItem= FreqItem->supportedSULBandList.list.array[sul];
	  FDDs->ul_nr_sul_band[sul]=SulItem->freqBandIndicatorNr;
	}
      }
      FDDs->dl_nr_arfcn=fDD_Info->dL_NRFreqInfo.nRARFCN;
      int dlBands=fDD_Info->dL_NRFreqInfo.freqBandListNr.list.count;
      for (int dlB=0; dlB < dlBands; dlB++) {
	F1AP_FreqBandNrItem_t * FreqItem=fDD_Info->dL_NRFreqInfo.freqBandListNr.list.array[dlB];
	FDDs->dl_nr_band[dlB]=FreqItem->freqBandIndicatorNr;
	int num_available_supported_SULBands = FreqItem->supportedSULBandList.list.count;
	for (int sul=0; sul < num_available_supported_SULBands; sul ++ ) {
	  F1AP_SupportedSULFreqBandItem_t * SulItem= FreqItem->supportedSULBandList.list.array[sul];
	  FDDs->ul_nr_sul_band[sul]=SulItem->freqBandIndicatorNr;
	}
      }
      FDDs->ul_scs=fDD_Info->uL_Transmission_Bandwidth.nRSCS;
      FDDs->ul_nrb=nrb_lut[fDD_Info->uL_Transmission_Bandwidth.nRNRB];
      FDDs->dl_scs=fDD_Info->dL_Transmission_Bandwidth.nRSCS;
      FDDs->dl_nrb=nrb_lut[fDD_Info->dL_Transmission_Bandwidth.nRNRB]; 
    }
    
    // TDD
    if (servedCellInformation->nR_Mode_Info.present==F1AP_NR_Mode_Info_PR_tDD) {
      struct tdd_s *TDDs=&req->nr_mode_info[i].tdd;
      F1AP_TDD_Info_t * tDD_Info=servedCellInformation->nR_Mode_Info.choice.tDD;
      TDDs->nr_arfcn=tDD_Info->nRFreqInfo.nRARFCN;
      TDDs->num_frequency_bands=tDD_Info->nRFreqInfo.freqBandListNr.list.count;
      for (int f=0; f < tDD_Info->nRFreqInfo.freqBandListNr.list.count; f++) {
	struct F1AP_FreqBandNrItem * FreqItem=tDD_Info->nRFreqInfo.freqBandListNr.list.array[f];
	TDDs->nr_band[f]=FreqItem->freqBandIndicatorNr;
	int num_available_supported_SULBands=FreqItem->supportedSULBandList.list.count;
	for (int sul=0; sul < num_available_supported_SULBands; sul ++ ) {
          struct F1AP_SupportedSULFreqBandItem * SulItem= FreqItem->supportedSULBandList.list.array[sul];
	  TDDs->nr_sul_band[sul]=SulItem->freqBandIndicatorNr;
	}
      }
      TDDs->scs=tDD_Info->transmission_Bandwidth.nRSCS;
      TDDs->nrb=nrb_lut[tDD_Info->transmission_Bandwidth.nRNRB];
    }
	
    struct F1AP_GNB_DU_System_Information * DUsi=served_cells_item->gNB_DU_System_Information;
    LOG_I(F1AP, "Received Cell in %d context\n", f1ap_req(true, instance)->cell_type==CELL_MACRO_GNB);
    // System Information
    /* mib */
    req->mib[i] = calloc(DUsi->mIB_message.size + 1, sizeof(char));
    memcpy(req->mib[i], DUsi->mIB_message.buf, DUsi->mIB_message.size);
    /* Convert the mme name to a printable string */
    req->mib[i][DUsi->mIB_message.size] = '\0';
    req->mib_length[i] = DUsi->mIB_message.size;
    LOG_D(F1AP, "req->mib[%d] len = %d \n", i, req->mib_length[i]);
    /* sib1 */
    req->sib1[i] = calloc(DUsi->sIB1_message.size + 1, sizeof(char));
    memcpy(req->sib1[i], DUsi->sIB1_message.buf, DUsi->sIB1_message.size);
    /* Convert the mme name to a printable string */
    req->sib1[i][DUsi->sIB1_message.size] = '\0';
    req->sib1_length[i] = DUsi->sIB1_message.size;
    LOG_D(F1AP, "req->sib1[%d] len = %d \n", i, req->sib1_length[i]);
  }
  
  // char *measurement_timing_information[F1AP_MAX_NB_CELLS];
  // uint8_t ranac[F1AP_MAX_NB_CELLS];
  // int fdd_flag = f1ap_setup_req->fdd_flag;
  // union {
  //   struct {
  //     uint32_t ul_nr_arfcn;
  //     uint8_t ul_scs;
  //     uint8_t ul_nrb;
  //     uint32_t dl_nr_arfcn;
  //     uint8_t dl_scs;
  //     uint8_t dl_nrb;
  //     uint32_t sul_active;
  //     uint32_t sul_nr_arfcn;
  //     uint8_t sul_scs;
  //     uint8_t sul_nrb;
  //     uint8_t num_frequency_bands;
  //     uint16_t nr_band[32];
  //     uint8_t num_sul_frequency_bands;
  //     uint16_t nr_sul_band[32];
  //   } fdd;
  //   struct {
  //     uint32_t nr_arfcn;
  //     uint8_t scs;
  //     uint8_t nrb;
  //     uint32_t sul_active;
  //     uint32_t sul_nr_arfcn;
  //     uint8_t sul_scs;
  //     uint8_t sul_nrb;
  //     uint8_t num_frequency_bands;
  //     uint16_t nr_band[32];
  //     uint8_t num_sul_frequency_bands;
  //     uint16_t nr_sul_band[32];
  //   } tdd;
  // } nr_mode_info[F1AP_MAX_NB_CELLS];


  
  // We copy and store in F1 task data, RRC will free "req" as it frees all itti received messages
  message_p = itti_alloc_new_message(TASK_CU_F1, 0, F1AP_SETUP_REQ);
  memcpy(&F1AP_SETUP_REQ(message_p), req, sizeof(f1ap_setup_req_t) );
  
  if (req->num_cells_available > 0) {
    if (f1ap_req(true, instance)->cell_type == CELL_MACRO_GNB) {
      itti_send_msg_to_task(TASK_RRC_GNB, GNB_MODULE_ID_TO_INSTANCE(instance), message_p);
    } else {
      itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), message_p);
    }
  } else {
    CU_send_F1_SETUP_FAILURE(instance);
    itti_free(TASK_CU_F1,message_p);
    return -1;
  }
  
  return 0;
}

int CU_send_F1_SETUP_RESPONSE(instance_t instance,
                              f1ap_setup_resp_t *f1ap_setup_resp) {
  instance_t enb_mod_idP;
  instance_t cu_mod_idP;
  // This should be fixed
  enb_mod_idP = (instance_t)0;
  cu_mod_idP  = (instance_t)0;
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
  ie1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(enb_mod_idP, cu_mod_idP);

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
      if (1) {
        /* 3.1.2 gNB-CUSystem Information */
        F1AP_ProtocolExtensionContainer_154P112_t *p_154P112=calloc(1, sizeof(* p_154P112));
        cells_to_be_activated_item->iE_Extensions = (struct F1AP_ProtocolExtensionContainer *)p_154P112;
        asn1cSequenceAdd(p_154P112->list, F1AP_Cells_to_be_Activated_List_ItemExtIEs_t, cells_to_be_activated_itemExtIEs);
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

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup response\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(true, instance, buffer, len, 0);
  return 0;
}

int CU_send_F1_SETUP_FAILURE(instance_t instance) {
  LOG_D(F1AP, "CU_send_F1_SETUP_FAILURE\n");
  instance_t enb_mod_idP=0;
  instance_t cu_mod_idP=0;
  // This should be fixed
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
  ie1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(enb_mod_idP, cu_mod_idP);
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
  f1ap_itti_send_sctp_data_req(true,instance, buffer, len, 0);
  return 0;
}

/*
    gNB-DU Configuration Update
*/

int CU_handle_gNB_DU_CONFIGURATION_UPDATE(instance_t instance,
    uint32_t assoc_id,
    uint32_t stream,
    F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_gNB_DU_CONFIGURATION_FAILURE(instance_t instance,
    F1AP_GNBDUConfigurationUpdateFailure_t *GNBDUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
    F1AP_GNBDUConfigurationUpdateAcknowledge_t *GNBDUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
    gNB-CU Configuration Update
*/

//void CU_send_gNB_CU_CONFIGURATION_UPDATE(F1AP_GNBCUConfigurationUpdate_t *GNBCUConfigurationUpdate) {
int CU_send_gNB_CU_CONFIGURATION_UPDATE(instance_t instance, f1ap_gnb_cu_configuration_update_t *f1ap_gnb_cu_configuration_update) {
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
  ieC1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(instance, 0);

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
      F1AP_ProtocolExtensionContainer_154P112_t *p_154P112=(F1AP_ProtocolExtensionContainer_154P112_t *) calloc(1,sizeof(*p_154P112));
      cells_to_be_activated_list_item->iE_Extensions = (struct F1AP_ProtocolExtensionContainer *)p_154P112;
      //F1AP_ProtocolExtensionContainer_154P112_t
      asn1cSequenceAdd(p_154P112->list,F1AP_Cells_to_be_Activated_List_ItemExtIEs_t,  cells_to_be_activated_itemExtIEs);
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

  // c3. Cells_to_be_Deactivated_List
  //
  /*
  if(!RC.nrrrc) {
    // mandatory
    // c3. Cells_to_be_Deactivated_List
    asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC3);
    ieC3->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Deactivated_List;
    ieC3->criticality               = F1AP_Criticality_reject;
    ieC3->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Deactivated_List;

    for (int i=0; i<1; i++) {
      asn1cSequenceAdd(ieC3->value.choice.Cells_to_be_Deactivated_List.list,
           F1AP_Cells_to_be_Deactivated_List_ItemIEs_t, cells_to_be_deactivated);
      cells_to_be_deactivated->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
      cells_to_be_deactivated->criticality = F1AP_Criticality_reject;
      cells_to_be_deactivated->value.present = F1AP_Cells_to_be_Deactivated_List_ItemIEs__value_PR_Cells_to_be_Deactivated_List_Item;
      // 3.1 cells to be Deactivated list item
      F1AP_Cells_to_be_Deactivated_List_Item_t *cells_to_be_deactivated_list=
  cells_to_be_deactivated->value.choice.Cells_to_be_Deactivated_List_Item;
      addnRCGI(cells_to_be_deactivated_item->nRCGI, f1ap_setup_resp->cells_to_deactivate+i);
      }
  }
  */

  // c4. GNB_CU_TNL_Association_To_Add_List
  /*
  asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC4);
  ieC4->id                        = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Add_List;
  ieC4->criticality               = F1AP_Criticality_reject;
  ieC4->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_GNB_CU_TNL_Association_To_Add_List;

  for (int i=0; i<1; i++) {
    asn1cSequenceAdd(ieC4->value.choice.GNB_CU_TNL_Association_To_Add_List.list,
         F1AP_GNB_CU_TNL_Association_To_Add_ItemIEs_t, gnb_cu_tnl_association_to_add;
       gnb_cu_tnl_association_to_add->id = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Add_Item;
       gnb_cu_tnl_association_to_add->criticality = F1AP_Criticality_reject;
       gnb_cu_tnl_association_to_add->value.present = F1AP_GNB_CU_TNL_Association_To_Add_ItemIEs__value_PR_GNB_CU_TNL_Association_To_Add_Item;

       // 4.1 GNB_CU_TNL_Association_To_Add_Item
       F1AP_GNB_CU_TNL_Association_To_Add_Item_t *gnb_cu_tnl_association_to_add_item=
         &gnb_cu_tnl_association_to_add_item_ies->value.choice.GNB_CU_TNL_Association_To_Add_Item;

       // 4.1.1 tNLAssociationTransportLayerAddress
       F1AP_CP_TransportLayerAddress_t *transportLayerAddress=;
         gnb_cu_tnl_association_to_add->value.choice.GNB_CU_TNL_Association_To_Add_Item;

       transportLayerAddress->present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address;
       TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &transportLayerAddress->choice.endpoint_IP_address);

       // memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
       // transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address_and_port;
       // transportLayerAddress.choice.endpoint_IP_address_and_port = (F1AP_Endpoint_IP_address_and_port_t *)calloc(1, sizeof(F1AP_Endpoint_IP_address_and_port_t));
       // TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address_and_port.endpoint_IP_address);

       // 4.1.2 tNLAssociationUsage
       gnb_cu_tnl_association_to_add_item->tNLAssociationUsage = F1AP_TNLAssociationUsage_non_ue;
  }
  */

  /*
    // c5. GNB_CU_TNL_Association_To_Remove_List
  asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC5);
    ieC5->id                        = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Remove_List;
    ieC5->criticality               = F1AP_Criticality_reject;
    ieC5->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_GNB_CU_TNL_Association_To_Remove_List;
    for (int i=0; i<1; i++) {
      asn1cSequenceAdd(ieC5->value.choice.GNB_CU_TNL_Association_To_Remove_List.list,
           F1AP_GNB_CU_TNL_Association_To_Remove_ItemIEs_t, gnb_cu_tnl_association_to_remove);
      gnb_cu_tnl_association_to_remove->id = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Remove_Item;
      gnb_cu_tnl_association_to_remove->criticality = F1AP_Criticality_reject;
      gnb_cu_tnl_association_to_remove->value.present = F1AP_GNB_CU_TNL_Association_To_Remove_ItemIEs__value_PR_GNB_CU_TNL_Association_To_Remove_Item;

         // 4.1 GNB_CU_TNL_Association_To_Remove_Item
         F1AP_GNB_CU_TNL_Association_To_Remove_Item_t *gnb_cu_tnl_association_to_remove_item=
     &gnb_cu_tnl_association_to_remove->value.choice.GNB_CU_TNL_Association_To_Remove_Item;

         // 4.1.1 tNLAssociationTransportLayerAddress
         F1AP_CP_TransportLayerAddress_t *transportLayerAddress=
     &gnb_cu_tnl_association_to_remove_item->tNLAssociationTransportLayerAddress;
         transportLayerAddress->present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address;
         TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &transportLayerAddress->choice.endpoint_IP_address);

         // memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
         // transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address_and_port;
         // transportLayerAddress.choice.endpoint_IP_address_and_port = (F1AP_Endpoint_IP_address_and_port_t *)calloc(1, sizeof(F1AP_Endpoint_IP_address_and_port_t));
         // TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address_and_port.endpoint_IP_address);
    }
  */

  /*
    //mandatory
   // c6. GNB_CU_TNL_Association_To_Update_List
   asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC6);
   ieC6->id                        = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Update_List;
   ieC6->criticality               = F1AP_Criticality_reject;
   ieC6->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_GNB_CU_TNL_Association_To_Update_List;

   for (int i=0;  i<1; i++) {
    asn1cSequenceAdd(ieC3->value.choice.GNB_CU_TNL_Association_To_Update_List.list,
         F1AP_GNB_CU_TNL_Association_To_Update_ItemIEs_t, gnb_cu_tnl_association_to_update);
     gnb_cu_tnl_association_to_update->id = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Update_Item;
     gnb_cu_tnl_association_to_update->criticality = F1AP_Criticality_reject;
     gnb_cu_tnl_association_to_update->value.present = F1AP_GNB_CU_TNL_Association_To_Update_ItemIEs__value_PR_GNB_CU_TNL_Association_To_Update_Item;
     // 4.1 GNB_CU_TNL_Association_To_Update_Item
     F1AP_GNB_CU_TNL_Association_To_Update_Item_t *gnb_cu_tnl_association_to_update_item=
       &gnb_cu_tnl_association_to_update->value.choice.GNB_CU_TNL_Association_To_Update_Item;
     // 4.1.1 tNLAssociationTransportLayerAddress
     F1AP_CP_TransportLayerAddress_t *transportLayerAddress=
       &gnb_cu_tnl_association_to_update_item.tNLAssociationTransportLayerAddress;
     transportLayerAddress->present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address;
     TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &transportLayerAddress->choice.endpoint_IP_address);
     // memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
     // transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address_and_port;
     // transportLayerAddress.choice.endpoint_IP_address_and_port = (F1AP_Endpoint_IP_address_and_port_t *)calloc(1, sizeof(F1AP_Endpoint_IP_address_and_port_t));
     // TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address_and_port.endpoint_IP_address);
     // 4.1.2 tNLAssociationUsage
     if (1) {
       gnb_cu_tnl_association_to_update_item.tNLAssociationUsage = (F1AP_TNLAssociationUsage_t *)calloc(1, sizeof(F1AP_TNLAssociationUsage_t));
       *gnb_cu_tnl_association_to_update_item.tNLAssociationUsage = F1AP_TNLAssociationUsage_non_ue;
     }
   }
  */

  /*
  // c7. Cells_to_be_Barred_List
  asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC7);
  ieC7->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Barred_List;
  ieC7->criticality               = F1AP_Criticality_reject;
  ieC7->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Barred_List;

  for (int i=0; i<1; i++) {
    asn1cSequenceAdd(ieC7->value.choice.Cells_to_be_Barred_List.list,
         F1AP_Cells_to_be_Barred_ItemIEs_t,cells_to_be_barred);
   cells_to_be_barred->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
   cells_to_be_barred->criticality = F1AP_Criticality_reject;
   cells_to_be_barred->value.present = F1AP_Cells_to_be_Barred_ItemIEs__value_PR_Cells_to_be_Barred_Item;
   // 7.1 cells to be Deactivated list item
   F1AP_Cells_to_be_Barred_Item_t *cells_to_be_barred_item=
     &cells_to_be_barred_item_ies->value.choice.Cells_to_be_Barred_Item;
   // - nRCGI
   addnRCGI(cells_to_be_barred_item->nRCGI, f1ap_gnb_cu_configuration_update->cells_to_activate+i);
   // 7.2 cellBarred
   cells_to_be_barred_item->cellBarred = F1AP_CellBarred_not_barred;
  }
  */

  /*
   // c8. Protected_EUTRA_Resources_List
  asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC8);
   ieC8->id                        = F1AP_ProtocolIE_ID_id_Protected_EUTRA_Resources_List;
   ieC8->criticality               = F1AP_Criticality_reject;
   ieC8->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Protected_EUTRA_Resources_List;

   for (int i=0; i<1; i++) {
     asn1cSequenceAdd(ieC8->value.choice.Protected_EUTRA_Resources_List.list,
          F1AP_Protected_EUTRA_Resources_ItemIEs_t, protected_eutra_resources);
     // 8.1 SpectrumSharingGroupID
     protected_eutra_resources->id = F1AP_ProtocolIE_ID_id_Protected_EUTRA_Resources_List;
     protected_eutra_resources->criticality = F1AP_Criticality_reject;
     protected_eutra_resources->value.present = F1AP_Protected_EUTRA_Resources_ItemIEs__value_PR_Protected_EUTRA_Resources_Item;
     ((F1AP_Protected_EUTRA_Resources_Item_t *)&protected_eutra_resources->value.choice.Protected_EUTRA_Resources_Item)->spectrumSharingGroupID = 123L;
     memset(&protected_eutra_resources->value.choice.Protected_EUTRA_Resources_Item,0,
            sizeof(F1AP_Protected_EUTRA_Resources_Item_t));

     asn1cSequenceAdd(protected_eutra_resources->value.choice.ListofEUTRACellsinGNBDUCoordination.list,
          F1AP_Served_EUTRA_Cells_Information_t, served_eutra_cells_information);
      memset((void *)&served_eutra_cells_information, 0, sizeof(F1AP_Served_EUTRA_Cells_Information_t));

      F1AP_EUTRA_Mode_Info_t *eUTRA_Mode_Info=
  &served_eutra_cells_information.eUTRA_Mode_Info;

      // eUTRAFDD
      eUTRA_Mode_Info->present = F1AP_EUTRA_Mode_Info_PR_eUTRAFDD;
      F1AP_EUTRA_FDD_Info_t *eutra_fdd_info = (F1AP_EUTRA_FDD_Info_t *)calloc(1, sizeof(F1AP_EUTRA_FDD_Info_t));
      eutra_fdd_info->uL_offsetToPointA = 123L;
      eutra_fdd_info->dL_offsetToPointA = 456L;
      eUTRA_Mode_Info.choice.eUTRAFDD = eutra_fdd_info;

      // eUTRATDD
      // eUTRA_Mode_Info.present = F1AP_EUTRA_Mode_Info_PR_eUTRATDD;
      // F1AP_EUTRA_TDD_Info_t *eutra_tdd_info;
      // eutra_tdd_info = (F1AP_EUTRA_TDD_Info_t *)calloc(1, sizeof(F1AP_EUTRA_TDD_Info_t));
      // eutra_tdd_info->uL_offsetToPointA = 123L;
      // eutra_tdd_info->dL_offsetToPointA = 456L;
      // eUTRA_Mode_Info.choice.eUTRATDD = eutra_tdd_info;

      OCTET_STRING_fromBuf(&served_eutra_cells_information.protectedEUTRAResourceIndication, "asdsa1d32sa1d31asd31as",
                      strlen("asdsa1d32sa1d31asd31as"));
   }
  */

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 gNB-CU CONFIGURATION UPDATE\n");
    return -1;
  }

  LOG_DUMPMSG(F1AP, LOG_DUMP_CHAR, buffer, len, "F1AP gNB-CU CONFIGURATION UPDATE : ");
  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(true,instance, buffer, len, 0);
  return 0;
}

int CU_handle_gNB_CU_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
    uint32_t assoc_id,
    uint32_t stream,
    F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
    uint32_t assoc_id,
    uint32_t stream,
    F1AP_F1AP_PDU_t *pdu) {
  LOG_I(F1AP,"Cell Configuration ok (assoc_id %d)\n",assoc_id);
  return(0);
}


int CU_handle_gNB_DU_RESOURCE_COORDINATION_REQUEST(instance_t instance,
    uint32_t assoc_id,
    uint32_t stream,
    F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(0, "Not implemented yet\n");
}

int CU_send_gNB_DU_RESOURCE_COORDINATION_RESPONSE(instance_t instance,
    F1AP_GNBDUResourceCoordinationResponse_t *GNBDUResourceCoordinationResponse) {
  AssertFatal(0, "Not implemented yet\n");
}
