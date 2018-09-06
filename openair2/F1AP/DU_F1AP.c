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

/*! \file openair2/F1AP/DU_F1AP.c
* \brief data structures for F1 interface modules
* \author EURECOM/NTUST
* \date 2018
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr, bing-kai.hong@eurecom.fr
* \note
* \warning
*/

#include "conversions.h"
#include "f1ap_common.h"
#include "platform_types.h"
#include "common/utils/LOG/log.h"
#include "sctp_du.h"
#include "intertask_interface.h"


/* This structure describes association of a DU to a CU */
typedef struct f1ap_info {

  module_id_t enb_mod_idP;
  module_id_t du_mod_idP;

  /* Unique eNB_id to identify the eNB within EPC.
   * In our case the eNB is a macro eNB so the id will be 20 bits long.
   * For Home eNB id, this field should be 28 bits long.
   */
  uint32_t GNB_DU_ID;
  
  /* This is the optional name provided by the MME */
  char *GNB_DU_Name;
  f1ap_net_ip_address_t    mme_net_ip_address; // useful for joining assoc_id and ip address of packets

  
  /* Number of input/ouput streams */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Connexion id used between SCTP/S1AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  uint16_t  mcc;
  uint16_t  mnc;
  uint8_t   mnc_digit_length;
  
} f1ap_info_t;

// helper functions 
#define F1AP_TRANSACTION_IDENTIFIER_NUMBER 3
#define F1AP_UE_IDENTIFIER_NUMBER 3
#define NUMBER_OF_eNB_MAX 3

void DU_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp);


uint8_t F1AP_get_next_transaction_identifier(module_id_t enb_mod_idP, module_id_t du_mod_idP) {  
  static uint8_t      transaction_identifier[NUMBER_OF_eNB_MAX];
  transaction_identifier[enb_mod_idP+du_mod_idP] = (transaction_identifier[enb_mod_idP+du_mod_idP] + 1) % F1AP_TRANSACTION_IDENTIFIER_NUMBER;
  //LOG_T(F1AP,"generated xid is %d\n",transaction_identifier[enb_mod_idP+du_mod_idP]);
  return transaction_identifier[enb_mod_idP+du_mod_idP];
}

uint8_t F1AP_get_UE_identifier(module_id_t enb_mod_idP, int CC_idP, int UE_id) {  
  static uint8_t      UE_identifier[NUMBER_OF_eNB_MAX];
  UE_identifier[enb_mod_idP+CC_idP+UE_id] = (UE_identifier[enb_mod_idP+CC_idP+UE_id] + 1) % F1AP_UE_IDENTIFIER_NUMBER;
  //LOG_T(F1AP,"generated xid is %d\n",transaction_identifier[enb_mod_idP+du_mod_idP]);
  return UE_identifier[enb_mod_idP+CC_idP+UE_id];
}

// ==============================================================================
static
void DU_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind)
{
  int result;

  DevAssert(sctp_data_ind != NULL);

  f1ap_handle_message(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                          sctp_data_ind->buffer, sctp_data_ind->buffer_length);

  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

void *F1AP_DU_task(void *arg) {
  printf("Start F1AP DU task!\n");

  //sctp_cu_init();

  MessageDef *received_msg = NULL;
  int         result;

  //F1AP_DEBUG("Starting F1AP at DU\n");

  //f1ap_eNB_prepare_internal_data();

  itti_mark_task_ready(TASK_DU_F1);

  while (1) {
    switch (ITTI_MSG_ID(received_msg)) {

      // case TERMINATE_MESSAGE:
      //   //F1AP_WARN(" *** Exiting F1AP DU thread\n");
      //   itti_exit_task();
      //   break;

      //case F1AP_SETUP_REQ: // this is not a true F1 message, but rather an ITTI message sent by enb_app
        // 1. save the itti msg so that you can use it to sen f1ap_setup_req
        // 2. send a sctp_association req
        // DU_send_sctp_association_req(ITTI_MESSAGE_GET_INSTANCE(received_msg),
        //                               &F1AP_SETUP_REQ(received_msg));
      //  break;

      case SCTP_NEW_ASSOCIATION_RESP:
        // 1. store the respon
        // 2. send the f1setup_req
        DU_handle_sctp_association_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                        &received_msg->ittiMsg.sctp_new_association_resp);
        break;

      case SCTP_DATA_IND: 
        DU_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
        break;

      default:
        // F1AP_ERROR("DU Received unhandled message: %d:%s\n",
        //          ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
        break;
    } // switch
    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

    received_msg = NULL;
  } // while

  return NULL;
}

