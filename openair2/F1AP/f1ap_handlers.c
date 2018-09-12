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

/*! \file f1ap_handlers.c
 * \brief f1ap messages handlers
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include <stdint.h>

#include "intertask_interface.h"

#include "asn1_conversions.h"

#include "f1ap_common.h"
// #include "f1ap_eNB.h"
#include "f1ap_handlers.h"
#include "f1ap_decoder.h"

#include "f1ap_default_values.h"

#include "assertions.h"
#include "conversions.h"
#include "msc.h"

extern f1ap_setup_req_t *f1ap_du_data_from_du;


static
int f1ap_handle_f1_setup_request(uint32_t               assoc_id,
                                 uint32_t               stream,
                                 F1AP_F1AP_PDU_t        *pdu);

static
int f1ap_handle_f1_setup_response(uint32_t               assoc_id,
                                 uint32_t               stream,
                                 F1AP_F1AP_PDU_t        *pdu);

/* Handlers matrix. Only f1 related procedure present here */
f1ap_message_decoded_callback f1ap_messages_callback[][3] = {
  

  { 0, 0, 0 }, /* Reset */
  { f1ap_handle_f1_setup_request, f1ap_handle_f1_setup_response, 0 }, /* F1Setup */
  { 0, 0, 0 }, /* ErrorIndication */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* gNBDUConfigurationUpdate */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* gNBCUConfigurationUpdate */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* UEContextSetup */
  { 0, 0, 0 }, /* UEContextRelease */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* UEContextModification */
  { 0, 0, 0 }, /* UEContextModificationRequired */
  { 0, 0, 0 }, /* UEMobilityCommand */
  { 0, 0, 0 }, /* UEContextReleaseRequest */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* InitialULRRCMessageTransfer */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* DLRRCMessageTransfer */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* ULRRCMessageTransfer */
  { 0, 0, 0 }, /* privateMessage */
  { 0, 0, 0 }, /* UEInactivityNotification */
  { 0, 0, 0 }, /* GNBDUResourceCoordination */
  { 0, 0, 0 }, /* SystemInformationDeliveryCommand */
  { 0, 0, 0 }, /* Paging */
  { 0, 0, 0 }, /* Notify */
  { 0, 0, 0 }, /* WriteReplaceWarning */
  { 0, 0, 0 }, /* PWSCancel */
  { 0, 0, 0 }, /* PWSRestartIndication */
  { 0, 0, 0 }, /* PWSFailureIndication */
};

const char *f1ap_direction2String(int f1ap_dir) {
static const char *f1ap_direction_String[] = {
  "", /* Nothing */
  "Initiating message", /* initiating message */
  "Successfull outcome", /* successfull outcome */
  "UnSuccessfull outcome", /* successfull outcome */
};
return(f1ap_direction_String[f1ap_dir]);
}

int f1ap_handle_message(uint32_t assoc_id, int32_t stream,
                            const uint8_t * const data, const uint32_t data_length)
{
  F1AP_F1AP_PDU_t pdu;
  int ret;

  DevAssert(data != NULL);

  memset(&pdu, 0, sizeof(pdu));

  if (f1ap_decode_pdu(&pdu, data, data_length) < 0) {
    LOG_E(F1AP, "Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (pdu.choice.initiatingMessage->procedureCode > sizeof(f1ap_messages_callback) / (3 * sizeof(
        f1ap_message_decoded_callback))
      || (pdu.present > F1AP_F1AP_PDU_PR_unsuccessfulOutcome)) {
    LOG_E(F1AP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu.choice.initiatingMessage->procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (f1ap_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1] == NULL) {
    LOG_E(F1AP, "[SCTP %d] No handler for procedureCode %ld in %s\n",
                assoc_id, pdu.choice.initiatingMessage->procedureCode,
               f1ap_direction2String(pdu.present - 1));
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }

  /* Calling the right handler */
  ret = (*f1ap_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])
        (assoc_id, stream, &pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
  return ret;
}

