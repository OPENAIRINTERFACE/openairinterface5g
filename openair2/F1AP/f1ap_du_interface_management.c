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
#include "f1ap_decoder.h"
#include "f1ap_du_interface_management.h"

extern f1ap_setup_req_t *f1ap_du_data;


void DU_handle_RESET(instance_t instance, F1AP_Reset_t *Reset) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_RESET_ACKKNOWLEDGE(instance_t instance, F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_RESET(instance_t instance, F1AP_Reset_t *Reset) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_RESET_ACKNOWLEDGE(instance_t instance, F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
    Error Indication
*/

void DU_send_ERROR_INDICATION(instance_t instance, F1AP_F1AP_PDU_t *pdu_p) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_ERROR_INDICATION(instance_t instance, F1AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
    F1 Setup
*/

// SETUP REQUEST
void DU_send_F1_SETUP_REQUEST(instance_t instance) {
  module_id_t enb_mod_idP;
  module_id_t du_mod_idP;

  F1AP_F1AP_PDU_t          pdu; 
  F1AP_F1SetupRequest_t    *out;
  F1AP_F1SetupRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;
  int       j = 0;

  /* Create */
  /* 0. pdu Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_F1SetupRequest;
  out = &pdu.choice.initiatingMessage->value.choice.F1SetupRequest;

  /* mandatory */
  /* c1. Transaction ID (integer value) */
  ie = (F1AP_F1SetupRequestIEs_t *)calloc(1, sizeof(F1AP_F1SetupRequestIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_F1SetupRequestIEs__value_PR_TransactionID;
  ie->value.choice.TransactionID = F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_ID (integrer value) */
  ie = (F1AP_F1SetupRequestIEs_t *)calloc(1, sizeof(F1AP_F1SetupRequestIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_ID;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_ID;
  asn_int642INTEGER(&ie->value.choice.GNB_DU_ID, f1ap_du_data->gNB_DU_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. GNB_DU_Name */
  if (f1ap_du_data->gNB_DU_name != NULL) {
    ie = (F1AP_F1SetupRequestIEs_t *)calloc(1, sizeof(F1AP_F1SetupRequestIEs_t));
    ie->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_Name;
    ie->criticality               = F1AP_Criticality_ignore;
    ie->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Name;
    OCTET_STRING_fromBuf(&ie->value.choice.GNB_DU_Name, f1ap_du_data->gNB_DU_name,
                         strlen(f1ap_du_data->gNB_DU_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. serverd cells list */
  ie = (F1AP_F1SetupRequestIEs_t *)calloc(1, sizeof(F1AP_F1SetupRequestIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_Served_Cells_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Served_Cells_List;

  int num_cells_available = f1ap_du_data->num_cells_available;
  printf("num_cells_available = %d \n", num_cells_available);
  for (i=0;
       i<num_cells_available;
       i++) {
        /* mandatory */
        /* 4.1 serverd cells item */

        F1AP_GNB_DU_Served_Cells_ItemIEs_t *gnb_du_served_cell_list_item_ies;
        gnb_du_served_cell_list_item_ies = (F1AP_GNB_DU_Served_Cells_ItemIEs_t *)calloc(1, sizeof(F1AP_GNB_DU_Served_Cells_ItemIEs_t));
        gnb_du_served_cell_list_item_ies->id = F1AP_ProtocolIE_ID_id_GNB_DU_Served_Cells_Item;
        gnb_du_served_cell_list_item_ies->criticality = F1AP_Criticality_reject;
        gnb_du_served_cell_list_item_ies->value.present = F1AP_GNB_DU_Served_Cells_ItemIEs__value_PR_GNB_DU_Served_Cells_Item;
        

        F1AP_GNB_DU_Served_Cells_Item_t gnb_du_served_cells_item;
        memset((void *)&gnb_du_served_cells_item, 0, sizeof(F1AP_GNB_DU_Served_Cells_Item_t));

        /* 4.1.1 serverd cell Information */
        F1AP_Served_Cell_Information_t served_cell_information;

        memset((void *)&served_cell_information, 0, sizeof(F1AP_Served_Cell_Information_t));

        /* - nRCGI */
        F1AP_NRCGI_t nRCGI;
        MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i], &nRCGI.pLMN_Identity);
        //MCC_MNC_TO_PLMNID(208, 95, 2, &nRCGI.pLMN_Identity);

        NR_CELL_ID_TO_BIT_STRING(f1ap_du_data->nr_cellid[i], &nRCGI.nRCellIdentity);
        served_cell_information.nRCGI = nRCGI;

        /* - nRPCI */
        served_cell_information.nRPCI = f1ap_du_data->nr_pci[i];  // int 0..1007

        /* - fiveGS_TAC */
        OCTET_STRING_fromBuf(&served_cell_information.fiveGS_TAC,
                             &f1ap_du_data->tac[i],
                             3);

        /* - Configured_EPS_TAC */
        if(0){
          served_cell_information.configured_EPS_TAC = (F1AP_Configured_EPS_TAC_t *)calloc(1, sizeof(F1AP_Configured_EPS_TAC_t));
          OCTET_STRING_fromBuf(served_cell_information.configured_EPS_TAC,
                             "2",
                             2);
        }

        /* - broadcast PLMNs */
        // RK: add the num_available_broadcast_PLMNs to the message 
        int num_available_broadcast_PLMNs = 1; //f1ap_du_data->num_available_broadcast_PLMNs;
        printf("num_available_broadcast_PLMNs = %d \n", num_available_broadcast_PLMNs);
        for (j=0;
            j<num_available_broadcast_PLMNs;    // num_available_broadcast_PLMNs
            j++) {
            /* > PLMN BroadcastPLMNs Item */
            F1AP_BroadcastPLMNs_Item_t *broadcastPLMNs_Item = (F1AP_BroadcastPLMNs_Item_t *)calloc(1, sizeof(F1AP_BroadcastPLMNs_Item_t));
            //MCC_MNC_TO_PLMNID(208, 95, 2, &broadcastPLMNs_Item->pLMN_Identity);
            MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i], &broadcastPLMNs_Item->pLMN_Identity);
            ASN_SEQUENCE_ADD(&served_cell_information.servedPLMNs.list, broadcastPLMNs_Item);
        }

        // // /* - CHOICE NR-MODE-Info */
        F1AP_NR_Mode_Info_t nR_Mode_Info;
        //f1ap_du_data->fdd_flag = 1;
        if (f1ap_du_data->fdd_flag) { // FDD
          nR_Mode_Info.present = F1AP_NR_Mode_Info_PR_fDD;
          /* > FDD >> FDD Info */
          F1AP_FDD_Info_t *fDD_Info = (F1AP_FDD_Info_t *)calloc(1, sizeof(F1AP_FDD_Info_t));
          /* >>> UL NRFreqInfo */
          fDD_Info->uL_NRFreqInfo.nRARFCN = f1ap_du_data->nr_mode_info[i].fdd.ul_nr_arfcn;

          F1AP_FreqBandNrItem_t ul_freqBandNrItem;
          memset((void *)&ul_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          ul_freqBandNrItem.freqBandIndicatorNr = 777L;

            F1AP_SupportedSULFreqBandItem_t ul_supportedSULFreqBandItem;
            memset((void *)&ul_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            ul_supportedSULFreqBandItem.freqBandIndicatorNr = 777L;
            ASN_SEQUENCE_ADD(&ul_freqBandNrItem.supportedSULBandList.list, &ul_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&fDD_Info->uL_NRFreqInfo.freqBandListNr.list, &ul_freqBandNrItem);

          /* >>> DL NRFreqInfo */
          fDD_Info->dL_NRFreqInfo.nRARFCN = f1ap_du_data->nr_mode_info[i].fdd.dl_nr_arfcn;

          F1AP_FreqBandNrItem_t dl_freqBandNrItem;
          memset((void *)&dl_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          dl_freqBandNrItem.freqBandIndicatorNr = 777L;

            F1AP_SupportedSULFreqBandItem_t dl_supportedSULFreqBandItem;
            memset((void *)&dl_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            dl_supportedSULFreqBandItem.freqBandIndicatorNr = 777L;
            ASN_SEQUENCE_ADD(&dl_freqBandNrItem.supportedSULBandList.list, &dl_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&fDD_Info->dL_NRFreqInfo.freqBandListNr.list, &dl_freqBandNrItem);

          /* >>> UL Transmission Bandwidth */
          fDD_Info->uL_Transmission_Bandwidth.nRSCS = f1ap_du_data->nr_mode_info[i].fdd.ul_scs;
          fDD_Info->uL_Transmission_Bandwidth.nRNRB = f1ap_du_data->nr_mode_info[i].fdd.ul_nrb;
          /* >>> DL Transmission Bandwidth */
          fDD_Info->dL_Transmission_Bandwidth.nRSCS = f1ap_du_data->nr_mode_info[i].fdd.dl_scs;
          fDD_Info->dL_Transmission_Bandwidth.nRNRB = f1ap_du_data->nr_mode_info[i].fdd.dl_nrb;
          
          nR_Mode_Info.choice.fDD = fDD_Info;
        } else { // TDD
          nR_Mode_Info.present = F1AP_NR_Mode_Info_PR_tDD;

          /* > TDD >> TDD Info */
          F1AP_TDD_Info_t *tDD_Info = (F1AP_TDD_Info_t *)calloc(1, sizeof(F1AP_TDD_Info_t));
          /* >>> ARFCN */
          tDD_Info->nRFreqInfo.nRARFCN = f1ap_du_data->nr_mode_info[i].tdd.nr_arfcn; // Integer
          F1AP_FreqBandNrItem_t nr_freqBandNrItem;
          memset((void *)&nr_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          // RK: missing params
          nr_freqBandNrItem.freqBandIndicatorNr = 555L;

            F1AP_SupportedSULFreqBandItem_t nr_supportedSULFreqBandItem;
            memset((void *)&nr_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            nr_supportedSULFreqBandItem.freqBandIndicatorNr = 444L;
            ASN_SEQUENCE_ADD(&nr_freqBandNrItem.supportedSULBandList.list, &nr_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&tDD_Info->nRFreqInfo.freqBandListNr.list, &nr_freqBandNrItem);

          tDD_Info->transmission_Bandwidth.nRSCS= f1ap_du_data->nr_mode_info[i].tdd.scs;
          tDD_Info->transmission_Bandwidth.nRNRB= f1ap_du_data->nr_mode_info[i].tdd.nrb;
     
          nR_Mode_Info.choice.tDD = tDD_Info;
        } 
        
        served_cell_information.nR_Mode_Info = nR_Mode_Info;

        /* - measurementTimingConfiguration */
        char *measurementTimingConfiguration = "0"; //&f1ap_du_data->measurement_timing_information[i]; // sept. 2018

        OCTET_STRING_fromBuf(&served_cell_information.measurementTimingConfiguration,
                             measurementTimingConfiguration,
                             strlen(measurementTimingConfiguration));
        gnb_du_served_cells_item.served_Cell_Information = served_cell_information; //

        /* 4.1.2 gNB-DU System Information */
        F1AP_GNB_DU_System_Information_t *gNB_DU_System_Information = (F1AP_GNB_DU_System_Information_t *)calloc(1, sizeof(F1AP_GNB_DU_System_Information_t));

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->mIB_message,  // sept. 2018
                             f1ap_du_data->mib[i],//f1ap_du_data->mib,
                             f1ap_du_data->mib_length[i]);

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->sIB1_message,  // sept. 2018
                             f1ap_du_data->sib1[i],
                             f1ap_du_data->sib1_length[i]);

        gnb_du_served_cells_item.gNB_DU_System_Information = gNB_DU_System_Information; //

        /* ADD */
        gnb_du_served_cell_list_item_ies->value.choice.GNB_DU_Served_Cells_Item = gnb_du_served_cells_item;

        ASN_SEQUENCE_ADD(&ie->value.choice.GNB_DU_Served_Cells_List.list, 
                        gnb_du_served_cell_list_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
  }

  du_f1ap_itti_send_sctp_data_req(instance, f1ap_du_data->assoc_id, buffer, len, 0);
}

int DU_handle_F1_SETUP_RESPONSE(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                F1AP_F1AP_PDU_t *pdu)
{
   printf("DU_handle_F1_SETUP_RESPONSE\n");
   return 0;
}

// SETUP FAILURE
void DU_handle_F1_SETUP_FAILURE(instance_t instance, F1AP_F1AP_PDU_t *pdu_p) {
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
    gNB-DU Configuration Update
*/

//void DU_send_gNB_DU_CONFIGURATION_UPDATE(F1AP_GNBDUConfigurationUpdate_t *GNBDUConfigurationUpdate) {
void DU_send_gNB_DU_CONFIGURATION_UPDATE(instance_t instance,
                                         instance_t du_mod_idP,
                                         f1ap_setup_req_t *f1ap_du_data) {
  F1AP_F1AP_PDU_t                     pdu;
  F1AP_GNBDUConfigurationUpdate_t     *out;
  F1AP_GNBDUConfigurationUpdateIEs_t  *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;
  int       j = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_gNBDUConfigurationUpdate;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_GNBDUConfigurationUpdate;
  out = &pdu.choice.initiatingMessage->value.choice.GNBDUConfigurationUpdate;

  /* mandatory */
  /* c1. Transaction ID (integer value) */
  ie = (F1AP_GNBDUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBDUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_TransactionID;
  ie->value.choice.TransactionID = F1AP_get_next_transaction_identifier(instance, du_mod_idP);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c2. Served_Cells_To_Add */
  ie = (F1AP_GNBDUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBDUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Add_List;

  for (j=0;
       j<1;
       j++) {
        //
        F1AP_Served_Cells_To_Add_ItemIEs_t *served_cells_to_add_item_ies;
        served_cells_to_add_item_ies = (F1AP_Served_Cells_To_Add_ItemIEs_t *)calloc(1, sizeof(F1AP_Served_Cells_To_Add_ItemIEs_t));
        served_cells_to_add_item_ies->id            = F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_Item;
        served_cells_to_add_item_ies->criticality   = F1AP_Criticality_reject;
        served_cells_to_add_item_ies->value.present = F1AP_Served_Cells_To_Add_ItemIEs__value_PR_Served_Cells_To_Add_Item;
        
        F1AP_Served_Cells_To_Add_Item_t served_cells_to_add_item;
        memset((void *)&served_cells_to_add_item, 0, sizeof(F1AP_Served_Cells_To_Add_Item_t));

        /* 2.1.1 serverd cell Information */
        F1AP_Served_Cell_Information_t served_cell_information;

        memset((void *)&served_cell_information, 0, sizeof(F1AP_Served_Cell_Information_t));
        /* - nRCGI */
        F1AP_NRCGI_t nRCGI;
        MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i], &nRCGI.pLMN_Identity);
        NR_CELL_ID_TO_BIT_STRING(f1ap_du_data->nr_cellid[i], &nRCGI.nRCellIdentity);
        served_cell_information.nRCGI = nRCGI;

        /* - nRPCI */
        served_cell_information.nRPCI = 321L;  // int 0..1007

        /* - fiveGS_TAC */
        OCTET_STRING_fromBuf(&served_cell_information.fiveGS_TAC,
                             "10",
                             3);

        /* - Configured_EPS_TAC */
        if(1){
          served_cell_information.configured_EPS_TAC = (F1AP_Configured_EPS_TAC_t *)calloc(1, sizeof(F1AP_Configured_EPS_TAC_t));
          OCTET_STRING_fromBuf(served_cell_information.configured_EPS_TAC,
                             "2",
                             2);
        }

        /* - broadcast PLMNs */
        int maxnoofBPLMNS = 1;
        for (i=0;
            i<maxnoofBPLMNS;
            i++) {
            /* > PLMN BroadcastPLMNs Item */
            F1AP_BroadcastPLMNs_Item_t *broadcastPLMNs_Item = (F1AP_BroadcastPLMNs_Item_t *)calloc(1, sizeof(F1AP_BroadcastPLMNs_Item_t));
            //memset((void *)&broadcastPLMNs_Item, 0, sizeof(F1AP_BroadcastPLMNs_Item_t));
            MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i], &broadcastPLMNs_Item->pLMN_Identity);
            ASN_SEQUENCE_ADD(&served_cell_information.servedPLMNs.list, broadcastPLMNs_Item);
        }

        // // /* - CHOICE NR-MODE-Info */
        F1AP_NR_Mode_Info_t nR_Mode_Info;

        if ("FDD") {
          nR_Mode_Info.present = F1AP_NR_Mode_Info_PR_fDD;
          /* > FDD >> FDD Info */
          F1AP_FDD_Info_t *fDD_Info = (F1AP_FDD_Info_t *)calloc(1, sizeof(F1AP_FDD_Info_t));
          /* >>> UL NRFreqInfo */
          fDD_Info->uL_NRFreqInfo.nRARFCN = 999L;

          F1AP_FreqBandNrItem_t ul_freqBandNrItem;
          memset((void *)&ul_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          ul_freqBandNrItem.freqBandIndicatorNr = 888L;

            F1AP_SupportedSULFreqBandItem_t ul_supportedSULFreqBandItem;
            memset((void *)&ul_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            ul_supportedSULFreqBandItem.freqBandIndicatorNr = 777L;
            ASN_SEQUENCE_ADD(&ul_freqBandNrItem.supportedSULBandList.list, &ul_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&fDD_Info->uL_NRFreqInfo.freqBandListNr.list, &ul_freqBandNrItem);

          /* >>> DL NRFreqInfo */
          fDD_Info->dL_NRFreqInfo.nRARFCN = 666L;

          F1AP_FreqBandNrItem_t dl_freqBandNrItem;
          memset((void *)&dl_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          dl_freqBandNrItem.freqBandIndicatorNr = 555L;

            F1AP_SupportedSULFreqBandItem_t dl_supportedSULFreqBandItem;
            memset((void *)&dl_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            dl_supportedSULFreqBandItem.freqBandIndicatorNr = 444L;
            ASN_SEQUENCE_ADD(&dl_freqBandNrItem.supportedSULBandList.list, &dl_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&fDD_Info->dL_NRFreqInfo.freqBandListNr.list, &dl_freqBandNrItem);

          /* >>> UL Transmission Bandwidth */
          fDD_Info->uL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
          fDD_Info->uL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
          /* >>> DL Transmission Bandwidth */
          fDD_Info->dL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
          fDD_Info->dL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
          
          nR_Mode_Info.choice.fDD = fDD_Info;
        } else { // TDD
          nR_Mode_Info.present = F1AP_NR_Mode_Info_PR_tDD;

          /* > TDD >> TDD Info */
          F1AP_TDD_Info_t *tDD_Info = (F1AP_TDD_Info_t *)calloc(1, sizeof(F1AP_TDD_Info_t));
          /* >>> ARFCN */
          tDD_Info->nRFreqInfo.nRARFCN = 999L; // Integer
          F1AP_FreqBandNrItem_t nr_freqBandNrItem;
          memset((void *)&nr_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          nr_freqBandNrItem.freqBandIndicatorNr = 555L;

            F1AP_SupportedSULFreqBandItem_t nr_supportedSULFreqBandItem;
            memset((void *)&nr_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            nr_supportedSULFreqBandItem.freqBandIndicatorNr = 444L;
            ASN_SEQUENCE_ADD(&nr_freqBandNrItem.supportedSULBandList.list, &nr_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&tDD_Info->nRFreqInfo.freqBandListNr.list, &nr_freqBandNrItem);

          tDD_Info->transmission_Bandwidth.nRSCS= F1AP_NRSCS_scs15;
          tDD_Info->transmission_Bandwidth.nRNRB= F1AP_NRNRB_nrb11;
     
          nR_Mode_Info.choice.tDD = tDD_Info;
        } 
        
        served_cell_information.nR_Mode_Info = nR_Mode_Info;

        /* - measurementTimingConfiguration */
        char *measurementTimingConfiguration = "0"; // sept. 2018

        OCTET_STRING_fromBuf(&served_cell_information.measurementTimingConfiguration,
                             measurementTimingConfiguration,
                             strlen(measurementTimingConfiguration));
        served_cells_to_add_item.served_Cell_Information = served_cell_information; //

        /* 2.1.2 gNB-DU System Information */
        F1AP_GNB_DU_System_Information_t *gNB_DU_System_Information = (F1AP_GNB_DU_System_Information_t *)calloc(1, sizeof(F1AP_GNB_DU_System_Information_t));

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->mIB_message,  // sept. 2018
                             "1",
                             sizeof("1"));

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->sIB1_message,  // sept. 2018
                             "1",
                             sizeof("1"));
        served_cells_to_add_item.gNB_DU_System_Information = gNB_DU_System_Information; //

        /* ADD */
        served_cells_to_add_item_ies->value.choice.Served_Cells_To_Add_Item = served_cells_to_add_item;

        ASN_SEQUENCE_ADD(&ie->value.choice.Served_Cells_To_Add_List.list, 
                        served_cells_to_add_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c3. Served_Cells_To_Modify */
  ie = (F1AP_GNBDUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBDUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Served_Cells_To_Modify_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Modify_List;

  for (i=0;
       i<1;
       i++) {
        //
        F1AP_Served_Cells_To_Modify_ItemIEs_t *served_cells_to_modify_item_ies;
        served_cells_to_modify_item_ies = (F1AP_Served_Cells_To_Modify_ItemIEs_t *)calloc(1, sizeof(F1AP_Served_Cells_To_Modify_ItemIEs_t));
        served_cells_to_modify_item_ies->id            = F1AP_ProtocolIE_ID_id_Served_Cells_To_Modify_Item;
        served_cells_to_modify_item_ies->criticality   = F1AP_Criticality_reject;
        served_cells_to_modify_item_ies->value.present = F1AP_Served_Cells_To_Modify_ItemIEs__value_PR_Served_Cells_To_Modify_Item;
        
        F1AP_Served_Cells_To_Modify_Item_t served_cells_to_modify_item;
        memset((void *)&served_cells_to_modify_item, 0, sizeof(F1AP_Served_Cells_To_Modify_Item_t));

        /* 3.1 oldNRCGI */
        F1AP_NRCGI_t oldNRCGI;
        MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i],
                                         &oldNRCGI.pLMN_Identity);
        NR_CELL_ID_TO_BIT_STRING(f1ap_du_data->nr_cellid[i], &oldNRCGI.nRCellIdentity);
        served_cells_to_modify_item.oldNRCGI = oldNRCGI;


        /* 3.2.1 serverd cell Information */
        F1AP_Served_Cell_Information_t served_cell_information;
        memset((void *)&served_cell_information, 0, sizeof(F1AP_Served_Cell_Information_t));

        /* - nRCGI */
        F1AP_NRCGI_t nRCGI;
        MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i],
                                         &nRCGI.pLMN_Identity);
        NR_CELL_ID_TO_BIT_STRING(f1ap_du_data->nr_cellid[i], &nRCGI.nRCellIdentity);
        served_cell_information.nRCGI = nRCGI;

        /* - nRPCI */
        served_cell_information.nRPCI = 321L;  // int 0..1007

        /* - fiveGS_TAC */
        OCTET_STRING_fromBuf(&served_cell_information.fiveGS_TAC,
                             "10",
                             3);

        /* - Configured_EPS_TAC */
        if(1){
          served_cell_information.configured_EPS_TAC = (F1AP_Configured_EPS_TAC_t *)calloc(1, sizeof(F1AP_Configured_EPS_TAC_t));
          OCTET_STRING_fromBuf(served_cell_information.configured_EPS_TAC,
                             "2",
                             2);
        }

        /* - broadcast PLMNs */
        int maxnoofBPLMNS = 1;
        for (i=0;
            i<maxnoofBPLMNS;
            i++) {
            /* > PLMN BroadcastPLMNs Item */
            F1AP_BroadcastPLMNs_Item_t *broadcastPLMNs_Item = (F1AP_BroadcastPLMNs_Item_t *)calloc(1, sizeof(F1AP_BroadcastPLMNs_Item_t));
            //memset((void *)&broadcastPLMNs_Item, 0, sizeof(F1AP_BroadcastPLMNs_Item_t));
            MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i], &broadcastPLMNs_Item->pLMN_Identity);
            ASN_SEQUENCE_ADD(&served_cell_information.servedPLMNs.list, broadcastPLMNs_Item);
        }

        // // /* - CHOICE NR-MODE-Info */
        F1AP_NR_Mode_Info_t nR_Mode_Info;

        if ("FDD") {
          nR_Mode_Info.present = F1AP_NR_Mode_Info_PR_fDD;
          /* > FDD >> FDD Info */
          F1AP_FDD_Info_t *fDD_Info = (F1AP_FDD_Info_t *)calloc(1, sizeof(F1AP_FDD_Info_t));
          /* >>> UL NRFreqInfo */
          fDD_Info->uL_NRFreqInfo.nRARFCN = 999L;

          F1AP_FreqBandNrItem_t ul_freqBandNrItem;
          memset((void *)&ul_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          ul_freqBandNrItem.freqBandIndicatorNr = 888L;

            F1AP_SupportedSULFreqBandItem_t ul_supportedSULFreqBandItem;
            memset((void *)&ul_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            ul_supportedSULFreqBandItem.freqBandIndicatorNr = 777L;
            ASN_SEQUENCE_ADD(&ul_freqBandNrItem.supportedSULBandList.list, &ul_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&fDD_Info->uL_NRFreqInfo.freqBandListNr.list, &ul_freqBandNrItem);

          /* >>> DL NRFreqInfo */
          fDD_Info->dL_NRFreqInfo.nRARFCN = 666L;

          F1AP_FreqBandNrItem_t dl_freqBandNrItem;
          memset((void *)&dl_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          dl_freqBandNrItem.freqBandIndicatorNr = 555L;

            F1AP_SupportedSULFreqBandItem_t dl_supportedSULFreqBandItem;
            memset((void *)&dl_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            dl_supportedSULFreqBandItem.freqBandIndicatorNr = 444L;
            ASN_SEQUENCE_ADD(&dl_freqBandNrItem.supportedSULBandList.list, &dl_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&fDD_Info->dL_NRFreqInfo.freqBandListNr.list, &dl_freqBandNrItem);

          /* >>> UL Transmission Bandwidth */
          fDD_Info->uL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
          fDD_Info->uL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
          /* >>> DL Transmission Bandwidth */
          fDD_Info->dL_Transmission_Bandwidth.nRSCS = F1AP_NRSCS_scs15;
          fDD_Info->dL_Transmission_Bandwidth.nRNRB = F1AP_NRNRB_nrb11;
          
          nR_Mode_Info.choice.fDD = fDD_Info;
        } else { // TDD
          nR_Mode_Info.present = F1AP_NR_Mode_Info_PR_tDD;

          /* > TDD >> TDD Info */
          F1AP_TDD_Info_t *tDD_Info = (F1AP_TDD_Info_t *)calloc(1, sizeof(F1AP_TDD_Info_t));
          /* >>> ARFCN */
          tDD_Info->nRFreqInfo.nRARFCN = 999L; // Integer
          F1AP_FreqBandNrItem_t nr_freqBandNrItem;
          memset((void *)&nr_freqBandNrItem, 0, sizeof(F1AP_FreqBandNrItem_t));
          nr_freqBandNrItem.freqBandIndicatorNr = 555L;

            F1AP_SupportedSULFreqBandItem_t nr_supportedSULFreqBandItem;
            memset((void *)&nr_supportedSULFreqBandItem, 0, sizeof(F1AP_SupportedSULFreqBandItem_t));
            nr_supportedSULFreqBandItem.freqBandIndicatorNr = 444L;
            ASN_SEQUENCE_ADD(&nr_freqBandNrItem.supportedSULBandList.list, &nr_supportedSULFreqBandItem);

          ASN_SEQUENCE_ADD(&tDD_Info->nRFreqInfo.freqBandListNr.list, &nr_freqBandNrItem);

          tDD_Info->transmission_Bandwidth.nRSCS= F1AP_NRSCS_scs15;
          tDD_Info->transmission_Bandwidth.nRNRB= F1AP_NRNRB_nrb11;
     
          nR_Mode_Info.choice.tDD = tDD_Info;
        } 
        
        served_cell_information.nR_Mode_Info = nR_Mode_Info;

        /* - measurementTimingConfiguration */
        char *measurementTimingConfiguration = "0"; // sept. 2018

        OCTET_STRING_fromBuf(&served_cell_information.measurementTimingConfiguration,
                             measurementTimingConfiguration,
                             strlen(measurementTimingConfiguration));
        served_cells_to_modify_item.served_Cell_Information = served_cell_information; //

        /* 3.2.2 gNB-DU System Information */
        F1AP_GNB_DU_System_Information_t *gNB_DU_System_Information = (F1AP_GNB_DU_System_Information_t *)calloc(1, sizeof(F1AP_GNB_DU_System_Information_t));

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->mIB_message,  // sept. 2018
                             "1",
                             sizeof("1"));

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->sIB1_message,  // sept. 2018
                             "1",
                             sizeof("1"));
        served_cells_to_modify_item.gNB_DU_System_Information = gNB_DU_System_Information; //

        /* ADD */
        served_cells_to_modify_item_ies->value.choice.Served_Cells_To_Modify_Item = served_cells_to_modify_item;

        ASN_SEQUENCE_ADD(&ie->value.choice.Served_Cells_To_Modify_List.list, 
                        served_cells_to_modify_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c4. Served_Cells_To_Delete */
  ie = (F1AP_GNBDUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBDUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Served_Cells_To_Delete_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Delete_List;

  for (i=0;
       i<1;
       i++) {
        //
        F1AP_Served_Cells_To_Delete_ItemIEs_t *served_cells_to_delete_item_ies;
        served_cells_to_delete_item_ies = (F1AP_Served_Cells_To_Delete_ItemIEs_t *)calloc(1, sizeof(F1AP_Served_Cells_To_Delete_ItemIEs_t));
        served_cells_to_delete_item_ies->id            = F1AP_ProtocolIE_ID_id_Served_Cells_To_Delete_Item;
        served_cells_to_delete_item_ies->criticality   = F1AP_Criticality_reject;
        served_cells_to_delete_item_ies->value.present = F1AP_Served_Cells_To_Delete_ItemIEs__value_PR_Served_Cells_To_Delete_Item;
        
        F1AP_Served_Cells_To_Delete_Item_t served_cells_to_delete_item;
        memset((void *)&served_cells_to_delete_item, 0, sizeof(F1AP_Served_Cells_To_Delete_Item_t));

        /* 3.1 oldNRCGI */
        F1AP_NRCGI_t oldNRCGI;
        MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i],
                                         &oldNRCGI.pLMN_Identity);
        NR_CELL_ID_TO_BIT_STRING(123456, &oldNRCGI.nRCellIdentity);
        served_cells_to_delete_item.oldNRCGI = oldNRCGI;

        /* ADD */
        served_cells_to_delete_item_ies->value.choice.Served_Cells_To_Delete_Item = served_cells_to_delete_item;

        ASN_SEQUENCE_ADD(&ie->value.choice.Served_Cells_To_Delete_List.list, 
                         served_cells_to_delete_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c5. Active_Cells_List */
  ie = (F1AP_GNBDUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBDUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Active_Cells_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Active_Cells_List;

  for (i=0;
       i<1;
       i++) {
        //
        F1AP_Active_Cells_ItemIEs_t *active_cells_item_ies;
        active_cells_item_ies = (F1AP_Active_Cells_ItemIEs_t *)calloc(1, sizeof(F1AP_Active_Cells_ItemIEs_t));
        active_cells_item_ies->id            = F1AP_ProtocolIE_ID_id_Active_Cells_Item;
        active_cells_item_ies->criticality   = F1AP_Criticality_reject;
        active_cells_item_ies->value.present = F1AP_Active_Cells_ItemIEs__value_PR_Active_Cells_Item;
        
        F1AP_Active_Cells_Item_t active_cells_item;
        memset((void *)&active_cells_item, 0, sizeof(F1AP_Active_Cells_Item_t));

        /* 3.1 oldNRCGI */
        F1AP_NRCGI_t nRCGI;
        MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i],
                                         &nRCGI.pLMN_Identity);
        NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
        active_cells_item.nRCGI = nRCGI;
        
        /* ADD */
        active_cells_item_ies->value.choice.Active_Cells_Item = active_cells_item;

        ASN_SEQUENCE_ADD(&ie->value.choice.Active_Cells_List.list, 
                         active_cells_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
  }

  printf("\n");

  //du_f1ap_itti_send_sctp_data_req(instance, f1ap_setup_req->assoc_id, buffer, len, 0);

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }
}



void DU_handle_gNB_DU_CONFIGURATION_FAILURE(instance_t instance,
                    F1AP_GNBDUConfigurationUpdateFailure_t GNBDUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                    F1AP_GNBDUConfigurationUpdateAcknowledge_t GNBDUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_handle_gNB_CU_CONFIGURATION_UPDATE(instance_t instance,
                    F1AP_GNBCUConfigurationUpdate_t *GNBCUConfigurationUpdate) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_gNB_CU_CONFIGURATION_UPDATE_FAILURE(instance_t instance,
                    F1AP_GNBCUConfigurationUpdateFailure_t *GNBCUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                    F1AP_GNBCUConfigurationUpdateAcknowledge_t *GNBCUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_send_gNB_DU_RESOURCE_COORDINATION_REQUEST(instance_t instance,
                    F1AP_GNBDUResourceCoordinationRequest_t *GNBDUResourceCoordinationRequest) {
  AssertFatal(0, "Not implemented yet\n");
}

void DU_handle_gNB_DU_RESOURCE_COORDINATION_RESPONSE(instance_t instance,
                    F1AP_GNBDUResourceCoordinationResponse_t *GNBDUResourceCoordinationResponse) {
  AssertFatal(0, "Not implemented yet\n");
}