// ==============================================================================

void DU_send_sctp_association_req(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp)
{
  //
  AssertFatal(0,"Not implemented yet\n");
}

void DU_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp)
{
  //
  AssertFatal(0,"Not implemented yet\n");
}


// ==============================================================================


// SETUP REQUEST
void DU_send_F1_SETUP_REQUEST(module_id_t enb_mod_idP, module_id_t du_mod_idP) {
//void DU_send_F1_SETUP_REQUEST(F1SetupRequest_t *F1SetupRequest) {
  F1AP_F1AP_PDU_t          pdu; 
  F1AP_F1SetupRequest_t    *out;
  F1AP_F1SetupRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  // for test
  f1ap_info_t f1ap_info;
  f1ap_info.GNB_DU_ID = 789;
  f1ap_info.GNB_DU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 3;

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
  asn_int642INTEGER(&ie->value.choice.GNB_DU_ID, f1ap_info.GNB_DU_ID);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  
  /* optional */
  /* c3. GNB_DU_Name */
  if (f1ap_info.GNB_DU_Name != NULL) {
    ie = (F1AP_F1SetupRequestIEs_t *)calloc(1, sizeof(F1AP_F1SetupRequestIEs_t));
    ie->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_Name;
    ie->criticality               = F1AP_Criticality_ignore;
    ie->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Name;
    OCTET_STRING_fromBuf(&ie->value.choice.GNB_DU_Name, f1ap_info.GNB_DU_Name,
                         strlen(f1ap_info.GNB_DU_Name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. serverd cells list */
  ie = (F1AP_F1SetupRequestIEs_t *)calloc(1, sizeof(F1AP_F1SetupRequestIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_gNB_DU_Served_Cells_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Served_Cells_List;

  for (i=0;
       i<1;
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
        MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length, &nRCGI.pLMN_Identity);
        

        //INT32_TO_BIT_STRING(123, &nRCGI.nRCellIdentity);
        nRCGI.nRCellIdentity.buf = malloc((36+7)/8);

        nRCGI.nRCellIdentity.size = (36+7)/8;
        nRCGI.nRCellIdentity.bits_unused = 4;

        nRCGI.nRCellIdentity.buf[0] = 123;

        //nRCGI.nRCellIdentity = 15;
        served_cell_information.nRCGI = nRCGI;

        /* - nRPCI */
        served_cell_information.nRPCI = 321;  // int 0..1007

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
            MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length, &broadcastPLMNs_Item->pLMN_Identity);
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
        gnb_du_served_cells_item.served_Cell_Information = served_cell_information; //

        /* 4.1.2 gNB-DU System Information */
        F1AP_GNB_DU_System_Information_t *gNB_DU_System_Information = (F1AP_GNB_DU_System_Information_t *)calloc(1, sizeof(F1AP_GNB_DU_System_Information_t));

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->mIB_message,  // sept. 2018
                             "1",
                             sizeof("1"));

        OCTET_STRING_fromBuf(&gNB_DU_System_Information->sIB1_message,  // sept. 2018
                             "1",
                             sizeof("1"));
        gnb_du_served_cells_item.gNB_DU_System_Information = gNB_DU_System_Information; //

        /* ADD */
        gnb_du_served_cell_list_item_ies->value.choice.GNB_DU_Served_Cells_Item = gnb_du_served_cells_item;

        ASN_SEQUENCE_ADD(&ie->value.choice.GNB_DU_Served_Cells_List.list, 
                        gnb_du_served_cell_list_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //printf("F1 setup request present = %d\n", ie.value.present);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
  }

  f1ap_du_send_message(buffer, len);
  // printf("\n");

  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  // int req;
  // req = s1ap_eNB_encode_s1_setup_request(&ie, &buffer, &len);

  //printf("encode F1 setup request, req = %d \n", req); 

  //f1ap_eNB_itti_send_sctp_data_req(instance_p->instance, f1ap_mme_data_p->assoc_id, buffer, len, 0);
  /* send Not Use ITTI */
  //f1ap_send_sctp_data_req(instance_p->instance, f1ap_mme_data_p->assoc_id, buffer, len, 0);

}

// ==============================================================================

int f1ap_encode_pdu(F1AP_F1AP_PDU_t *pdu, uint8_t **buffer, uint32_t *length) {
  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  DevAssert(length != NULL);


  xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, pdu);


  ssize_t    encoded;
  
  // asn_DEF_F1AP_F1SetupRequest
  // asn_DEF_F1AP_F1AP_PDU

  /* We can safely free list of IE from sptr */
  //ASN_STRUCT_FREE_CONTENTS_ONLY(&asn_DEF_F1AP_F1SetupRequest, pdu);


  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_F1AP_F1AP_PDU, 0, 
                 pdu,
                 (void **)buffer)) < 0) {
    printf("encoded len = %ld\n", encoded);
    return -1;
  }

  printf("encoded len = %ld\n", encoded);
  printf("buffer = \n");
  
  int i_ret;
  for (i_ret = 0; i_ret < encoded; i_ret++) {
      printf("%x ", *((*buffer)+i_ret));
  }
  printf("\n");
  *length = encoded;
  //ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, pdu);
  return encoded;
}