static
int f1ap_handle_f1_setup_request(uint32_t               assoc_id,
                                 uint32_t               stream,
                                 F1AP_F1AP_PDU_t       *pdu)
{
  printf("f1ap_handle_f1_setup_request\n");
  
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

  message_p = itti_alloc_new_message(TASK_RRC_ENB, F1AP_SETUP_REQ); 
  
  /* assoc_id */
  F1AP_SETUP_REQ(message_p).assoc_id = assoc_id;
  
  /* gNB_DU_id */
  // this function exits if the ie is mandatory
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_ID, true);
  asn_INTEGER2ulong(&ie->value.choice.GNB_DU_ID, &F1AP_SETUP_REQ(message_p).gNB_DU_id);
  printf("F1AP_SETUP_REQ(message_p).gNB_DU_id %lu \n", F1AP_SETUP_REQ(message_p).gNB_DU_id);

  /* gNB_DU_name */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                              F1AP_ProtocolIE_ID_id_gNB_DU_Name, true);
  F1AP_SETUP_REQ(message_p).gNB_DU_name = calloc(ie->value.choice.GNB_DU_Name.size + 1, sizeof(char));
  memcpy(F1AP_SETUP_REQ(message_p).gNB_DU_name, ie->value.choice.GNB_DU_Name.buf,
         ie->value.choice.GNB_DU_Name.size);
  /* Convert the mme name to a printable string */
  F1AP_SETUP_REQ(message_p).gNB_DU_name[ie->value.choice.GNB_DU_Name.size] = '\0';
  printf ("F1AP_SETUP_REQ(message_p).gNB_DU_name %s \n", F1AP_SETUP_REQ(message_p).gNB_DU_name);

  /* GNB_DU_Served_Cells_List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_F1SetupRequestIEs_t, ie, container,
                              F1AP_ProtocolIE_ID_id_gNB_DU_Served_Cells_List, true);
  F1AP_SETUP_REQ(message_p).num_cells_available = ie->value.choice.GNB_DU_Served_Cells_List.list.count;
  printf ("F1AP_SETUP_REQ(message_p).num_cells_available %d \n", F1AP_SETUP_REQ(message_p).num_cells_available);

  int num_cells_available = F1AP_SETUP_REQ(message_p).num_cells_available;

  for (i=0; i<num_cells_available; i++) {
    F1AP_GNB_DU_Served_Cells_Item_t *served_celles_item_p;

    served_celles_item_p = &(((F1AP_GNB_DU_Served_Cells_ItemIEs_t *)ie->value.choice.GNB_DU_Served_Cells_List.list.array[i])->value.choice.GNB_DU_Served_Cells_Item);
    
    /* tac */
    // @issue in here
    OCTET_STRING_TO_INT16(&(served_celles_item_p->served_Cell_Information.fiveGS_TAC), F1AP_SETUP_REQ(message_p).tac[i]);
    printf ("F1AP_SETUP_REQ(message_p).tac[%d] %d \n", i, F1AP_SETUP_REQ(message_p).tac[i]);

    /* - nRCGI */
    TBCD_TO_MCC_MNC(&(served_celles_item_p->served_Cell_Information.nRCGI.pLMN_Identity), F1AP_SETUP_REQ(message_p).mcc[i],
                    F1AP_SETUP_REQ(message_p).mnc[i],
                    F1AP_SETUP_REQ(message_p).mnc_digit_length[i]);
    
    // @issue in here cellID
    F1AP_SETUP_REQ(message_p).nr_cellid[i] = 1;
    printf("[SCTP %d] Received nRCGI: MCC %d, MNC %d, CELL_ID %d\n", assoc_id,
               F1AP_SETUP_REQ(message_p).mcc[i],
               F1AP_SETUP_REQ(message_p).mnc[i],
               F1AP_SETUP_REQ(message_p).nr_cellid[i]);
    
    /* - nRPCI */
    F1AP_SETUP_REQ(message_p).nr_pci[i] = served_celles_item_p->served_Cell_Information.nRPCI;
    printf ("F1AP_SETUP_REQ(message_p).nr_pci[%d] %d \n", i, F1AP_SETUP_REQ(message_p).nr_pci[i]);
  
    // System Information
    /* mib */
    F1AP_SETUP_REQ(message_p).mib[i] = calloc(served_celles_item_p->gNB_DU_System_Information->mIB_message.size + 1, sizeof(char));
    memcpy(F1AP_SETUP_REQ(message_p).mib[i], served_celles_item_p->gNB_DU_System_Information->mIB_message.buf,
           served_celles_item_p->gNB_DU_System_Information->mIB_message.size);
    /* Convert the mme name to a printable string */
    F1AP_SETUP_REQ(message_p).mib[i][served_celles_item_p->gNB_DU_System_Information->mIB_message.size] = '\0';
    F1AP_SETUP_REQ(message_p).mib_length[i] = served_celles_item_p->gNB_DU_System_Information->mIB_message.size;
    printf ("F1AP_SETUP_REQ(message_p).mib[%d] %s , len = %d \n", i, F1AP_SETUP_REQ(message_p).mib[i], F1AP_SETUP_REQ(message_p).mib_length[i]);

    /* sib1 */
    F1AP_SETUP_REQ(message_p).sib1[i] = calloc(served_celles_item_p->gNB_DU_System_Information->sIB1_message.size + 1, sizeof(char));
    memcpy(F1AP_SETUP_REQ(message_p).sib1[i], served_celles_item_p->gNB_DU_System_Information->sIB1_message.buf,
           served_celles_item_p->gNB_DU_System_Information->sIB1_message.size);
    /* Convert the mme name to a printable string */
    F1AP_SETUP_REQ(message_p).sib1[i][served_celles_item_p->gNB_DU_System_Information->sIB1_message.size] = '\0';
    F1AP_SETUP_REQ(message_p).sib1_length[i] = served_celles_item_p->gNB_DU_System_Information->sIB1_message.size;
    printf ("F1AP_SETUP_REQ(message_p).sib1[%d] %s , len = %d \n", i, F1AP_SETUP_REQ(message_p).sib1[i], F1AP_SETUP_REQ(message_p).sib1_length[i]);
  }

  
  *f1ap_du_data_from_du = F1AP_SETUP_REQ(message_p);
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

  return itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(assoc_id), message_p);
}

static
int f1ap_handle_f1_setup_response(uint32_t               assoc_id,
                                 uint32_t               stream,
                                 F1AP_F1AP_PDU_t       *pdu)
{
   printf("f1ap_handle_f1_setup_response\n");

   return 0;
}