// int f1ap_decode_pdu(F1AP_F1AP_PDU_t *message, uint8_t *buffer, uint32_t length) {

//   //LOG_I(F1AP,"Entering main loop of DU F1AP pdu receiver\n");
//   F1AP_F1AP_PDU_t pdu;
//   F1AP_F1AP_PDU_t *pdu_p = &pdu;
//   asn_dec_rval_t dec_ret;
  
//   DevAssert(buffer != NULL);

//   printf("buffer = \n");
//   int i_ret;
//   for (i_ret = 0; i_ret < length; i_ret++) {
//       printf("%x ", *(buffer+i_ret));
//   }
//   printf("\n");

//   memset((void *)pdu_p, 0, sizeof(F1AP_F1AP_PDU_t));
  

//   dec_ret = aper_decode(NULL,
//                         &asn_DEF_F1AP_F1AP_PDU,
//                         (void **)&pdu_p,
//                         buffer,
//                         length,
//                         0,
//                         0);

//   xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, pdu_p);

//   printf("\n");

//   printf("dec_ret.code = %d \n", dec_ret.code);

//   AssertFatal(dec_ret.code == RC_OK,"Failed to decode pdu\n");

//   // switch(pdu_p->present) {
//   //   case F1AP_F1AP_PDU_PR_initiatingpdu:
//   //     return F1AP_DU_decode_initiating_pdu(&pdu_p->choice.initiatingpdu);
      
//   //   case F1AP_F1AP_PDU_PR_successfulOutcome:
//   //     return F1AP_DU_decode_successful_outcome(&pdu_p->choice.successfulOutcome);
      
//   //   case F1AP_F1AP_PDU_PR_unsuccessfulOutcome:
//   //     return F1AP_DU_decode_unsuccessful_outcome(&pdu_p->choice.unsuccessfulOutcome);
      
//   //   default:
//   //     /*AssertFatal(1==0,"Unknown presence (%d) or not implemented\n", (int)pdu_p->present);*/
//   //     break;
//   // }
  
//   //AssertFatal(1==0,"Shouldn't get here\n");
//   return -1;
// }


// ssize_t s1ap_generate_initiating_pdu(
//   uint8_t               **buffer,
//   uint32_t               *length,
//   F1AP_ProcedureCode_t    procedureCode,
//   F1AP_Criticality_t      criticality,
//   asn_TYPE_descriptor_t  *td,
//   void                   *sptr)
// {
//   F1AP_F1AP_PDU_t pdu;
//   ssize_t    encoded;

//   memset(&pdu, 0, sizeof(F1AP_F1AP_PDU_t));
  
//   pdu.present = F1AP_F1AP_PDU_PR_initiatingpdu;
//   pdu.choice.initiatingpdu = (F1AP_Initiatingpdu_t *)calloc(1, sizeof(F1AP_Initiatingpdu_t));
//   pdu.choice.initiatingpdu->procedureCode = procedureCode;
//   pdu.choice.initiatingpdu->criticality   = criticality;
//   //ANY_fromType_aper(&pdu.choice.initiatingpdu->value, td, sptr);

//   //if (asn1_xer_print) {
//      xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, (void *)&pdu);
//   //}

//   /* We can safely free list of IE from sptr */
//   ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

//   if ((encoded = aper_encode_to_new_buffer(&asn_DEF_F1AP_F1AP_PDU, 0, &pdu,
//                  (void **)buffer)) < 0) {
//     return -1;
//   }

//   *length = encoded;
//   return encoded;
// }

// int s1ap_eNB_encode_s1_setup_request(
//   F1AP_F1SetupRequestIEs_t *s1SetupRequestIEs,
//   uint8_t                 **buffer,
//   uint32_t                 *length)
// {
//   F1AP_F1SetupRequestIEs_t  s1SetupRequest;
//   F1AP_F1SetupRequestIEs_t *s1SetupRequest_p = &s1SetupRequest;

//   memset((void *)s1SetupRequest_p, 0, sizeof(s1SetupRequest));

//   if (s1ap_encode_s1ap_s1setuprequesties(s1SetupRequest_p, s1SetupRequestIEs) < 0) {
//     return -1;
//   }

//   return s1ap_generate_initiating_pdu(buffer,
//                                           length,
//                                           F1AP_ProcedureCode_id_F1Setup,
//                                           F1AP_Criticality_reject,
//                                           &asn_DEF_F1AP_F1SetupRequest,
//                                           s1SetupRequest_p);
// }




// SETUP SUCCESSFUL
void DU_handle_F1_SETUP_RESPONSE(struct F1AP_F1AP_PDU_t *pdu_p) {
   
  /* decode  */
  //DU_F1AP_decode(args_p);  

  /* handle */
  
  /* save state  */
}

// ==============================================================================

// SETUP FAILURE
void DU_handle_F1_SETUP_FAILURE(struct F1AP_F1AP_PDU_t *pdu_p) {
  //AssertFatal(1==0,"Not implemented yet\n");

  //F1AP_F1SetupFailureIEs_t *f1_setup_failure_p;
  //f1_setup_failure_p = &pdu_p.choice.unsuccessfulOutcome.value.choice.F1SetupFailureIEs.protocolIEs;

}



void DU_send_ERROR_INDICATION(struct F1AP_F1AP_PDU_t *pdu_p) {
  //AssertFatal(1==0,"Not implemented yet\n");
  
  //F1AP_F1ErrorIndicationIEs_t *f1_error_indication_p;
  //f1_error_indication_p = &pdu_p.choice.successfulOutcome.value.choice.F1ErrorIndicationIEs.protocolIEs;
}


void DU_handle_ERROR_INDICATION(F1AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_handle_RESET(F1AP_Reset_t *Reset) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_RESET_ACKKNOWLEDGE(F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_RESET(F1AP_Reset_t *Reset) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_RESET_ACKKNOWLEDGE(F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(F1AP_ULRRCMessageTransfer_t *ULRRCMessageTransfer) {
//void DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER() {
void DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(
  module_id_t     module_idP,
  int             CC_idP,
  int             UE_id,
  rnti_t          rntiP,
  uint8_t        *sduP,
  sdu_size_t      sdu_lenP
)
{
  F1AP_F1AP_PDU_t                       pdu;
  F1AP_InitialULRRCMessageTransfer_t    *out;
  F1AP_InitialULRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

  // for test
  f1ap_info_t f1ap_info;
  f1ap_info.GNB_DU_ID = 789;
  f1ap_info.GNB_DU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 3;

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
  MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                         &nRCGI.pLMN_Identity);
  NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
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
  }

  printf("\n");

  f1ap_du_send_message(buffer, len);
  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  //AssertFatal(1==0,"Not implemented yet\n");
}

//void DU_send_UL_RRC_MESSAGE_TRANSFER(F1AP_ULRRCMessageTransfer_t *ULRRCMessageTransfer) {
void DU_send_UL_RRC_MESSAGE_TRANSFER(void) {
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
  }

  printf("\n");

  f1ap_du_send_message(buffer, len);
  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  //AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_DL_RRC_MESSAGE_TRANSFER(F1AP_DLRRCMessageTransfer_t *DLRRCMessageTransfer) {
  AssertFatal(1==0,"Not implemented yet\n");
}


//void DU_send_gNB_DU_CONFIGURATION_UPDATE(F1AP_GNBDUConfigurationUpdate_t *GNBDUConfigurationUpdate) {
void DU_send_gNB_DU_CONFIGURATION_UPDATE(module_id_t enb_mod_idP, module_id_t du_mod_idP) {
  F1AP_F1AP_PDU_t                     pdu;
  F1AP_GNBDUConfigurationUpdate_t     *out;
  F1AP_GNBDUConfigurationUpdateIEs_t  *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  // for test
  f1ap_info_t f1ap_info;
  f1ap_info.GNB_DU_ID = 789;
  f1ap_info.GNB_DU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 3;
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
  ie->value.choice.TransactionID = F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c2. Served_Cells_To_Add */
  ie = (F1AP_GNBDUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBDUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Add_List;

  for (i=0;
       i<1;
       i++) {
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
        MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length, &nRCGI.pLMN_Identity);
        

        //INT32_TO_BIT_STRING(123, &nRCGI.nRCellIdentity);
        nRCGI.nRCellIdentity.buf = malloc((36+7)/8);

        nRCGI.nRCellIdentity.size = (36+7)/8;
        nRCGI.nRCellIdentity.bits_unused = 4;

        nRCGI.nRCellIdentity.buf[0] = 123;

        //nRCGI.nRCellIdentity = 15;
        served_cell_information.nRCGI = nRCGI;

        /* - nRPCI */
        served_cell_information.nRPCI = 321;  // int 0..1007

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
            MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length, &broadcastPLMNs_Item->pLMN_Identity);
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
        MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                         &oldNRCGI.pLMN_Identity);
        NR_CELL_ID_TO_BIT_STRING(123456, &oldNRCGI.nRCellIdentity);
        served_cells_to_modify_item.oldNRCGI = oldNRCGI;


        /* 3.2.1 serverd cell Information */
        F1AP_Served_Cell_Information_t served_cell_information;
        memset((void *)&served_cell_information, 0, sizeof(F1AP_Served_Cell_Information_t));

        /* - nRCGI */
        F1AP_NRCGI_t nRCGI;
        MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                         &nRCGI.pLMN_Identity);
        NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

        served_cell_information.nRCGI = nRCGI;

        /* - nRPCI */
        served_cell_information.nRPCI = 321;  // int 0..1007

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
            MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length, &broadcastPLMNs_Item->pLMN_Identity);
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
        MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
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
        MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
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

  //f1ap_du_send_message(buffer, len);

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }
}

void DU_handle_gNB_DU_CONFIGURATION_FAILURE(F1AP_GNBDUConfigurationUpdateFailure_t GNBDUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(F1AP_GNBDUConfigurationUpdateAcknowledge_t GNBDUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_handle_gNB_CU_CONFIGURATION_UPDATE(F1AP_GNBCUConfigurationUpdate_t *GNBCUConfigurationUpdate) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_gNB_CU_CONFIGURATION_UPDATE_FALIURE(F1AP_GNBCUConfigurationUpdateFailure_t *GNBCUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(F1AP_GNBCUConfigurationUpdateAcknowledge_t *GNBCUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_handle_UE_CONTEXT_SETUP_REQUEST(F1AP_UEContextSetupRequest_t *UEContextSetupRequest) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void DU_send_UE_CONTEXT_SETUP_RESPONSE(F1AP_UEContextSetupResponse_t *UEContextSetupResponse) {
void DU_send_UE_CONTEXT_SETUP_RESPONSE(void) {
  F1AP_F1AP_PDU_t                  pdu;
  F1AP_UEContextSetupResponse_t    *out;
  F1AP_UEContextSetupResponseIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  f1ap_info_t f1ap_info;
  f1ap_info.GNB_DU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 8;

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

  OCTET_STRING_fromBuf(&ie->value.choice.DUtoCURRCInformation.cellGroupConfig, "asdsa",
                       strlen("asdsa"));
  /* OPTIONAL */
  if (0) {
    ie->value.choice.DUtoCURRCInformation.measGapConfig = (F1AP_MeasGapConfig_t *)calloc(1, sizeof(F1AP_MeasGapConfig_t));
    OCTET_STRING_fromBuf( ie->value.choice.DUtoCURRCInformation.measGapConfig, "asdsa",
                         strlen("asdsa"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c4. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa",
                         strlen("asdsa"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  // /*  */
  /* c5. DRBs_Setup_List */
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

     /* 5.1 DRBs_Setup_Item */
     F1AP_DRBs_Setup_Item_t drbs_setup_item;
     memset((void *)&drbs_setup_item, 0, sizeof(F1AP_DRBs_Setup_Item_t));

     drbs_setup_item.dRBID = 12;
     
     int j;
     for (j=0;
       j<1;
       j++) {

       F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dLUPTNLInformation_ToBeSetup_Item;
       dLUPTNLInformation_ToBeSetup_Item = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_DLUPTNLInformation_ToBeSetup_Item_t));
       dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
       
       F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

       // F1AP_TransportLayerAddress_t transportLayerAddress;
       // transportLayerAddress.buf = malloc((36+7)/8);
       // transportLayerAddress.size = (36+7)/8;
       // transportLayerAddress.bits_unused = 4;
       // *transportLayerAddress.buf = 123;
       // dLUPTNLInformation_ToBeSetup_Item.dL_GTP_Tunnel_EndPoint.transportLayerAddress = transportLayerAddress;

       TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

       OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                             strlen("1204"));

       dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

       ASN_SEQUENCE_ADD(&drbs_setup_item.dLUPTNLInformation_ToBeSetup_List.list,
                        dLUPTNLInformation_ToBeSetup_Item);
     }

     //   /* ADD */
     drbs_setup_item_ies->value.choice.DRBs_Setup_Item = drbs_setup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_Setup_List.list,
                     drbs_setup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c6. SRBs_FailedToBeSetup_List */
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

     /* 6.1 SRBs_Setup_Item */
     F1AP_SRBs_FailedToBeSetup_Item_t srbs_failedToBeSetup_item;
     memset((void *)&srbs_failedToBeSetup_item, 0, sizeof(F1AP_SRBs_FailedToBeSetup_Item_t));

     srbs_failedToBeSetup_item.sRBID = 13;
     srbs_failedToBeSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     srbs_failedToBeSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;
     srbs_failedToBeSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_cu_ue_f1ap_id;

     //   /* ADD */
     srbs_failedToBeSetup_item_ies->value.choice.SRBs_FailedToBeSetup_Item = srbs_failedToBeSetup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_FailedToBeSetup_List.list,
                     srbs_failedToBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c7. DRBs_FailedToBeSetup_List */
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

     /* 7.1 DRBs_Setup_Item */
     F1AP_DRBs_FailedToBeSetup_Item_t drbs_failedToBeSetup_item;
     memset((void *)&drbs_failedToBeSetup_item, 0, sizeof(F1AP_DRBs_FailedToBeSetup_Item_t));

     drbs_failedToBeSetup_item.dRBID = 14;
     drbs_failedToBeSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     drbs_failedToBeSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;
     drbs_failedToBeSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_cu_ue_f1ap_id;

     //   /* ADD */
     drbs_failedToBeSetup_item_ies->value.choice.DRBs_FailedToBeSetup_Item = drbs_failedToBeSetup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_FailedToBeSetup_List.list,
                     drbs_failedToBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c8. SCell_FailedtoSetup_List */
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

     /* 8.1 DRBs_Setup_Item */
     F1AP_SCell_FailedtoSetup_Item_t sCell_FailedtoSetup_item;
     memset((void *)&sCell_FailedtoSetup_item, 0, sizeof(F1AP_SCell_FailedtoSetup_Item_t));

     /* - nRCGI */
     F1AP_NRCGI_t nRCGI;  // issue here
     MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length, &nRCGI.pLMN_Identity);
//
     // INT32_TO_BIT_STRING(123, &nRCGI.nRCellIdentity);
     // nRCGI.nRCellIdentity.buf = malloc((36+7)/8);

     // nRCGI.nRCellIdentity.size = (36+7)/8;
     // nRCGI.nRCellIdentity.bits_unused = 4;

     // nRCGI.nRCellIdentity.buf[0] = 123;

     //nRCGI.nRCellIdentity = 15;

     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

     sCell_FailedtoSetup_item.sCell_ID = nRCGI;

     sCell_FailedtoSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     sCell_FailedtoSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;
     sCell_FailedtoSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_cu_ue_f1ap_id;

     //   /* ADD */
     sCell_FailedtoSetup_item_ies->value.choice.SCell_FailedtoSetup_Item = sCell_FailedtoSetup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_FailedtoSetup_List.list,
                      sCell_FailedtoSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
 
  // /*  */
  /* c9. CriticalityDiagnostics */
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
    printf("Failed to encode F1 setup request\n");
    return;
  }

  printf("\n");

  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  f1ap_du_send_message(buffer, len);
}

void DU_send_UE_CONTEXT_SETUP_FAILURE(F1AP_UEContextSetupFailure_t UEContextSetupFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_send_UE_CONTEXT_RELEASE_REQUEST(F1AP_UEContextReleaseRequest_t *UEContextReleaseRequest) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_handle_UE_CONTEXT_RELEASE_COMMAND(F1AP_UEContextReleaseCommand_t *UEContextReleaseCommand) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_send_UE_CONTEXT_RELEASE_COMPLETE(F1AP_UEContextReleaseComplete_t *UEContextReleaseComplete) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_UE_CONTEXT_MODIFICATION_REQUEST(F1AP_UEContextModificationRequest_t *UEContextModificationRequest) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(F1AP_UEContextModificationResponse_t *UEContextModificationResponse) {
void DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(void) {
  F1AP_F1AP_PDU_t                        pdu;
  F1AP_UEContextModificationResponse_t    *out;
  F1AP_UEContextModificationResponseIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  f1ap_info_t f1ap_info;
  f1ap_info.GNB_DU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 8;

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

        TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

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

        TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

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
     MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                        &nRCGI.pLMN_Identity);

     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

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
    ie->value.choice.CriticalityDiagnostics.procedureCode = (F1AP_ProcedureCode_t *)calloc(1, sizeof(F1AP_ProcedureCode_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCode = F1AP_ProcedureCode_id_UEContextModification;
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
    printf("Failed to encode F1 setup request\n");
    return;
  }

  printf("\n");

  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  f1ap_du_send_message(buffer, len);

}

void DU_send_UE_CONTEXT_MODIFICATION_FAILURE(F1AP_UEContextModificationFailure_t UEContextModificationFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_UE_CONTEXT_MODIFICATION_REQUIRED(F1AP_UEContextModificationRequired_t *UEContextModificationRequired) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_UE_CONTEXT_MODIFICATION_CONFIRM(F1AP_UEContextModificationConfirm_t UEContextModificationConfirm_t) {
  AssertFatal(1==0,"Not implemented yet\n");
}


static int F1AP_DU_decode_initiating_message(F1AP_InitiatingMessage_t *initiating_p) {

  switch (initiating_p->value.present) {
    
  case F1AP_InitiatingMessage__value_PR_NOTHING: /* No components present */
    AssertFatal(1==0,"Should not receive NOTHING on DU\n");
    break;
    
  case F1AP_InitiatingMessage__value_PR_Reset:
    DU_handle_RESET(&initiating_p->value.choice.Reset);
    break;
  /*case F1AP_Initiatingpdu__value_PR_F1SetupRequest:
    DU_send_F1_SETUP_REQUEST(&initiating_p->value.choice.F1SetupRequest);
    break;*/
  case F1AP_InitiatingMessage__value_PR_GNBDUConfigurationUpdate:
    AssertFatal(1==0,"Should not receive GNBDUConfigurationUpdate on DU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate:
    DU_handle_gNB_CU_CONFIGURATION_UPDATE(&initiating_p->value.choice.GNBCUConfigurationUpdate);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextSetupRequest:
    DU_handle_UE_CONTEXT_SETUP_REQUEST(&initiating_p->value.choice.UEContextSetupRequest);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextReleaseCommand:
    DU_handle_UE_CONTEXT_RELEASE_COMMAND(&initiating_p->value.choice.UEContextReleaseCommand);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextModificationRequest:
    DU_handle_UE_CONTEXT_MODIFICATION_REQUEST(&initiating_p->value.choice.UEContextModificationRequest);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextModificationRequired:
    AssertFatal(1==0,"Should not receive UECONTEXTMODIFICATIONREQUIRED on DU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_ErrorIndication:
    AssertFatal(1==0,"Should not receive ErrorIndication on DU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextReleaseRequest:
    AssertFatal(1==0,"Should not receive UECONTEXTRELEASEREQUEST on DU\n");    
    break;
  case F1AP_InitiatingMessage__value_PR_DLRRCMessageTransfer:
    DU_handle_DL_RRC_Message_TRANSFER(&initiating_p->value.choice.DLRRCMessageTransfer);
    break;
  case F1AP_InitiatingMessage__value_PR_ULRRCMessageTransfer:
    AssertFatal(1==0,"Should not receive ULRRCmessageTRANSFER on DU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_PrivateMessage:
    AssertFatal(1==0,"Should not receive PRIVATEmessage on DU\n");
    break;
  default:
    AssertFatal(1==0,"Shouldn't get here\n");
  }

}

static int F1AP_DU_decode_successful_outcome(F1AP_SuccessfulOutcome_t *successfulOutcome_p) {

    switch (successfulOutcome_p->value.present) {
      
    case F1AP_SuccessfulOutcome__value_PR_NOTHING:    /* No components present */
      AssertFatal(1==0,"Should not received NOTHING!\n");
      break;
    case F1AP_SuccessfulOutcome__value_PR_ResetAcknowledge:
      AssertFatal(1==0,"DU Should not receive ResetAcknowled!\n");
      break;
    case F1AP_SuccessfulOutcome__value_PR_F1SetupResponse:
      AssertFatal(1==0,"DU Should not receive F1SetupResponse!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_GNBDUConfigurationUpdateAcknowledge:
      DU_handle_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(successfulOutcome_p->value.choice.GNBDUConfigurationUpdateAcknowledge);
      break;
    case F1AP_SuccessfulOutcome__value_PR_GNBCUConfigurationUpdateAcknowledge:
      AssertFatal(1==0,"DU Should not receive GNBCUConfigurationUpdateAcknowledge!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextSetupResponse:
      AssertFatal(1==0,"DU Should not receive UEContextSetupResponse!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextReleaseComplete:
      AssertFatal(1==0,"DU Should not receive UEContextReleaseComplete!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextModificationResponse:
      AssertFatal(1==0,"DU Should not receive UEContextModificationResponse!\n"); 
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextModificationConfirm:
      DU_handle_UE_CONTEXT_MODIFICATION_CONFIRM(successfulOutcome_p->value.choice.UEContextModificationConfirm);
      break;  
    }
}

static int F1AP_DU_decode_unsuccessful_outcome(F1AP_UnsuccessfulOutcome_t *unSuccessfulOutcome_p) {

  switch (unSuccessfulOutcome_p->value.present) {
    
  case F1AP_UnsuccessfulOutcome__value_PR_NOTHING:
    AssertFatal(1==0,"Should not receive NOTHING!\n");
    break;  /* No components present */
  case F1AP_UnsuccessfulOutcome__value_PR_F1SetupFailure:
    AssertFatal(1==0,"Should not receive F1SetupFailure\n");
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_GNBDUConfigurationUpdateFailure:
    DU_handle_gNB_DU_CONFIGURATION_FAILURE(unSuccessfulOutcome_p->value.choice.GNBDUConfigurationUpdateFailure);
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_GNBCUConfigurationUpdateFailure:
    AssertFatal(1==0,"Should not receive GNBCUConfigurationUpdateFailure\n");
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_UEContextSetupFailure:
    DU_send_UE_CONTEXT_SETUP_FAILURE(unSuccessfulOutcome_p->value.choice.UEContextSetupFailure);
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_UEContextModificationFailure:
    DU_send_UE_CONTEXT_MODIFICATION_FAILURE(unSuccessfulOutcome_p->value.choice.UEContextModificationFailure);
    break;
  }

}

static int F1AP_DU_encode_initiating_message(F1AP_InitiatingMessage_t *initiating_p) {
  return -1;
}

static int F1AP_DU_encode_successful_outcome(F1AP_SuccessfulOutcome_t *successfulOutcome_p)
{
  return -1;
}

static int F1AP_DU_encode_unsuccessful_outcome(F1AP_UnsuccessfulOutcome_t *unSuccessfulOutcome_p)
{
  return -1;
}

#define MAX_F1AP_BUFFER_SIZE 4096


// or init function

void DU_F1AP_init(void* args_p ) {

}
