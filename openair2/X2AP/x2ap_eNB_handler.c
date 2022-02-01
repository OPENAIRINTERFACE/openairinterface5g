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

/*! \file x2ap_eNB_handler.c
 * \brief x2ap handler procedures for eNB
 * \author Konstantinos Alexandris <Konstantinos.Alexandris@eurecom.fr>, Cedric Roux <Cedric.Roux@eurecom.fr>, Navid Nikaein <Navid.Nikaein@eurecom.fr>
 * \date 2018
 * \version 1.0
 */

#include <stdint.h>

#include "intertask_interface.h"

#include "asn1_conversions.h"

#include "x2ap_common.h"
#include "x2ap_eNB_defs.h"
#include "x2ap_eNB_handler.h"
#include "x2ap_eNB_decoder.h"
#include "x2ap_ids.h"

#include "x2ap_eNB_management_procedures.h"
#include "x2ap_eNB_generate_messages.h"

#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "X2AP_FreqBandNrItem.h"

static
int x2ap_eNB_handle_x2_setup_request (instance_t instance,
                                      uint32_t assoc_id,
                                      uint32_t stream,
                                      X2AP_X2AP_PDU_t *pdu);
static
int x2ap_eNB_handle_x2_setup_response (instance_t instance,
                                       uint32_t assoc_id,
                                       uint32_t stream,
                                       X2AP_X2AP_PDU_t *pdu);
static
int x2ap_eNB_handle_x2_reset_request (instance_t instance,
                                      uint32_t assoc_id,
                                      uint32_t stream,
                                      X2AP_X2AP_PDU_t *pdu);
static
int x2ap_eNB_handle_x2_reset_response (instance_t instance,
                                       uint32_t assoc_id,
                                       uint32_t stream,
                                       X2AP_X2AP_PDU_t *pdu);

static
int x2ap_eNB_handle_x2_setup_failure (instance_t instance,
                                      uint32_t assoc_id,
                                      uint32_t stream,
                                      X2AP_X2AP_PDU_t *pdu);

static
int x2ap_eNB_handle_handover_preparation (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);
static
int x2ap_eNB_handle_handover_response (instance_t instance,
                                      uint32_t assoc_id,
                                      uint32_t stream,
                                      X2AP_X2AP_PDU_t *pdu);

static
int x2ap_eNB_handle_ue_context_release (instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        X2AP_X2AP_PDU_t *pdu);

static
int x2ap_eNB_handle_handover_cancel (instance_t instance,
                                     uint32_t assoc_id,
                                     uint32_t stream,
                                     X2AP_X2AP_PDU_t *pdu);

static
int x2ap_eNB_handle_senb_addition_request (instance_t instance,
                                           uint32_t assoc_id,
                                           uint32_t stream,
                                           X2AP_X2AP_PDU_t *pdu);


static
int x2ap_eNB_handle_senb_addition_request_ack (instance_t instance,
                                                       uint32_t assoc_id,
                                                       uint32_t stream,
                                                       X2AP_X2AP_PDU_t *pdu);

static
int x2ap_eNB_handle_senb_addition_request_reject (instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  X2AP_X2AP_PDU_t *pdu);

int x2ap_eNB_handle_ENDC_x2_setup_request(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 X2AP_X2AP_PDU_t *pdu);

int
x2ap_gNB_handle_ENDC_x2_setup_response(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 X2AP_X2AP_PDU_t *pdu);

static
int x2ap_gNB_handle_ENDC_sGNB_addition_request (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);

static
int x2ap_eNB_handle_ENDC_sGNB_addition_response (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);

static
int x2ap_gNB_handle_ENDC_sGNB_reconfiguration_complete (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);

static
int x2ap_gNB_handle_ENDC_sGNB_release_request(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);

static
int x2ap_gNB_handle_ENDC_sGNB_release_request_acknowledge(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);

static
int x2ap_gNB_handle_ENDC_sGNB_release_required(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);

static
int x2ap_gNB_handle_ENDC_sGNB_release_confirm(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu);

/* Handlers matrix. Only eNB related procedure present here. Placement of callback functions according to X2AP_ProcedureCode.h */
x2ap_message_decoded_callback x2ap_messages_callback[][3] = {
  { x2ap_eNB_handle_handover_preparation, x2ap_eNB_handle_handover_response, 0 }, /* handoverPreparation */
  { x2ap_eNB_handle_handover_cancel, 0, 0 }, /* handoverCancel */
  { 0, 0, 0 }, /* loadIndication */
  { 0, 0, 0 }, /* errorIndication */
  { 0, 0, 0 }, /* snStatusTransfer */
  { x2ap_eNB_handle_ue_context_release, 0, 0 }, /* uEContextRelease */
  { x2ap_eNB_handle_x2_setup_request, x2ap_eNB_handle_x2_setup_response, x2ap_eNB_handle_x2_setup_failure }, /* x2Setup */
  { x2ap_eNB_handle_x2_reset_request, x2ap_eNB_handle_x2_reset_response, 0 }, /* reset */
  { 0, 0, 0 }, /* eNBConfigurationUpdate */
  { 0, 0, 0 }, /* resourceStatusReportingInitiation */
  { 0, 0, 0 }, /* resourceStatusReporting */
  { 0, 0, 0 }, /* privateMessage */
  { 0, 0, 0 }, /* mobilitySettingsChange */
  { 0, 0, 0 }, /* rLFIndication */
  { 0, 0, 0 }, /* handoverReport */
  { 0, 0, 0 }, /* cellActivation */
  { 0, 0, 0 }, /* x2Release */
  { 0, 0, 0 }, /* x2APMessageTransfer */
  { 0, 0, 0 }, /* x2Removal */
  { x2ap_eNB_handle_senb_addition_request, x2ap_eNB_handle_senb_addition_request_ack, x2ap_eNB_handle_senb_addition_request_reject }, /* seNBAdditionPreparation */
  { 0, 0, 0 }, /* seNBReconfigurationCompletion */
  { 0, 0, 0 }, /* meNBinitiatedSeNBModificationPreparation */
  { 0, 0, 0 }, /* seNBinitiatedSeNBModification */
  { 0, 0, 0 }, /* meNBinitiatedSeNBRelease */
  { 0, 0, 0 }, /* seNBinitiatedSeNBRelease */
  { 0, 0, 0 }, /* seNBCounterCheck */
  { 0, 0, 0 },  /* retrieveUEContext */
  { x2ap_gNB_handle_ENDC_sGNB_addition_request, x2ap_eNB_handle_ENDC_sGNB_addition_response, 0 }, /*X2AP_ProcedureCode_id_sgNBAdditionPreparation*/
  { x2ap_gNB_handle_ENDC_sGNB_reconfiguration_complete, 0, 0 }, /*X2AP_ProcedureCode_id_sgNBReconfigurationCompletion*/
  { 0, 0, 0 },
  { 0, 0, 0 },
  { x2ap_gNB_handle_ENDC_sGNB_release_request, x2ap_gNB_handle_ENDC_sGNB_release_request_acknowledge, 0 }, /* meNBinitiatedSgNBRelease */
  { x2ap_gNB_handle_ENDC_sGNB_release_required, x2ap_gNB_handle_ENDC_sGNB_release_confirm, 0 }, /* sgNBinitiatedSgNBRelease */

  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { x2ap_eNB_handle_ENDC_x2_setup_request, x2ap_gNB_handle_ENDC_x2_setup_response, 0 }, /*X2AP_ProcedureCode_id_endcX2Setup*/
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 }
};

char *x2ap_direction2String(int x2ap_dir) {
static char *x2ap_direction_String[] = {
  "", /* Nothing */
  "Originating message", /* originating message */
  "Successfull outcome", /* successfull outcome */
  "UnSuccessfull outcome", /* successfull outcome */
};
return(x2ap_direction_String[x2ap_dir]);
}

void x2ap_handle_x2_setup_message(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *enb_desc_p, int sctp_shutdown)
{
  if (sctp_shutdown) {
    /* A previously connected eNB has been shutdown */

    /* TODO check if it was used by some eNB and send a message to inform these eNB if there is no more associated eNB */
    if (enb_desc_p->state == X2AP_ENB_STATE_CONNECTED) {
      enb_desc_p->state = X2AP_ENB_STATE_DISCONNECTED;

      if (instance_p-> x2_target_enb_associated_nb > 0) {
        /* Decrease associated eNB number */
        instance_p-> x2_target_enb_associated_nb --;
      }

      /* If there are no more associated eNB, inform eNB app */
      if (instance_p->x2_target_enb_associated_nb == 0) {
        MessageDef                 *message_p;

        message_p = itti_alloc_new_message(TASK_X2AP, 0, X2AP_DEREGISTERED_ENB_IND);
        X2AP_DEREGISTERED_ENB_IND(message_p).nb_x2 = 0;
        itti_send_msg_to_task(TASK_ENB_APP, instance_p->instance, message_p);
      }
    }
  } else {
    /* Check that at least one setup message is pending */
    DevCheck(instance_p->x2_target_enb_pending_nb > 0,
             instance_p->instance,
             instance_p->x2_target_enb_pending_nb, 0);

    if (instance_p->x2_target_enb_pending_nb > 0) {
      /* Decrease pending messages number */
      instance_p->x2_target_enb_pending_nb --;
    }

    /* If there are no more pending messages, inform eNB app */
    if (instance_p->x2_target_enb_pending_nb == 0) {
      MessageDef                 *message_p;

      message_p = itti_alloc_new_message(TASK_X2AP, 0, X2AP_REGISTER_ENB_CNF);
      X2AP_REGISTER_ENB_CNF(message_p).nb_x2 = instance_p->x2_target_enb_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, instance_p->instance, message_p);
    }
  }
}


int x2ap_eNB_handle_message(instance_t instance, uint32_t assoc_id, int32_t stream,
                                const uint8_t *const data, const uint32_t data_length)
{
  X2AP_X2AP_PDU_t pdu;
  int ret = 0;

  DevAssert(data != NULL);

  memset(&pdu, 0, sizeof(pdu));

  //printf("Data length received: %d\n", data_length);

  if (x2ap_eNB_decode_pdu(&pdu, data, data_length) < 0) {
    X2AP_ERROR("Failed to decode PDU\n");
    return -1;
  }

  switch (pdu.present) {

  case X2AP_X2AP_PDU_PR_initiatingMessage:
    /* Checking procedure Code and direction of message */
    if (pdu.choice.initiatingMessage.procedureCode >= sizeof(x2ap_messages_callback) / (3 * sizeof(
          x2ap_message_decoded_callback))) {
        //|| (pdu.present > X2AP_X2AP_PDU_PR_unsuccessfulOutcome)) {
      X2AP_ERROR("[SCTP %d] Either procedureCode %ld exceed expected\n",
                 assoc_id, pdu.choice.initiatingMessage.procedureCode);
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_X2AP_X2AP_PDU, &pdu);
      return -1;
    }

    /* No handler present.
     * This can mean not implemented or no procedure for eNB (wrong direction).
     */
    if (x2ap_messages_callback[pdu.choice.initiatingMessage.procedureCode][pdu.present - 1] == NULL) {
      X2AP_ERROR("[SCTP %d] No handler for procedureCode %ld in %s\n",
                  assoc_id, pdu.choice.initiatingMessage.procedureCode,
                 x2ap_direction2String(pdu.present - 1));
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_X2AP_X2AP_PDU, &pdu);
      return -1;
    }
    /* Calling the right handler */
    ret = (*x2ap_messages_callback[pdu.choice.initiatingMessage.procedureCode][pdu.present - 1])
        (instance, assoc_id, stream, &pdu);
    break;

  case X2AP_X2AP_PDU_PR_successfulOutcome:
    /* Checking procedure Code and direction of message */
    if (pdu.choice.successfulOutcome.procedureCode >= sizeof(x2ap_messages_callback) / (3 * sizeof(
          x2ap_message_decoded_callback))) {
        //|| (pdu.present > X2AP_X2AP_PDU_PR_unsuccessfulOutcome)) {
      X2AP_ERROR("[SCTP %d] Either procedureCode %ld exceed expected\n",
                 assoc_id, pdu.choice.successfulOutcome.procedureCode);
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_X2AP_X2AP_PDU, &pdu);
      return -1;
    }

    /* No handler present.
     * This can mean not implemented or no procedure for eNB (wrong direction).
     */
    if (x2ap_messages_callback[pdu.choice.successfulOutcome.procedureCode][pdu.present - 1] == NULL) {
      X2AP_ERROR("[SCTP %d] No handler for procedureCode %ld in %s\n",
                  assoc_id, pdu.choice.successfulOutcome.procedureCode,
                 x2ap_direction2String(pdu.present - 1));
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_X2AP_X2AP_PDU, &pdu);
      return -1;
    }
    /* Calling the right handler */
    ret = (*x2ap_messages_callback[pdu.choice.successfulOutcome.procedureCode][pdu.present - 1])
        (instance, assoc_id, stream, &pdu);
    break;

  case X2AP_X2AP_PDU_PR_unsuccessfulOutcome:
    /* Checking procedure Code and direction of message */
    if (pdu.choice.unsuccessfulOutcome.procedureCode >= sizeof(x2ap_messages_callback) / (3 * sizeof(
          x2ap_message_decoded_callback))) {
        //|| (pdu.present > X2AP_X2AP_PDU_PR_unsuccessfulOutcome)) {
      X2AP_ERROR("[SCTP %d] Either procedureCode %ld exceed expected\n",
                 assoc_id, pdu.choice.unsuccessfulOutcome.procedureCode);
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_X2AP_X2AP_PDU, &pdu);
      return -1;
    }

    /* No handler present.
     * This can mean not implemented or no procedure for eNB (wrong direction).
     */
    if (x2ap_messages_callback[pdu.choice.unsuccessfulOutcome.procedureCode][pdu.present - 1] == NULL) {
      X2AP_ERROR("[SCTP %d] No handler for procedureCode %ld in %s\n",
                  assoc_id, pdu.choice.unsuccessfulOutcome.procedureCode,
                  x2ap_direction2String(pdu.present - 1));
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_X2AP_X2AP_PDU, &pdu);
      return -1;
    }
    /* Calling the right handler */
    ret = (*x2ap_messages_callback[pdu.choice.unsuccessfulOutcome.procedureCode][pdu.present - 1])
        (instance, assoc_id, stream, &pdu);
    break;

  default:
    X2AP_ERROR("[SCTP %d] Direction %d exceed expected\n",
               assoc_id, pdu.present);
    break;
  }

  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_X2AP_X2AP_PDU, &pdu);
  return ret;
}

int
x2ap_eNB_handle_x2_setup_request(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 X2AP_X2AP_PDU_t *pdu)
{

  X2AP_X2SetupRequest_t              *x2SetupRequest;
  X2AP_X2SetupRequest_IEs_t          *ie;
  ServedCells__Member                *servedCellMember;

  x2ap_eNB_instance_t                *instance_p;
  x2ap_eNB_data_t                    *x2ap_eNB_data;
  MessageDef                         *msg;
  uint32_t                           eNB_id = 0;

  DevAssert (pdu != NULL);
  x2SetupRequest = &pdu->choice.initiatingMessage.value.choice.X2SetupRequest;

  /*
   * We received a new valid X2 Setup Request on a stream != 0.
   * * * * This should not happen -> reject eNB x2 setup request.
   */

  if (stream != 0) {
    X2AP_ERROR("Received new x2 setup request on stream != 0\n");
      /*
       * Send a x2 setup failure with protocol cause unspecified
       */
    return x2ap_eNB_generate_x2_setup_failure (instance,
                                               assoc_id,
                                               X2AP_Cause_PR_protocol,
                                               X2AP_CauseProtocol_unspecified,
                                               -1);
  }

  X2AP_DEBUG("Received a new X2 setup request\n");

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_X2SetupRequest_IEs_t, ie, x2SetupRequest,
                             X2AP_ProtocolIE_ID_id_GlobalENB_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  } else {
    if (ie->value.choice.GlobalENB_ID.eNB_ID.present == X2AP_ENB_ID_PR_home_eNB_ID) {
    // Home eNB ID = 28 bits
      uint8_t  *eNB_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.home_eNB_ID.buf;

      if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 28) {
      //TODO: handle case were size != 28 -> notify ? reject ?
      }

      eNB_id = (eNB_id_buf[0] << 20) + (eNB_id_buf[1] << 12) + (eNB_id_buf[2] << 4) + ((eNB_id_buf[3] & 0xf0) >> 4);
      X2AP_DEBUG("Home eNB id: %07x\n", eNB_id);
    } else {
    // Macro eNB = 20 bits
      uint8_t *eNB_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf;

      if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 20) {
      //TODO: handle case were size != 20 -> notify ? reject ?
      }

      eNB_id = (eNB_id_buf[0] << 12) + (eNB_id_buf[1] << 4) + ((eNB_id_buf[2] & 0xf0) >> 4);
      X2AP_DEBUG("macro eNB id: %05x\n", eNB_id);
    }
  }

  X2AP_DEBUG("Adding eNB to the list of associated eNBs\n");

  if ((x2ap_eNB_data = x2ap_is_eNB_id_in_list (eNB_id)) == NULL) {
      /*
       * eNB has not been found in list of associated eNB,
       * * * * Add it to the tail of list and initialize data
       */
    if ((x2ap_eNB_data = x2ap_is_eNB_assoc_id_in_list (assoc_id)) == NULL) {
      /*
       * ??
       */
      return -1;
    } else {
      x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
      x2ap_eNB_data->eNB_id = eNB_id;
    }
  } else {
    x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
    /*
     * eNB has been found in list, consider the x2 setup request as a reset connection,
     * * * * reseting any previous UE state if sctp association is != than the previous one
     */
    if (x2ap_eNB_data->assoc_id != assoc_id) {
      /*
       * ??: Send an overload cause...
       */
      X2AP_ERROR("Rejecting x2 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n", eNB_id, x2ap_eNB_data->assoc_id, assoc_id);

      x2ap_eNB_generate_x2_setup_failure (instance,
                                          assoc_id,
                                          X2AP_Cause_PR_protocol,
                                          X2AP_CauseProtocol_unspecified,
                                          -1);
      return -1;
    }
    /*
     * TODO: call the reset procedure
     */
  }

  /* Set proper pci */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_X2SetupRequest_IEs_t, ie, x2SetupRequest,
                             X2AP_ProtocolIE_ID_id_ServedCells, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_SETUP_REQ);

  X2AP_SETUP_REQ(msg).num_cc = ie->value.choice.ServedCells.list.count;

  if (ie->value.choice.ServedCells.list.count > 0) {
    x2ap_eNB_data->num_cc = ie->value.choice.ServedCells.list.count;
    for (int i=0; i<ie->value.choice.ServedCells.list.count;i++) {
      servedCellMember = (ServedCells__Member *)ie->value.choice.ServedCells.list.array[i];
      x2ap_eNB_data->Nid_cell[i] = servedCellMember->servedCellInfo.pCI;
      X2AP_SETUP_REQ(msg).Nid_cell[i] = x2ap_eNB_data->Nid_cell[i];
    }
  }

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  return x2ap_eNB_generate_x2_setup_response(instance_p, x2ap_eNB_data);
}

const char *X2AP_ResetRequest_str[2]={"X2AP_ResetRequest_IEs__value_PR_Cause","X2AP_ResetRequest_IEs__value_PR_InterfaceInstanceIndication"};
const char *X2AP_case_str_radio[50]={"X2AP_CauseRadioNetwork_handover_desirable_for_radio_reasons",
        "X2AP_CauseRadioNetwork_time_critical_handover",
        "X2AP_CauseRadioNetwork_resource_optimisation_handover",
        "X2AP_CauseRadioNetwork_reduce_load_in_serving_cell",
        "X2AP_CauseRadioNetwork_partial_handover",
        "X2AP_CauseRadioNetwork_unknown_new_eNB_UE_X2AP_ID",
        "X2AP_CauseRadioNetwork_unknown_old_eNB_UE_X2AP_ID",
        "X2AP_CauseRadioNetwork_unknown_pair_of_UE_X2AP_ID",
        "X2AP_CauseRadioNetwork_ho_target_not_allowed",
        "X2AP_CauseRadioNetwork_tx2relocoverall_expiry",
        "X2AP_CauseRadioNetwork_trelocprep_expiry",
        "X2AP_CauseRadioNetwork_cell_not_available",
        "X2AP_CauseRadioNetwork_no_radio_resources_available_in_target_cell",
        "X2AP_CauseRadioNetwork_invalid_MME_GroupID",
        "X2AP_CauseRadioNetwork_unknown_MME_Code",
        "X2AP_CauseRadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported",
        "X2AP_CauseRadioNetwork_reportCharacteristicsEmpty",
        "X2AP_CauseRadioNetwork_noReportPeriodicity",
        "X2AP_CauseRadioNetwork_existingMeasurementID",
        "X2AP_CauseRadioNetwork_unknown_eNB_Measurement_ID",
        "X2AP_CauseRadioNetwork_measurement_temporarily_not_available",
        "X2AP_CauseRadioNetwork_unspecified",
        "X2AP_CauseRadioNetwork_load_balancing",
        "X2AP_CauseRadioNetwork_handover_optimisation",
        "X2AP_CauseRadioNetwork_value_out_of_allowed_range",
        "X2AP_CauseRadioNetwork_multiple_E_RAB_ID_instances",
        "X2AP_CauseRadioNetwork_switch_off_ongoing",
        "X2AP_CauseRadioNetwork_not_supported_QCI_value",
        "X2AP_CauseRadioNetwork_measurement_not_supported_for_the_object",
        "X2AP_CauseRadioNetwork_tDCoverall_expiry",
        "X2AP_CauseRadioNetwork_tDCprep_expiry",
        "X2AP_CauseRadioNetwork_action_desirable_for_radio_reasons",
        "X2AP_CauseRadioNetwork_reduce_load",
        "X2AP_CauseRadioNetwork_resource_optimisation",
        "X2AP_CauseRadioNetwork_time_critical_action",
        "X2AP_CauseRadioNetwork_target_not_allowed",
        "X2AP_CauseRadioNetwork_no_radio_resources_available",
        "X2AP_CauseRadioNetwork_invalid_QoS_combination",
        "X2AP_CauseRadioNetwork_encryption_algorithms_not_supported",
        "X2AP_CauseRadioNetwork_procedure_cancelled",
        "X2AP_CauseRadioNetwork_rRM_purpose",
        "X2AP_CauseRadioNetwork_improve_user_bit_rate",
        "X2AP_CauseRadioNetwork_user_inactivity",
        "X2AP_CauseRadioNetwork_radio_connection_with_UE_lost",
        "X2AP_CauseRadioNetwork_bearer_option_not_supported",
        "X2AP_CauseRadioNetwork_mCG_Mobility",
        "X2AP_CauseRadioNetwork_sCG_Mobility",
        "X2AP_CauseRadioNetwork_count_reaches_max_value",
        "X2AP_CauseRadioNetwork_unknown_old_en_gNB_UE_X2AP_ID",
        "X2AP_CauseRadioNetwork_pDCP_Overload"};

const char *X2AP_cause_str_radio[2]={"X2AP_CauseTransport_transport_resource_unavailable",
        "X2AP_CauseTransport_unspecified"};
const char *X2AP_cause_str_protocol[7]={" X2AP_CauseProtocol_transfer_syntax_error",
        "X2AP_CauseProtocol_abstract_syntax_error_reject",
        "X2AP_CauseProtocol_abstract_syntax_error_ignore_and_notify",
        "X2AP_CauseProtocol_message_not_compatible_with_receiver_state",
        "X2AP_CauseProtocol_semantic_error",
        "X2AP_CauseProtocol_unspecified",
        "X2AP_CauseProtocol_abstract_syntax_error_falsely_constructed_message"};
const char *X2AP_cause_str_misc[5]={"X2AP_CauseMisc_control_processing_overload",
        "X2AP_CauseMisc_hardware_failure",
        "X2AP_CauseMisc_om_intervention",
        "X2AP_CauseMisc_not_enough_user_plane_processing_resources",
        "X2AP_CauseMisc_unspecified"};


int
x2ap_eNB_handle_x2_reset_response(instance_t instance,
                                  uint32_t assoc_id,
                                  uint32_t stream,
                                  X2AP_X2AP_PDU_t *pdu)
{

   return (0);
}


int
x2ap_eNB_handle_x2_reset_request(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 X2AP_X2AP_PDU_t *pdu)
{

  X2AP_ResetRequest_t              *ResetRequest;
  X2AP_ResetRequest_IEs_t          *ie;

  x2ap_eNB_instance_t                *instance_p;
  x2ap_eNB_data_t                    *x2ap_eNB_data;
  MessageDef                         *msg;
  uint32_t                           eNB_id = 0;

  DevAssert (pdu != NULL);
  ResetRequest = &pdu->choice.initiatingMessage.value.choice.ResetRequest;

  X2AP_DEBUG("Received a new X2 reset request\n");

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_ResetRequest_IEs_t, ie, ResetRequest,
                             X2AP_ProtocolIE_ID_id_Cause, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  } else {
    AssertFatal(ie->value.present <= X2AP_ResetRequest_IEs__value_PR_InterfaceInstanceIndication && ie->value.present > 0,
                "Cause value %d, is impossible\n",ie->value.present);
    LOG_I(X2AP,"Received X2AP Reset Request with Cause Type %s\n",X2AP_ResetRequest_str[ie->value.present-1]);
  }

  X2AP_DEBUG("Adding eNB to the list of associated eNBs\n");

  if ((x2ap_eNB_data = x2ap_is_eNB_id_in_list (eNB_id)) == NULL) {
      /*
       * eNB has not been found in list of associated eNB,
       * * * * Add it to the tail of list and initialize data
       */
    if ((x2ap_eNB_data = x2ap_is_eNB_assoc_id_in_list (assoc_id)) == NULL) {
      /*
       * ??
       */
      return -1;
    } else {
      x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
      x2ap_eNB_data->eNB_id = eNB_id;
    }
  } else {
    x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
    /*
     * eNB has been found in list, consider the x2 setup request as a reset connection,
     * * * * reseting any previous UE state if sctp association is != than the previous one
     */
    if (x2ap_eNB_data->assoc_id != assoc_id) {
      /*
       * ??: Send an overload cause...
       */
      X2AP_ERROR("Reset Request: eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n", eNB_id, x2ap_eNB_data->assoc_id, assoc_id);

      return -1;
    }
    /*
     * TODO: call the reset procedure
     */
  }

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_RESET_REQ);

  X2AP_RESET_REQ(msg).cause = ie->value.present;


  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  return x2ap_eNB_generate_x2_setup_response(instance_p, x2ap_eNB_data);
}


static
int x2ap_eNB_handle_x2_setup_response(instance_t instance,
                                      uint32_t assoc_id,
                                      uint32_t stream,
                                      X2AP_X2AP_PDU_t *pdu)
{

  X2AP_X2SetupResponse_t              *x2SetupResponse;
  X2AP_X2SetupResponse_IEs_t          *ie;
  ServedCells__Member                 *servedCellMember;

  x2ap_eNB_instance_t                 *instance_p;
  x2ap_eNB_data_t                     *x2ap_eNB_data;
  MessageDef                          *msg;
  uint32_t                            eNB_id = 0;

  DevAssert (pdu != NULL);
  x2SetupResponse = &pdu->choice.successfulOutcome.value.choice.X2SetupResponse;

  /*
   * We received a new valid X2 Setup Response on a stream != 0.
   * * * * This should not happen -> reject eNB x2 setup response.
   */

  if (stream != 0) {
    X2AP_ERROR("Received new x2 setup response on stream != 0\n");
  }

  if ((x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0)) == NULL) {
    X2AP_ERROR("[SCTP %d] Received X2 setup response for non existing "
               "eNB context\n", assoc_id);
    return -1;
  }

  if((x2ap_eNB_data->state == X2AP_ENB_STATE_CONNECTED) ||
     (x2ap_eNB_data->state == X2AP_ENB_STATE_READY))
  
  {
    X2AP_ERROR("Received Unexpexted X2 Setup Response Message\n");
    return -1;
  }

  X2AP_DEBUG("Received a new X2 setup response\n");

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_X2SetupResponse_IEs_t, ie, x2SetupResponse,
                             X2AP_ProtocolIE_ID_id_GlobalENB_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  } 
  if (ie->value.choice.GlobalENB_ID.eNB_ID.present == X2AP_ENB_ID_PR_home_eNB_ID) {
    // Home eNB ID = 28 bits
    uint8_t  *eNB_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.home_eNB_ID.buf;

    if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 28) {
      //TODO: handle case were size != 28 -> notify ? reject ?
    }

    eNB_id = (eNB_id_buf[0] << 20) + (eNB_id_buf[1] << 12) + (eNB_id_buf[2] << 4) + ((eNB_id_buf[3] & 0xf0) >> 4);
    X2AP_DEBUG("Home eNB id: %07x\n", eNB_id);
  } else {
    // Macro eNB = 20 bits
    uint8_t *eNB_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf;

    if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 20) {
      //TODO: handle case were size != 20 -> notify ? reject ?
    }

    eNB_id = (eNB_id_buf[0] << 12) + (eNB_id_buf[1] << 4) + ((eNB_id_buf[2] & 0xf0) >> 4);
    X2AP_DEBUG("macro eNB id: %05x\n", eNB_id);
  }

  X2AP_DEBUG("Adding eNB to the list of associated eNBs\n");

  if ((x2ap_eNB_data = x2ap_is_eNB_id_in_list (eNB_id)) == NULL) {
      /*
       * eNB has not been found in list of associated eNB,
       * * * * Add it to the tail of list and initialize data
       */
    if ((x2ap_eNB_data = x2ap_is_eNB_assoc_id_in_list (assoc_id)) == NULL) {
      /*
       * ??: Send an overload cause...
       */
      return -1;
    } else {
      x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
      x2ap_eNB_data->eNB_id = eNB_id;
    }
  } else {
    x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
    /*
     * TODO: call the reset procedure
     */
  }

  /* Set proper pci */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_X2SetupResponse_IEs_t, ie, x2SetupResponse,
                             X2AP_ProtocolIE_ID_id_ServedCells, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_SETUP_RESP);

  X2AP_SETUP_RESP(msg).num_cc = ie->value.choice.ServedCells.list.count;

  if (ie->value.choice.ServedCells.list.count > 0) {
    x2ap_eNB_data->num_cc = ie->value.choice.ServedCells.list.count;
    for (int i=0; i<ie->value.choice.ServedCells.list.count;i++) {
      servedCellMember = (ServedCells__Member *)ie->value.choice.ServedCells.list.array[i];
      x2ap_eNB_data->Nid_cell[i] = servedCellMember->servedCellInfo.pCI;
      X2AP_SETUP_RESP(msg).Nid_cell[i] = x2ap_eNB_data->Nid_cell[i];
    }
  }

  /* Optionaly set the target eNB name */

  /* The association is now ready as source and target eNBs know parameters of each other.
   * Mark the association as connected.
   */
  x2ap_eNB_data->state = X2AP_ENB_STATE_READY;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  instance_p->x2_target_enb_associated_nb ++;
  x2ap_handle_x2_setup_message(instance_p, x2ap_eNB_data, 0);

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  return 0;
}

static
int x2ap_eNB_handle_x2_setup_failure(instance_t instance,
                                     uint32_t assoc_id,
                                     uint32_t stream,
                                     X2AP_X2AP_PDU_t *pdu)
{

  X2AP_X2SetupFailure_t              *x2SetupFailure;
  X2AP_X2SetupFailure_IEs_t          *ie;

  x2ap_eNB_instance_t                *instance_p;
  x2ap_eNB_data_t                    *x2ap_eNB_data;

  DevAssert(pdu != NULL);

  x2SetupFailure = &pdu->choice.unsuccessfulOutcome.value.choice.X2SetupFailure;

  /*
   * We received a new valid X2 Setup Failure on a stream != 0.
   * * * * This should not happen -> reject eNB x2 setup failure.
  */

  if (stream != 0) {
    X2AP_WARN("[SCTP %d] Received x2 setup failure on stream != 0 (%d)\n",
    assoc_id, stream);
  }

  if ((x2ap_eNB_data = x2ap_get_eNB (NULL, assoc_id, 0)) == NULL) {
    X2AP_ERROR("[SCTP %d] Received X2 setup failure for non existing "
    "eNB context\n", assoc_id);
    return -1;
  }

  if((x2ap_eNB_data->state == X2AP_ENB_STATE_CONNECTED) ||
     (x2ap_eNB_data->state == X2AP_ENB_STATE_READY))
  
  {
    X2AP_ERROR("Received Unexpexted X2 Setup Failure Message\n");
    return -1;
  }

  X2AP_DEBUG("Received a new X2 setup failure\n");

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_X2SetupFailure_IEs_t, ie, x2SetupFailure,
                             X2AP_ProtocolIE_ID_id_Cause, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  } 
  // need a FSM to handle all cases
  if ((ie->value.choice.Cause.present == X2AP_Cause_PR_misc) &&
      (ie->value.choice.Cause.choice.misc == X2AP_CauseMisc_unspecified)) {
    X2AP_ERROR("Received X2 setup failure for eNB ... eNB is not ready\n");
    exit(1);
  } else {
    X2AP_ERROR("Received x2 setup failure for eNB... please check your parameters\n");
    exit(1);
  }

  x2ap_eNB_data->state = X2AP_ENB_STATE_WAITING;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  x2ap_handle_x2_setup_message(instance_p, x2ap_eNB_data, 0);

  return 0;
}

static
int x2ap_eNB_handle_handover_preparation (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{

  X2AP_HandoverRequest_t             *x2HandoverRequest;
  X2AP_HandoverRequest_IEs_t         *ie;

  X2AP_E_RABs_ToBeSetup_ItemIEs_t    *e_RABS_ToBeSetup_ItemIEs;
  X2AP_E_RABs_ToBeSetup_Item_t       *e_RABs_ToBeSetup_Item;

  x2ap_eNB_instance_t                *instance_p;
  x2ap_eNB_data_t                    *x2ap_eNB_data;
  MessageDef                         *msg;
  int                                ue_id;

  DevAssert (pdu != NULL);
  x2HandoverRequest = &pdu->choice.initiatingMessage.value.choice.HandoverRequest;

  if (stream == 0) {
    X2AP_ERROR ("Received new x2 handover request on stream == 0\n");
    /* TODO: send a x2 failure response */
    return 0;
  }

  X2AP_DEBUG ("Received a new X2 handover request\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_HANDOVER_REQ);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequest_IEs_t, ie, x2HandoverRequest,
                             X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }

  /* allocate a new X2AP UE ID */
  ue_id = x2ap_allocate_new_id(&instance_p->id_manager);
  if (ue_id == -1) {
    X2AP_ERROR("could not allocate a new X2AP UE ID\n");
    /* TODO: cancel handover: send HO preparation failure to source eNB */
    exit(1);
  }
  /* rnti is unknown yet, must not be set to -1, 0 is fine */
  x2ap_set_ids(&instance_p->id_manager, ue_id, 0, ie->value.choice.UE_X2AP_ID, ue_id);
  x2ap_id_set_state(&instance_p->id_manager, ue_id, X2ID_STATE_TARGET);

  X2AP_HANDOVER_REQ(msg).x2_id = ue_id;

  //X2AP_HANDOVER_REQ(msg).target_physCellId = measResults2->measResultNeighCells->choice.
                                               //measResultListEUTRA.list.array[ncell_index]->physCellId;
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequest_IEs_t, ie, x2HandoverRequest,
                             X2AP_ProtocolIE_ID_id_GUMMEI_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }
  TBCD_TO_MCC_MNC(&ie->value.choice.ECGI.pLMN_Identity, X2AP_HANDOVER_REQ(msg).ue_gummei.mcc,
                  X2AP_HANDOVER_REQ(msg).ue_gummei.mnc, X2AP_HANDOVER_REQ(msg).ue_gummei.mnc_len);
  OCTET_STRING_TO_INT8(&ie->value.choice.GUMMEI.mME_Code, X2AP_HANDOVER_REQ(msg).ue_gummei.mme_code);
  OCTET_STRING_TO_INT16(&ie->value.choice.GUMMEI.gU_Group_ID.mME_Group_ID, X2AP_HANDOVER_REQ(msg).ue_gummei.mme_group_id);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequest_IEs_t, ie, x2HandoverRequest,
                             X2AP_ProtocolIE_ID_id_UE_ContextInformation, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }

  X2AP_HANDOVER_REQ(msg).mme_ue_s1ap_id = ie->value.choice.UE_ContextInformation.mME_UE_S1AP_ID;

  /* TODO: properly store Target Cell ID */

  X2AP_HANDOVER_REQ(msg).target_assoc_id = assoc_id;

  X2AP_HANDOVER_REQ(msg).security_capabilities.encryption_algorithms =
    BIT_STRING_to_uint16(&ie->value.choice.UE_ContextInformation.uESecurityCapabilities.encryptionAlgorithms);
  X2AP_HANDOVER_REQ(msg).security_capabilities.integrity_algorithms =
    BIT_STRING_to_uint16(&ie->value.choice.UE_ContextInformation.uESecurityCapabilities.integrityProtectionAlgorithms);

  //X2AP_HANDOVER_REQ(msg).ue_ambr=ue_context_pP->ue_context.ue_ambr;

  if ((ie->value.choice.UE_ContextInformation.aS_SecurityInformation.key_eNodeB_star.buf) &&
          (ie->value.choice.UE_ContextInformation.aS_SecurityInformation.key_eNodeB_star.size == 32)) {
    memcpy(X2AP_HANDOVER_REQ(msg).kenb, ie->value.choice.UE_ContextInformation.aS_SecurityInformation.key_eNodeB_star.buf, 32);
    X2AP_HANDOVER_REQ(msg).kenb_ncc = ie->value.choice.UE_ContextInformation.aS_SecurityInformation.nextHopChainingCount;
  } else {
    X2AP_WARN ("Size of eNB key star does not match the expected value\n");
  }

  if (ie->value.choice.UE_ContextInformation.e_RABs_ToBeSetup_List.list.count > 0) {

    X2AP_HANDOVER_REQ(msg).nb_e_rabs_tobesetup = ie->value.choice.UE_ContextInformation.e_RABs_ToBeSetup_List.list.count;

    for (int i=0;i<ie->value.choice.UE_ContextInformation.e_RABs_ToBeSetup_List.list.count;i++) {
      e_RABS_ToBeSetup_ItemIEs = (X2AP_E_RABs_ToBeSetup_ItemIEs_t *) ie->value.choice.UE_ContextInformation.e_RABs_ToBeSetup_List.list.array[i];
      e_RABs_ToBeSetup_Item = &e_RABS_ToBeSetup_ItemIEs->value.choice.E_RABs_ToBeSetup_Item;

      X2AP_HANDOVER_REQ(msg).e_rabs_tobesetup[i].e_rab_id = e_RABs_ToBeSetup_Item->e_RAB_ID ;

      memcpy(X2AP_HANDOVER_REQ(msg).e_rabs_tobesetup[i].eNB_addr.buffer,
                     e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.buf,
                     e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size);

      X2AP_HANDOVER_REQ(msg).e_rabs_tobesetup[i].eNB_addr.length =
                      e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size * 8 - e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.bits_unused;

      OCTET_STRING_TO_INT32(&e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.gTP_TEID,
                                                X2AP_HANDOVER_REQ(msg).e_rabs_tobesetup[i].gtp_teid);

      X2AP_HANDOVER_REQ(msg).e_rab_param[i].qos.qci = e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.qCI;
      X2AP_HANDOVER_REQ(msg).e_rab_param[i].qos.allocation_retention_priority.priority_level = e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.priorityLevel;
      X2AP_HANDOVER_REQ(msg).e_rab_param[i].qos.allocation_retention_priority.pre_emp_capability = e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionCapability;
      X2AP_HANDOVER_REQ(msg).e_rab_param[i].qos.allocation_retention_priority.pre_emp_vulnerability = e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionVulnerability;
    }

  }
  else {
    X2AP_ERROR ("Can't decode the e_RABs_ToBeSetup_List \n");
  }

  X2AP_RRC_Context_t *c = &ie->value.choice.UE_ContextInformation.rRC_Context;

  if (c->size > 8192 /* TODO: this is the size of rrc_buffer in struct x2ap_handover_req_s */)
    { printf("%s:%d: fatal: buffer too big\n", __FILE__, __LINE__); abort(); }

  memcpy(X2AP_HANDOVER_REQ(msg).rrc_buffer, c->buf, c->size);
  X2AP_HANDOVER_REQ(msg).rrc_buffer_size = c->size;

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  return 0;
}

static
int x2ap_eNB_handle_handover_response (instance_t instance,
                                       uint32_t assoc_id,
                                       uint32_t stream,
                                       X2AP_X2AP_PDU_t *pdu)
{
  X2AP_HandoverRequestAcknowledge_t             *x2HandoverRequestAck;
  X2AP_HandoverRequestAcknowledge_IEs_t         *ie;
  X2AP_E_RABs_Admitted_ItemIEs_t		 		*e_RABS_Admitted_ItemIEs;
  X2AP_E_RABs_Admitted_Item_t 		   			*e_RABs_Admitted_Item;

  x2ap_eNB_instance_t                           *instance_p;
  x2ap_eNB_data_t                               *x2ap_eNB_data;
  MessageDef                                    *msg;
  int                                           ue_id;
  int                                           id_source;
  int                                           id_target;
  int                                           rnti;

  DevAssert (pdu != NULL);
  x2HandoverRequestAck = &pdu->choice.successfulOutcome.value.choice.HandoverRequestAcknowledge;

  if (stream == 0) {
    X2AP_ERROR ("Received new x2 handover response on stream == 0\n");
    /* TODO: send a x2 failure response */
    return 0;
  }

  X2AP_DEBUG ("Received a new X2 handover response\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_HANDOVER_REQ_ACK);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequestAcknowledge_IEs_t, ie, x2HandoverRequestAck,
                             X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }

  id_source = ie->value.choice.UE_X2AP_ID;

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequestAcknowledge_IEs_t, ie, x2HandoverRequestAck,
                             X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }

  id_target = ie->value.choice.UE_X2AP_ID_1;

  ue_id = id_source;

  if (ue_id != x2ap_find_id_from_id_source(&instance_p->id_manager, id_source)) {
    X2AP_WARN("incorrect/unknown X2AP IDs for UE (old ID %d new ID %d), ignoring handover response\n",
              id_source, id_target);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return 0;
  }

  rnti = x2ap_id_get_rnti(&instance_p->id_manager, ue_id);

  /* id_target is a new information, store it */
  x2ap_set_ids(&instance_p->id_manager, ue_id, rnti, id_source, id_target);
  x2ap_id_set_state(&instance_p->id_manager, ue_id, X2ID_STATE_SOURCE_OVERALL);
  x2ap_set_reloc_overall_timer(&instance_p->id_manager, ue_id,
                               x2ap_timer_get_tti(&instance_p->timers));

  X2AP_HANDOVER_REQ_ACK(msg).rnti = rnti;


  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequestAcknowledge_IEs_t, ie, x2HandoverRequestAck,
                             X2AP_ProtocolIE_ID_id_E_RABs_Admitted_List, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n", __FILE__, __LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }else{
    if (ie->value.choice.E_RABs_Admitted_List.list.count > 0) {
  
      uint8_t nb_e_rabs_tobesetup;
      nb_e_rabs_tobesetup = ie->value.choice.E_RABs_Admitted_List.list.count;
      X2AP_HANDOVER_REQ_ACK(msg).nb_e_rabs_tobesetup = nb_e_rabs_tobesetup;
  
      for (int i=0; i<nb_e_rabs_tobesetup; i++) {
        e_RABS_Admitted_ItemIEs = (X2AP_E_RABs_Admitted_ItemIEs_t *) ie->value.choice.E_RABs_Admitted_List.list.array[i];
	e_RABs_Admitted_Item = &e_RABS_Admitted_ItemIEs->value.choice.E_RABs_Admitted_Item;
  
	X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].e_rab_id = e_RABs_Admitted_Item->e_RAB_ID ;
	X2AP_ERROR("x2u tunnel: index %d e_rab_id %d\n", i, X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].e_rab_id);
		  
	if(e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint == NULL){
	  X2AP_DEBUG("%s %d: X2AP_E_RABs_Admitted_Item_t->dL_GTP_TunnelEndpoint is a NULL pointer \n", __FILE__, __LINE__);
	  continue;
	}
		  
         memcpy(X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].eNB_addr.buffer,
		e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.buf,
		e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.size);
  
	X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].eNB_addr.length = e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.size;
  
	OCTET_STRING_TO_INT32(&e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->gTP_TEID,
			      X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].gtp_teid);
  
  
        X2AP_DEBUG("x2u tunnel: index %d target enb ip %d.%d.%d.%d length %d gtp teid %u\n", 
		    i, 
		    X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].eNB_addr.buffer[0],
		    X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].eNB_addr.buffer[1],
		    X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].eNB_addr.buffer[2],
		    X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].eNB_addr.buffer[3],
		    X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].eNB_addr.length,
	            X2AP_HANDOVER_REQ_ACK(msg).e_rabs_tobesetup[i].gtp_teid);
        }
	
      }

    }
  
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequestAcknowledge_IEs_t, ie, x2HandoverRequestAck,
                             X2AP_ProtocolIE_ID_id_TargeteNBtoSource_eNBTransparentContainer, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }

  X2AP_TargeteNBtoSource_eNBTransparentContainer_t *c = &ie->value.choice.TargeteNBtoSource_eNBTransparentContainer;

  if (c->size > 1024 /* TODO: this is the size of rrc_buffer in struct x2ap_handover_req_ack_s */)
    { printf("%s:%d: fatal: buffer too big\n", __FILE__, __LINE__); abort(); }

  memcpy(X2AP_HANDOVER_REQ_ACK(msg).rrc_buffer, c->buf, c->size);
  X2AP_HANDOVER_REQ_ACK(msg).rrc_buffer_size = c->size;

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);
  return 0;
}


static
int x2ap_eNB_handle_ue_context_release (instance_t instance,
                                        uint32_t assoc_id,
                                        uint32_t stream,
                                        X2AP_X2AP_PDU_t *pdu)
{
  X2AP_UEContextRelease_t             *x2UEContextRelease;
  X2AP_UEContextRelease_IEs_t         *ie;

  x2ap_eNB_instance_t                 *instance_p;
  x2ap_eNB_data_t                     *x2ap_eNB_data;
  MessageDef                          *msg;
  int                                 ue_id;
  int                                 id_source;
  int                                 id_target;

  DevAssert (pdu != NULL);
  x2UEContextRelease = &pdu->choice.initiatingMessage.value.choice.UEContextRelease;

  if (stream == 0) {
    X2AP_ERROR ("Received new x2 ue context release on stream == 0\n");
    /* TODO: send a x2 failure response */
    return 0;
  }

  X2AP_DEBUG ("Received a new X2 ue context release\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_UE_CONTEXT_RELEASE);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_UEContextRelease_IEs_t, ie, x2UEContextRelease,
                             X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }

  id_source = ie->value.choice.UE_X2AP_ID;

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_UEContextRelease_IEs_t, ie, x2UEContextRelease,
                             X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return -1;
  }

  id_target = ie->value.choice.UE_X2AP_ID_1;

  ue_id = id_source;
  if (ue_id != x2ap_find_id_from_id_source(&instance_p->id_manager, id_source)) {
    X2AP_WARN("incorrect/unknown X2AP IDs for UE (old ID %d new ID %d), ignoring UE context release\n",
              id_source, id_target);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return 0;
  }

  if (id_target != x2ap_id_get_id_target(&instance_p->id_manager, ue_id)) {
    X2AP_ERROR("UE context release: bad id_target for UE %x (id_source %d) expected %d got %d, ignoring message\n",
               x2ap_id_get_rnti(&instance_p->id_manager, ue_id),
               id_source,
               x2ap_id_get_id_target(&instance_p->id_manager, ue_id),
               id_target);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return 0;
  }

  X2AP_UE_CONTEXT_RELEASE(msg).rnti = x2ap_id_get_rnti(&instance_p->id_manager, ue_id);

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  x2ap_release_id(&instance_p->id_manager, ue_id);

  return 0;
}

static
int x2ap_eNB_handle_handover_cancel (instance_t instance,
                                     uint32_t assoc_id,
                                     uint32_t stream,
                                     X2AP_X2AP_PDU_t *pdu)
{
  X2AP_HandoverCancel_t             *x2HandoverCancel;
  X2AP_HandoverCancel_IEs_t         *ie;

  x2ap_eNB_instance_t               *instance_p;
  x2ap_eNB_data_t                   *x2ap_eNB_data;
  MessageDef                        *msg;
  int                               ue_id;
  int                               id_source;
  int                               id_target;
  x2ap_handover_cancel_cause_t      cause;

  DevAssert (pdu != NULL);
  x2HandoverCancel = &pdu->choice.initiatingMessage.value.choice.HandoverCancel;

  if (stream == 0) {
    X2AP_ERROR ("Received new x2 handover cancel on stream == 0\n");
    return 0;
  }

  X2AP_DEBUG ("Received a new X2 handover cancel\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverCancel_IEs_t, ie, x2HandoverCancel,
                             X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_source = ie->value.choice.UE_X2AP_ID;

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverCancel_IEs_t, ie, x2HandoverCancel,
                             X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID, false);

  if (ie == NULL ) {
    X2AP_INFO("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    id_target = -1;
  } else
    id_target = ie->value.choice.UE_X2AP_ID_1;

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverCancel_IEs_t, ie, x2HandoverCancel,
                             X2AP_ProtocolIE_ID_id_Cause, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  if (ie->value.present != X2AP_HandoverCancel_IEs__value_PR_Cause ||
      ie->value.choice.Cause.present != X2AP_Cause_PR_radioNetwork ||
      !(ie->value.choice.Cause.choice.radioNetwork ==
            X2AP_CauseRadioNetwork_trelocprep_expiry ||
        ie->value.choice.Cause.choice.radioNetwork ==
            X2AP_CauseRadioNetwork_tx2relocoverall_expiry)) {
    X2AP_ERROR("%s %d: Cause not supported (only T_reloc_prep and TX2_reloc_overall handled)\n",__FILE__,__LINE__);
    return -1;
  }

  switch (ie->value.choice.Cause.choice.radioNetwork) {
  case X2AP_CauseRadioNetwork_trelocprep_expiry:
    cause = X2AP_T_RELOC_PREP_TIMEOUT;
    break;
  case X2AP_CauseRadioNetwork_tx2relocoverall_expiry:
    cause = X2AP_TX2_RELOC_OVERALL_TIMEOUT;
    break;
  default: /* can't come here */ exit(1);
  }

  ue_id = x2ap_find_id_from_id_source(&instance_p->id_manager, id_source);
  if (ue_id == -1) {
    X2AP_WARN("Handover cancel: UE not found (id_source = %d), ignoring message\n", id_source);
    return 0;
  }

  if (id_target != -1 &&
      id_target != x2ap_id_get_id_target(&instance_p->id_manager, ue_id)) {
    X2AP_ERROR("Handover cancel: bad id_target for UE %x (id_source %d) expected %d got %d\n",
               x2ap_id_get_rnti(&instance_p->id_manager, ue_id),
               id_source,
               x2ap_id_get_id_target(&instance_p->id_manager, ue_id),
               id_target);
    exit(1);
  }

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_HANDOVER_CANCEL);

  X2AP_HANDOVER_CANCEL(msg).rnti = x2ap_id_get_rnti(&instance_p->id_manager, ue_id);
  X2AP_HANDOVER_CANCEL(msg).cause = cause;

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  x2ap_release_id(&instance_p->id_manager, ue_id);

  return 0;
}


static
int x2ap_eNB_handle_senb_addition_request (instance_t instance,
                                           uint32_t assoc_id,
                                           uint32_t stream,
                                           X2AP_X2AP_PDU_t *pdu)
{

  X2AP_SeNBAdditionRequest_t             *x2SeNBAdditionRequest;
  X2AP_SeNBAdditionRequest_IEs_t         *ie;

  X2AP_E_RABs_ToBeAdded_ItemIEs_t    *e_RABS_ToBeAdded_ItemIEs;
  X2AP_E_RABs_ToBeAdded_Item_t       *e_RABs_ToBeAdded_Item;

  x2ap_eNB_instance_t                *instance_p;
  //x2ap_eNB_data_t                    *x2ap_eNB_data;
  MessageDef                         *msg;
  //int                                ue_id;

  DevAssert (pdu != NULL);
  x2SeNBAdditionRequest = &pdu->choice.initiatingMessage.value.choice.SeNBAdditionRequest;

  if (stream == 0) {
	  X2AP_ERROR ("Received new x2 senb addition request on stream == 0\n");
	  /* TODO: send a x2 failure response */
      return 0;
  }

  X2AP_DEBUG ("Received a new X2 senb addition request\n");

  //Panos: Do we need this for this message? Where is the x2ap_eNB_data used?
  /*x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);*/

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_SENB_ADDITION_REQ);

  /*MeNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SeNBAdditionRequest_IEs_t, ie, x2SeNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
	  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
	  return -1;
  }

  //Panos: Here not sure which UE ID should be allocated to be forwarded to RRC...
  /* allocate a new X2AP UE ID */
  /*ue_id = x2ap_allocate_new_id(&instance_p->id_manager);
  if (ue_id == -1) {
	  X2AP_ERROR("could not allocate a new X2AP UE ID\n");
      exit(1);
  }

  x2ap_set_ids(&instance_p->id_manager, ue_id, 0, ie->value.choice.UE_X2AP_ID, ue_id);
  x2ap_id_set_state(&instance_p->id_manager, ue_id, X2ID_STATE_TARGET);*/

  //For now we hardcode both MeNB and SeNB UE IDs values
  X2AP_SENB_ADDITION_REQ(msg).x2_MeNB_UE_id = 0;

  /* UESecurityCapabilities is specific to SCGBearerOption.*/
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SeNBAdditionRequest_IEs_t, ie, x2SeNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_UE_SecurityCapabilities, true);

  if (ie == NULL ) {
    	  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    	  return -1;
  }


  X2AP_SENB_ADDITION_REQ(msg).UE_security_capabilities.encryption_algorithms =
  		  BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.encryptionAlgorithms);
  X2AP_SENB_ADDITION_REQ(msg).UE_security_capabilities.integrity_algorithms =
  		  BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.integrityProtectionAlgorithms);


  /* SeNBSecurityKey is specific to SCGBearerOption.*/
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SeNBAdditionRequest_IEs_t, ie, x2SeNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_SeNBSecurityKey, true);

  if (ie == NULL ) {
	  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
	  return -1;
  }

  memcpy(X2AP_SENB_ADDITION_REQ(msg).SeNB_security_key,
		  &ie->value.choice.SeNBSecurityKey.buf,
		  ie->value.choice.SeNBSecurityKey.size);

  /* SeNB to UE Aggregate Maximum Bit Rate */
  //Panos: SeNBUEAggregateMaximumBitRate: should it be added? It is not active in handover_req
  /*X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SeNBAdditionRequest_IEs_t, ie, x2SeNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_SeNBUEAggregateMaximumBitRate, true);*/


  /* E-RABs_To Be Added List */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SeNBAdditionRequest_IEs_t, ie, x2SeNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_E_RABs_ToBeAdded_List, true);

  if (ie == NULL ) {
  	  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
  	  return -1;
    }

  if (ie->value.choice.E_RABs_ToBeAdded_List.list.count > 0) {

	  X2AP_SENB_ADDITION_REQ(msg).total_nb_e_rabs_tobeadded = ie->value.choice.E_RABs_ToBeAdded_List.list.count;
      int sCG_Bearers_cnt = 0;
      int split_Bearers_cnt = 0;
	  for (int i=0;i<ie->value.choice.E_RABs_ToBeAdded_List.list.count;i++) {
		  e_RABS_ToBeAdded_ItemIEs = (X2AP_E_RABs_ToBeAdded_ItemIEs_t *) ie->value.choice.E_RABs_ToBeAdded_List.list. array[i];
		  e_RABs_ToBeAdded_Item = &e_RABS_ToBeAdded_ItemIEs->value.choice.E_RABs_ToBeAdded_Item;

		  if(e_RABs_ToBeAdded_Item->present == X2AP_E_RABs_ToBeAdded_Item_PR_sCG_Bearer){

			  X2AP_SENB_ADDITION_REQ(msg).e_sCG_rabs_tobeadded[sCG_Bearers_cnt].e_rab_id = e_RABs_ToBeAdded_Item->choice.sCG_Bearer.e_RAB_ID;

			  memcpy(X2AP_SENB_ADDITION_REQ(msg).e_sCG_rabs_tobeadded[sCG_Bearers_cnt].eNB_addr.buffer,
        		e_RABs_ToBeAdded_Item->choice.sCG_Bearer.s1_UL_GTPtunnelEndpoint.transportLayerAddress.buf,
        		e_RABs_ToBeAdded_Item->choice.sCG_Bearer.s1_UL_GTPtunnelEndpoint.transportLayerAddress.size);

			  X2AP_SENB_ADDITION_REQ(msg).e_sCG_rabs_tobeadded[sCG_Bearers_cnt].eNB_addr.length =
					  e_RABs_ToBeAdded_Item->choice.sCG_Bearer.s1_UL_GTPtunnelEndpoint.transportLayerAddress.size * 8 - e_RABs_ToBeAdded_Item->choice.sCG_Bearer.s1_UL_GTPtunnelEndpoint.transportLayerAddress.bits_unused;

			  OCTET_STRING_TO_INT32(&e_RABs_ToBeAdded_Item->choice.sCG_Bearer.s1_UL_GTPtunnelEndpoint.gTP_TEID,
        		X2AP_SENB_ADDITION_REQ(msg).e_sCG_rabs_tobeadded[sCG_Bearers_cnt].gtp_teid);

			  X2AP_SENB_ADDITION_REQ(msg).e_sCG_rab_param[sCG_Bearers_cnt].qos.qci = e_RABs_ToBeAdded_Item->choice.sCG_Bearer.e_RAB_Level_QoS_Parameters.qCI;
			  X2AP_SENB_ADDITION_REQ(msg).e_sCG_rab_param[sCG_Bearers_cnt].qos.allocation_retention_priority.priority_level = e_RABs_ToBeAdded_Item->choice.sCG_Bearer.e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.priorityLevel;
			  X2AP_SENB_ADDITION_REQ(msg).e_sCG_rab_param[sCG_Bearers_cnt].qos.allocation_retention_priority.pre_emp_capability = e_RABs_ToBeAdded_Item->choice.sCG_Bearer.e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionCapability;
			  X2AP_SENB_ADDITION_REQ(msg).e_sCG_rab_param[sCG_Bearers_cnt].qos.allocation_retention_priority.pre_emp_vulnerability = e_RABs_ToBeAdded_Item->choice.sCG_Bearer.e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionVulnerability;
			  sCG_Bearers_cnt++;
		  }
		  else if(e_RABs_ToBeAdded_Item->present == X2AP_E_RABs_ToBeAdded_Item_PR_split_Bearer){
			  X2AP_SENB_ADDITION_REQ(msg).e_split_rabs_tobeadded[split_Bearers_cnt].e_rab_id = e_RABs_ToBeAdded_Item->choice.split_Bearer.e_RAB_ID;

			  memcpy(X2AP_SENB_ADDITION_REQ(msg).e_split_rabs_tobeadded[split_Bearers_cnt].eNB_addr.buffer,
					  e_RABs_ToBeAdded_Item->choice.split_Bearer.meNB_GTPtunnelEndpoint.transportLayerAddress.buf,
					  e_RABs_ToBeAdded_Item->choice.split_Bearer.meNB_GTPtunnelEndpoint.transportLayerAddress.size);

			  X2AP_SENB_ADDITION_REQ(msg).e_split_rabs_tobeadded[split_Bearers_cnt].eNB_addr.length =
			  					  e_RABs_ToBeAdded_Item->choice.split_Bearer.meNB_GTPtunnelEndpoint.transportLayerAddress.size * 8 - e_RABs_ToBeAdded_Item->choice.split_Bearer.meNB_GTPtunnelEndpoint.transportLayerAddress.bits_unused;

			  OCTET_STRING_TO_INT32(&e_RABs_ToBeAdded_Item->choice.split_Bearer.meNB_GTPtunnelEndpoint.gTP_TEID,
			          		X2AP_SENB_ADDITION_REQ(msg).e_split_rabs_tobeadded[split_Bearers_cnt].gtp_teid);

			  X2AP_SENB_ADDITION_REQ(msg).e_split_rab_param[split_Bearers_cnt].qos.qci = e_RABs_ToBeAdded_Item->choice.split_Bearer.e_RAB_Level_QoS_Parameters.qCI;
			  X2AP_SENB_ADDITION_REQ(msg).e_split_rab_param[split_Bearers_cnt].qos.allocation_retention_priority.priority_level = e_RABs_ToBeAdded_Item->choice.split_Bearer.e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.priorityLevel;
			  X2AP_SENB_ADDITION_REQ(msg).e_split_rab_param[split_Bearers_cnt].qos.allocation_retention_priority.pre_emp_capability = e_RABs_ToBeAdded_Item->choice.split_Bearer.e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionCapability;
			  X2AP_SENB_ADDITION_REQ(msg).e_split_rab_param[split_Bearers_cnt].qos.allocation_retention_priority.pre_emp_vulnerability = e_RABs_ToBeAdded_Item->choice.split_Bearer.e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionVulnerability;
			  split_Bearers_cnt++;
		  }
	  }

  }
  else {
	  X2AP_ERROR ("Can't decode the e_RABs_ToBeAdded_List \n");
  }

  /*MeNB to SeNB Container */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SeNBAdditionRequest_IEs_t, ie, x2SeNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_MeNBtoSeNBContainer, true);

  if (ie == NULL ) {
  	  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
  	  return -1;
  }

  X2AP_MeNBtoSeNBContainer_t *c = &ie->value.choice.MeNBtoSeNBContainer;

  if (c->size > 1024)
  { printf("%s:%d: fatal: buffer too big\n", __FILE__, __LINE__); abort(); }

  memcpy(X2AP_SENB_ADDITION_REQ(msg).rrc_buffer, c->buf, c->size);
  X2AP_SENB_ADDITION_REQ(msg).rrc_buffer_size = c->size;


  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  return 0;
}

static
int x2ap_eNB_handle_senb_addition_request_ack (instance_t instance,
                                                       uint32_t assoc_id,
                                                       uint32_t stream,
                                                       X2AP_X2AP_PDU_t *pdu)
{
  printf("%s:%d:%s:TODO\n", __FILE__, __LINE__, __FUNCTION__);
  return 0;
}


static
int x2ap_eNB_handle_senb_addition_request_reject (instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  X2AP_X2AP_PDU_t *pdu)
{
  printf("%s:%d:%s:TODO\n", __FILE__, __LINE__, __FUNCTION__);
  return 0;
}

int
x2ap_eNB_handle_ENDC_x2_setup_request(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 X2AP_X2AP_PDU_t *pdu)
{

  X2AP_ENDCX2SetupRequest_t              *x2_ENDC_SetupRequest;
  X2AP_ENDCX2SetupRequest_IEs_t          *ie;
  X2AP_En_gNB_ENDCX2SetupReqIEs_t 			 *ie_GNB_ENDC;
  ServedNRcellsENDCX2ManagementList__Member                *servedCellMember;

  x2ap_eNB_instance_t                *instance_p;
  MessageDef                         *msg;
  x2ap_eNB_data_t                    *x2ap_eNB_data;
  uint32_t                           gNB_id = 0;

  x2ap_eNB_data = NULL;
  DevAssert (pdu != NULL);
  x2_ENDC_SetupRequest = &pdu->choice.initiatingMessage.value.choice.ENDCX2SetupRequest;
  /*
   * We received a new valid X2 Setup Request on a stream != 0.
   * * * * This should not happen -> reject eNB x2 setup request.
   */

  if (stream != 0) {
    X2AP_ERROR("Received new x2 setup request on stream != 0\n");
      /*
       * Send a x2 setup failure with protocol cause unspecified
       */
    // Panos: Here we should be calling an ENDC specific setup_failure function instead
    return x2ap_eNB_generate_x2_setup_failure (instance,
                                               assoc_id,
                                               X2AP_Cause_PR_protocol,
                                               X2AP_CauseProtocol_unspecified,
                                               -1);
  }

  X2AP_DEBUG("Received a new X2 setup request\n");

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_ENDCX2SetupRequest_IEs_t, ie, x2_ENDC_SetupRequest,
		  X2AP_ProtocolIE_ID_id_InitiatingNodeType_EndcX2Setup, true);


  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_ENDC_SETUP_REQ);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  } else {
	  if (ie->value.choice.InitiatingNodeType_EndcX2Setup.choice.init_en_gNB.list.count > 0) {
	    for (int i=0; i<ie->value.choice.InitiatingNodeType_EndcX2Setup.choice.init_en_gNB.list.count;i++) {
	      ie_GNB_ENDC = (X2AP_En_gNB_ENDCX2SetupReqIEs_t*) ie->value.choice.InitiatingNodeType_EndcX2Setup.choice.init_eNB.list.array[i];
	      if (ie_GNB_ENDC == NULL ) {
	        X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
	        return -1;
	      }

	      else if (ie_GNB_ENDC->id == X2AP_ProtocolIE_ID_id_Globalen_gNB_ID) {
	        if (ie_GNB_ENDC->value.choice.GlobalGNB_ID.gNB_ID.choice.gNB_ID.size!= 28) {
	          //TODO: handle case were size != 28 -> notify ? reject ?
	        }
	        OCTET_STRING_TO_INT32(&ie_GNB_ENDC->value.choice.GlobalGNB_ID.gNB_ID.choice.gNB_ID,gNB_id);
	        X2AP_DEBUG("gNB id: %07x\n", gNB_id);

	        X2AP_DEBUG("Adding gNB to the list of associated gNBs\n");
	        if ((x2ap_eNB_data = x2ap_is_eNB_id_in_list (gNB_id)) == NULL) {
	          /*
	           * eNB has not been found in list of associated eNB,
	           * * * * Add it to the tail of list and initialize data
	           */
	          if ((x2ap_eNB_data = x2ap_is_eNB_assoc_id_in_list (assoc_id)) == NULL) {
	            /*
	             * ??
	             */
	            return -1;
	          } else {
	              x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
	              x2ap_eNB_data->eNB_id = gNB_id;
	          }
	        } else {
	            x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
	            /*
	             * eNB has been found in list, consider the x2 setup request as a reset connection,
	             * * * * reseting any previous UE state if sctp association is != than the previous one
	             */
	            if (x2ap_eNB_data->assoc_id != assoc_id) {
	              /*
	               * ??: Send an overload cause...
	               */
	              X2AP_ERROR("Rejecting x2 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n", gNB_id, x2ap_eNB_data->assoc_id, assoc_id);

	              // Here we should be calling an ENDC specific setup_failure function instead
	              x2ap_eNB_generate_x2_setup_failure (instance,
	                  assoc_id,
	                  X2AP_Cause_PR_protocol,
	                  X2AP_CauseProtocol_unspecified,
	                  -1);
	              return -1;
	            }
	            /*
	             *  * TODO: call the reset procedure
	             */
	        }
	      }
	      else if (ie_GNB_ENDC->id == X2AP_ProtocolIE_ID_id_ServedNRcellsENDCX2ManagementList){
	        if (ie_GNB_ENDC->value.choice.ServedNRcellsENDCX2ManagementList.list.count > 0) {
	          x2ap_eNB_data->num_cc = ie_GNB_ENDC->value.choice.ServedNRcellsENDCX2ManagementList.list.count;
	          X2AP_ENDC_SETUP_REQ(msg).num_cc = x2ap_eNB_data->num_cc;
	          for (int i=0; i<ie_GNB_ENDC->value.choice.ServedNRcellsENDCX2ManagementList.list.count;i++) {
	            servedCellMember = (ServedNRcellsENDCX2ManagementList__Member *)ie_GNB_ENDC->value.choice.ServedNRcellsENDCX2ManagementList.list.array[i];
	            x2ap_eNB_data->Nid_cell[i] = servedCellMember->servedNRCellInfo.nrpCI;
	            X2AP_ENDC_SETUP_REQ(msg).Nid_cell[i] = x2ap_eNB_data->Nid_cell[i];
	            //servedCellMember->servedNRCellInfo.nrCellID.pLMN_Identity.buf[0]
	            if (servedCellMember->servedNRCellInfo.fiveGS_TAC != NULL) {
	              X2AP_INFO("TAC: %02x%02x%02x\n",
	                  servedCellMember->servedNRCellInfo.fiveGS_TAC->buf[0],
	                  servedCellMember->servedNRCellInfo.fiveGS_TAC->buf[1],
	                  servedCellMember->servedNRCellInfo.fiveGS_TAC->buf[2]);
	            } else {
	                X2AP_INFO("TAC: (NULL)\n");
	            }

	            X2AP_INFO("PLMN: %02x%02x%02x\n",
	                servedCellMember->servedNRCellInfo.nrCellID.pLMN_Identity.buf[0],
	                servedCellMember->servedNRCellInfo.nrCellID.pLMN_Identity.buf[1],
	                servedCellMember->servedNRCellInfo.nrCellID.pLMN_Identity.buf[2]);
	            if(servedCellMember->servedNRCellInfo.nrModeInfo.choice.tdd.nRFreqInfo.freqBandListNr.list.count > 0){
	                X2AP_FreqBandNrItem_t *FreqBandItem = servedCellMember->servedNRCellInfo.nrModeInfo.choice.tdd.nRFreqInfo.freqBandListNr.list.array[0];
	                x2ap_eNB_data->servedNrCell_band[i] = FreqBandItem->freqBandIndicatorNr;
	                X2AP_ENDC_SETUP_REQ(msg).servedNrCell_band[i] = x2ap_eNB_data->servedNrCell_band[i];
	            }
	          }
	        }
	      }
	    }
	  }
	  else {
	    X2AP_ERROR("%s %d: init_eNB list is empty \n",__FILE__,__LINE__);
	    return -1;
	  }
  }

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  return x2ap_eNB_generate_ENDC_x2_setup_response(instance_p, x2ap_eNB_data);
}

int
x2ap_gNB_handle_ENDC_x2_setup_response(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 X2AP_X2AP_PDU_t *pdu)
{

  X2AP_ENDCX2SetupResponse_t              *x2_ENDC_SetupResponse;
  X2AP_ENDCX2SetupResponse_IEs_t          *ie;
  X2AP_ENB_ENDCX2SetupReqAckIEs_t 			 *ie_ENB_ENDC;
  ServedEUTRAcellsENDCX2ManagementList__Member                *servedCellMember;

  x2ap_eNB_instance_t                *instance_p;
  x2ap_eNB_data_t                    *x2ap_eNB_data;
  uint32_t                           eNB_id = 0;

  x2ap_eNB_data = NULL;
  DevAssert (pdu != NULL);
  x2_ENDC_SetupResponse = &pdu->choice.successfulOutcome.value.choice.ENDCX2SetupResponse;

  /*
   * We received a new valid X2 Setup Request on a stream != 0.
   * * * * This should not happen -> reject eNB x2 setup request.
   */

  if (stream != 0) {
    X2AP_ERROR("Received new x2 setup request on stream != 0\n");
      /*
       * Send a x2 setup failure with protocol cause unspecified
       */
    // Here we should be calling an ENDC specific setup_failure function instead
    return x2ap_eNB_generate_x2_setup_failure (instance,
                                               assoc_id,
                                               X2AP_Cause_PR_protocol,
                                               X2AP_CauseProtocol_unspecified,
                                               -1);
  }

  X2AP_DEBUG("Received a new X2 setup request\n");

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_ENDCX2SetupResponse_IEs_t, ie, x2_ENDC_SetupResponse,
		  X2AP_ProtocolIE_ID_id_RespondingNodeType_EndcX2Setup, true);



  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  } else {
	  if (ie->value.choice.RespondingNodeType_EndcX2Setup.choice.respond_eNB.list.count > 0) {
		  for (int i=0; i<ie->value.choice.RespondingNodeType_EndcX2Setup.choice.respond_eNB.list.count;i++) {

			  ie_ENB_ENDC = (X2AP_ENB_ENDCX2SetupReqAckIEs_t*) ie->value.choice.RespondingNodeType_EndcX2Setup.choice.respond_eNB.list.array[i];
			  if (ie_ENB_ENDC == NULL ) {
				  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
				  return -1;
			  }

			  else if (ie_ENB_ENDC->id == X2AP_ProtocolIE_ID_id_GlobalENB_ID) {
				  if (ie_ENB_ENDC->value.choice.GlobalENB_ID.eNB_ID.present == X2AP_ENB_ID_PR_home_eNB_ID) {
					  // Home eNB ID = 28 bits
					  uint8_t  *eNB_id_buf = ie_ENB_ENDC->value.choice.GlobalENB_ID.eNB_ID.choice.home_eNB_ID.buf;
					  if (ie_ENB_ENDC->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 28) {
						  //TODO: handle case were size != 28 -> notify ? reject ?
					  }
					  eNB_id = (eNB_id_buf[0] << 20) + (eNB_id_buf[1] << 12) + (eNB_id_buf[2] << 4) + ((eNB_id_buf[3] & 0xf0) >> 4);
					  X2AP_DEBUG("Home eNB id: %07x\n", eNB_id);
				  } else {
					  // Macro eNB = 20 bits
					  uint8_t *eNB_id_buf = ie_ENB_ENDC->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf;
					  if (ie_ENB_ENDC->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 20) {
							  //TODO: handle case were size != 20 -> notify ? reject ?
					  }
					  eNB_id = (eNB_id_buf[0] << 12) + (eNB_id_buf[1] << 4) + ((eNB_id_buf[2] & 0xf0) >> 4);
					  X2AP_DEBUG("macro eNB id: %05x\n", eNB_id);
				  }
				  X2AP_DEBUG("Adding eNB to the list of associated eNBs\n");
				  if ((x2ap_eNB_data = x2ap_is_eNB_id_in_list (eNB_id)) == NULL) {
					  /*
				       * eNB has not been found in list of associated eNB,
				       * * * * Add it to the tail of list and initialize data
				       */
					  if ((x2ap_eNB_data = x2ap_is_eNB_assoc_id_in_list (assoc_id)) == NULL) {
						  /*
						   * ??
						   */
						  return -1;
					  } else {
						  x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;
						  x2ap_eNB_data->eNB_id = eNB_id;
					  }
				  } else {
					  x2ap_eNB_data->state = X2AP_ENB_STATE_RESETTING;

					  /*
					   * eNB has been found in list, consider the x2 setup request as a reset connection,
					   * * * * reseting any previous UE state if sctp association is != than the previous one
					   */
					  if (x2ap_eNB_data->assoc_id != assoc_id) {
						  /*
						   * ??: Send an overload cause...
						   */
						  X2AP_ERROR("Rejecting x2 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n", eNB_id, x2ap_eNB_data->assoc_id, assoc_id);

						  // Here we should be calling an ENDC specific setup_failure function instead
						  x2ap_eNB_generate_x2_setup_failure (instance,
				                                          assoc_id,
				                                          X2AP_Cause_PR_protocol,
				                                          X2AP_CauseProtocol_unspecified,
				                                          -1);
						  return -1;
					  }
					  /*
					   *  * TODO: call the reset procedure
					   */
				  }
			  }
			  else if (ie_ENB_ENDC->id == X2AP_ProtocolIE_ID_id_ServedEUTRAcellsENDCX2ManagementList){
				  if (ie_ENB_ENDC->value.choice.ServedEUTRAcellsENDCX2ManagementList.list.count > 0) {
				      x2ap_eNB_data->num_cc = ie_ENB_ENDC->value.choice.ServedEUTRAcellsENDCX2ManagementList.list.count;
				      for (int i=0; i<ie_ENB_ENDC->value.choice.ServedEUTRAcellsENDCX2ManagementList.list.count;i++) {
				    	  servedCellMember = (ServedEUTRAcellsENDCX2ManagementList__Member *)ie_ENB_ENDC->value.choice.ServedEUTRAcellsENDCX2ManagementList.list.array[i];
				    	  x2ap_eNB_data->Nid_cell[i] = servedCellMember->servedEUTRACellInfo.pCI;
				      }
				  }
			  }
		  }
	  }
	  else {
		  X2AP_ERROR("%s %d: init_eNB list is empty \n",__FILE__,__LINE__);
		  return -1;
	  }
  }

  /* Optionaly set the target eNB name */

    /* The association is now ready as source and target eNBs know parameters of each other.
     * Mark the association as connected.
     */
    x2ap_eNB_data->state = X2AP_ENB_STATE_READY;

    instance_p = x2ap_eNB_get_instance(instance);
    DevAssert(instance_p != NULL);

    instance_p->x2_target_enb_associated_nb ++;
    x2ap_handle_x2_setup_message(instance_p, x2ap_eNB_data, 0);

    return 0;
}

static
int x2ap_gNB_handle_ENDC_sGNB_addition_request (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{
  X2AP_SgNBAdditionRequest_t             *x2SgNBAdditionRequest;
  X2AP_SgNBAdditionRequest_IEs_t         *ie;

  X2AP_E_RABs_ToBeAdded_SgNBAddReq_ItemIEs_t 		*e_RABS_ToBeAdded_SgNBAddReq_ItemIEs;
  X2AP_E_RABs_ToBeAdded_SgNBAddReq_Item_t         *e_RABS_ToBeAdded_SgNBAddReq_Item;

  x2ap_eNB_instance_t                *instance_p;
  x2ap_eNB_data_t                    *x2ap_eNB_data;
  MessageDef                         *msg;

  DevAssert (pdu != NULL);
  x2SgNBAdditionRequest = &pdu->choice.initiatingMessage.value.choice.SgNBAdditionRequest;

  /*if (stream == 0) {
    X2AP_ERROR ("Received new x2 SgNB Addition request on stream == 0\n");
    // TODO: send a x2 failure response
    return 0;
  }*/

  X2AP_DEBUG ("Received a new X2 SgNB Addition request\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  X2AP_INFO("X2AP Association id: %d \n",assoc_id);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  //Allocate an ITTI X2AP_SGNB_ADDITION_REQ message instead
  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_ENDC_SGNB_ADDITION_REQ);

  /* X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequest_IEs_t, ie, x2SgNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }
  /* ue_x2_id = MeNB X2AP Id */
  X2AP_ENDC_SGNB_ADDITION_REQ(msg).ue_x2_id = ie->value.choice.UE_X2AP_ID;
  X2AP_ENDC_SGNB_ADDITION_REQ(msg).target_assoc_id = assoc_id;


  /* X2AP_ProtocolIE_ID_id_NRUESecurityCapabilities */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequest_IEs_t, ie, x2SgNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_NRUESecurityCapabilities, true);

  X2AP_ENDC_SGNB_ADDITION_REQ(msg).security_capabilities.encryption_algorithms =
		  BIT_STRING_to_uint16(&ie->value.choice.NRUESecurityCapabilities.nRencryptionAlgorithms);

  X2AP_ENDC_SGNB_ADDITION_REQ(msg).security_capabilities.integrity_algorithms =
  		  BIT_STRING_to_uint16(&ie->value.choice.NRUESecurityCapabilities.nRintegrityProtectionAlgorithms);

  /* X2AP_ProtocolIE_ID_id_SgNBSecurityKey */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequest_IEs_t, ie, x2SgNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_SgNBSecurityKey, true);

  if ((ie->value.choice.SgNBSecurityKey.buf) &&
            (ie->value.choice.SgNBSecurityKey.size == 32)) {
      memcpy(X2AP_ENDC_SGNB_ADDITION_REQ(msg).kgnb, ie->value.choice.SgNBSecurityKey.buf, 32);
    } else {
      X2AP_WARN ("Size of eNB key star does not match the expected value\n");
    }


  /* X2AP_ProtocolIE_ID_id_SgNBUEAggregateMaximumBitRate */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequest_IEs_t, ie, x2SgNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_SgNBUEAggregateMaximumBitRate, true);

  //X2AP_ENDC_SGNB_ADDITION_REQ(msg).ue_ambr.br_dl =ie->value.choice.UEAggregateMaximumBitRate.uEaggregateMaximumBitRateDownlink;
  //X2AP_ENDC_SGNB_ADDITION_REQ(msg).ue_ambr.br_ul = ie->value.choice.UEAggregateMaximumBitRate.uEaggregateMaximumBitRateUplink;

  /* X2AP_ProtocolIE_ID_id_E_RABs_ToBeAdded_SgNBAddReqList */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequest_IEs_t, ie, x2SgNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_E_RABs_ToBeAdded_SgNBAddReqList, true);


  if (ie->value.choice.E_RABs_ToBeAdded_SgNBAddReqList.list.count > 0) {

	  X2AP_ENDC_SGNB_ADDITION_REQ(msg).nb_e_rabs_tobeadded = ie->value.choice.E_RABs_ToBeAdded_SgNBAddReqList.list.count;

    for (int i=0;i<ie->value.choice.E_RABs_ToBeAdded_SgNBAddReqList.list.count;i++) {
      e_RABS_ToBeAdded_SgNBAddReq_ItemIEs = (X2AP_E_RABs_ToBeAdded_SgNBAddReq_ItemIEs_t *) ie->value.choice.E_RABs_ToBeAdded_SgNBAddReqList.list.array[i];
      e_RABS_ToBeAdded_SgNBAddReq_Item = &e_RABS_ToBeAdded_SgNBAddReq_ItemIEs->value.choice.E_RABs_ToBeAdded_SgNBAddReq_Item;

      X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].e_rab_id = e_RABS_ToBeAdded_SgNBAddReq_Item->e_RAB_ID ;
      X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].drb_ID = e_RABS_ToBeAdded_SgNBAddReq_Item->drb_ID;
      if(e_RABS_ToBeAdded_SgNBAddReq_Item->en_DC_ResourceConfiguration.pDCPatSgNB == X2AP_EN_DC_ResourceConfiguration__pDCPatSgNB_present){
    	  X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rab_param[i].qos.qci = e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.full_E_RAB_Level_QoS_Parameters.qCI;
    	  X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rab_param[i].qos.allocation_retention_priority.pre_emp_vulnerability = e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.full_E_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionVulnerability;
    	  X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rab_param[i].qos.allocation_retention_priority.pre_emp_capability = e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.full_E_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionCapability;
    	  X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rab_param[i].qos.allocation_retention_priority.priority_level = e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.full_E_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.priorityLevel;

          memcpy(X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].sgw_addr.buffer,
        		  e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.s1_UL_GTPtunnelEndpoint.transportLayerAddress.buf,
        		  e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.s1_UL_GTPtunnelEndpoint.transportLayerAddress.size);

          X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].sgw_addr.length =
        		  e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.s1_UL_GTPtunnelEndpoint.transportLayerAddress.size * 8 - e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.s1_UL_GTPtunnelEndpoint.transportLayerAddress.bits_unused;

LOG_I(RRC,"x2u tunnel: index %d target sgw ip %d.%d.%d.%d length %d gtp teid %u\n",
              	        		    i,
              	        		X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].sgw_addr.buffer[0],
              	        		X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].sgw_addr.buffer[1],
              	        		X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].sgw_addr.buffer[2],
              	        		X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].sgw_addr.buffer[3],
              	        		X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].sgw_addr.length,
              	        		X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].gtp_teid);

          OCTET_STRING_TO_INT32(&e_RABS_ToBeAdded_SgNBAddReq_Item->resource_configuration.choice.sgNBPDCPpresent.s1_UL_GTPtunnelEndpoint.gTP_TEID,
        		  X2AP_ENDC_SGNB_ADDITION_REQ(msg).e_rabs_tobeadded[i].gtp_teid);
      }

    }

  }
  else {
    X2AP_ERROR ("Can't decode the e_RABs_ToBeSetup_List \n");
  }

  /* X2AP_ProtocolIE_ID_id_MeNBtoSgNBContainer */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequest_IEs_t, ie, x2SgNBAdditionRequest,
		  X2AP_ProtocolIE_ID_id_MeNBtoSgNBContainer, true);

  if (ie->value.choice.MeNBtoSgNBContainer.size > 8192 ) // TODO: this is the size of rrc_buffer in struct x2ap_handover_req_s
    { printf("%s:%d: fatal: buffer too big\n", __FILE__, __LINE__); abort(); }

  memcpy(X2AP_ENDC_SGNB_ADDITION_REQ(msg).rrc_buffer, ie->value.choice.MeNBtoSgNBContainer.buf, ie->value.choice.MeNBtoSgNBContainer.size);
  X2AP_ENDC_SGNB_ADDITION_REQ(msg).rrc_buffer_size = ie->value.choice.MeNBtoSgNBContainer.size;

  itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, msg);

  return 0;
}


static
int x2ap_eNB_handle_ENDC_sGNB_addition_response (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{

	  X2AP_SgNBAdditionRequestAcknowledge_t             *x2SgNBAdditionRequest_ack;
	  X2AP_SgNBAdditionRequestAcknowledge_IEs_t         *ie;

	  X2AP_E_RABs_Admitted_ToBeAdded_SgNBAddReqAck_ItemIEs_t 		*e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_ItemIEs;
	  X2AP_E_RABs_Admitted_ToBeAdded_SgNBAddReqAck_Item_t           *e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item;

	  x2ap_eNB_instance_t                *instance_p;
	  x2ap_eNB_data_t                    *x2ap_eNB_data;
	  MessageDef                         *msg;
	  int                                ue_id;
          int                                id_source;
          int                                id_target;
          int                                rnti;

	  DevAssert (pdu != NULL);
	  x2SgNBAdditionRequest_ack = &pdu->choice.successfulOutcome.value.choice.SgNBAdditionRequestAcknowledge;

	  /*if (stream == 0) {
	    X2AP_ERROR ("Received new x2 SgNB Addition request on stream == 0\n");
	    // TODO: send a x2 failure response
	    return 0;
	  }*/

	  X2AP_DEBUG ("Received a new X2 SgNB Addition request\n");

	  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
	  DevAssert(x2ap_eNB_data != NULL);

	  X2AP_INFO("X2AP Association id: %d \n",assoc_id);

	  instance_p = x2ap_eNB_get_instance(instance);
	  DevAssert(instance_p != NULL);

	  //Allocate an ITTI X2AP_SGNB_ADDITION_REQ message instead
	  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_ENDC_SGNB_ADDITION_REQ_ACK);

	  X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).gnb_x2_assoc_id = assoc_id;

	  /* X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID */
	  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequestAcknowledge_IEs_t, ie, x2SgNBAdditionRequest_ack,
			  X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID, true);
	  if (ie == NULL ) {
	    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
	    return -1;
	  }

	  id_source = ie->value.choice.UE_X2AP_ID;

	  X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).MeNB_ue_x2_id = ie->value.choice.UE_X2AP_ID;


	  /* X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID */
	  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequestAcknowledge_IEs_t, ie, x2SgNBAdditionRequest_ack,
			  X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID, true);
	  if (ie == NULL ) {
		  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
		  return -1;
	  }

	  id_target = ie->value.choice.SgNB_UE_X2AP_ID;

          ue_id = id_source;

          if (ue_id != x2ap_find_id_from_id_source(&instance_p->id_manager, id_source)) {
            X2AP_WARN("incorrect/unknown X2AP IDs for UE (old ID %d new ID %d), ignoring sGNB addition response\n",
                      id_source, id_target);
            itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
            return 0;
          }

          rnti = x2ap_id_get_rnti(&instance_p->id_manager, ue_id);

          /* id_target is a new information, store it */
          x2ap_set_ids(&instance_p->id_manager, ue_id, rnti, id_source, id_target);

	  X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rnti = rnti;

	  X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).SgNB_ue_x2_id = ie->value.choice.SgNB_UE_X2AP_ID;

	  /* X2AP_ProtocolIE_ID_id_E_RABs_ToBeAdded_SgNBAddReqList */
	  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequestAcknowledge_IEs_t, ie, x2SgNBAdditionRequest_ack,
			  X2AP_ProtocolIE_ID_id_E_RABs_Admitted_ToBeAdded_SgNBAddReqAckList, true);

	  if (ie == NULL ) {
		  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
		  return -1;
	  }


	  if (ie->value.choice.E_RABs_Admitted_ToBeAdded_SgNBAddReqAckList.list.count > 0) {

		  X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).nb_e_rabs_admitted_tobeadded = ie->value.choice.E_RABs_Admitted_ToBeAdded_SgNBAddReqAckList.list.count;

	    for (int i=0;i<ie->value.choice.E_RABs_Admitted_ToBeAdded_SgNBAddReqAckList.list.count;i++) {

	    	e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_ItemIEs = (X2AP_E_RABs_Admitted_ToBeAdded_SgNBAddReqAck_ItemIEs_t *) ie->value.choice.E_RABs_Admitted_ToBeAdded_SgNBAddReqAckList.list.array[i];
	    	e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item = &e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_ItemIEs->value.choice.E_RABs_Admitted_ToBeAdded_SgNBAddReqAck_Item;

	    	X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].e_rab_id = e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item->e_RAB_ID ;
	    	if(e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item->en_DC_ResourceConfiguration.pDCPatSgNB == X2AP_EN_DC_ResourceConfiguration__pDCPatSgNB_present){

	          memcpy(X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.buffer,
	        		  e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item->resource_configuration.choice.sgNBPDCPpresent.s1_DL_GTPtunnelEndpoint.transportLayerAddress.buf,
	        		  e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item->resource_configuration.choice.sgNBPDCPpresent.s1_DL_GTPtunnelEndpoint.transportLayerAddress.size);
	          X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.length =
	        		  e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item->resource_configuration.choice.sgNBPDCPpresent.s1_DL_GTPtunnelEndpoint.transportLayerAddress.size * 8 - e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item->resource_configuration.choice.sgNBPDCPpresent.s1_DL_GTPtunnelEndpoint.transportLayerAddress.bits_unused;

	          OCTET_STRING_TO_INT32(&e_RABS_Admitted_ToBeAdded_SgNBAddReqAck_Item->resource_configuration.choice.sgNBPDCPpresent.s1_DL_GTPtunnelEndpoint.gTP_TEID,
	        		  X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gtp_teid);
	      }

	    }

	  }
	  else {
	    X2AP_ERROR ("Can't decode the e_RABs_ToBeSetup_List \n");
	  }

	  /* X2AP_ProtocolIE_ID_id_SgNBtoMeNBContainer */

	  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBAdditionRequestAcknowledge_IEs_t, ie, x2SgNBAdditionRequest_ack,
			  X2AP_ProtocolIE_ID_id_SgNBtoMeNBContainer, true);

	  if (ie == NULL ) {
		  X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
		  return -1;
	  }

	  //X2AP_MeNBtoSgNBContainer_t *container = &ie->value.choice.MeNBtoSgNBContainer;

	    if (ie->value.choice.SgNBtoMeNBContainer.size > 8192 ) // TODO: this is the size of rrc_buffer in struct x2ap_handover_req_s
	      { printf("%s:%d: fatal: buffer too big\n", __FILE__, __LINE__); abort(); }

	    memcpy(X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer, ie->value.choice.SgNBtoMeNBContainer.buf, ie->value.choice.SgNBtoMeNBContainer.size);
	    X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer_size = ie->value.choice.SgNBtoMeNBContainer.size;

	  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

          /* remove UE from x2, DC preparation is over */
          x2ap_release_id(&instance_p->id_manager, ue_id);

	  return 0;
}

static
int x2ap_gNB_handle_ENDC_sGNB_reconfiguration_complete (instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{
  X2AP_SgNBReconfigurationComplete_t     *x2SgNBReconfigurationComplete;
  X2AP_SgNBReconfigurationComplete_IEs_t *ie;

  x2ap_eNB_instance_t                    *instance_p;
  x2ap_eNB_data_t                        *x2ap_eNB_data;
  MessageDef                             *msg;

  int                                    id_target;
  int                                    ue_id;

  DevAssert (pdu != NULL);
  x2SgNBReconfigurationComplete = &pdu->choice.initiatingMessage.value.choice.SgNBReconfigurationComplete;

  /*if (stream == 0) {
    X2AP_ERROR ("Received new x2 SgNB Addition request on stream == 0\n");
    // TODO: send a x2 failure response
    return 0;
  }*/

  X2AP_DEBUG ("Received X2 SgNB Reconfiguration complete message\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  X2AP_INFO("X2AP Association id: %d \n",assoc_id);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  //Allocate an ITTI X2AP_sGNB_reconfiguration_complete message
  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_ENDC_SGNB_RECONF_COMPLETE);

  /* X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBReconfigurationComplete_IEs_t, ie, x2SgNBReconfigurationComplete,
      X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  X2AP_ENDC_SGNB_RECONF_COMPLETE(msg).MeNB_ue_x2_id = ie->value.choice.UE_X2AP_ID;


  /* X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBReconfigurationComplete_IEs_t, ie, x2SgNBReconfigurationComplete,
      X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_target = ie->value.choice.SgNB_UE_X2AP_ID;
  X2AP_ENDC_SGNB_RECONF_COMPLETE(msg).SgNB_ue_x2_id = id_target;

  ue_id = x2ap_find_id_from_id_target(&instance_p->id_manager, id_target);
  if (ue_id == -1) {
    X2AP_WARN("incorrect/unknown X2AP IDs for UE (SgNB_UE_X2AP_ID %d), ignoring sGNB reconfiguration complete\n",
              id_target);
    itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    return 0;
  }

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBReconfigurationComplete_IEs_t, ie, x2SgNBReconfigurationComplete,
      X2AP_ProtocolIE_ID_id_ResponseInformationSgNBReconfComp, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  if (ie->value.choice.ResponseInformationSgNBReconfComp.present != X2AP_ResponseInformationSgNBReconfComp_PR_success_SgNBReconfComp){
    X2AP_ERROR("%s %d: fatal: unsuccessfulSgNB Reconfiguration Complete\n",__FILE__,__LINE__);
    exit(1);
  }

  x2ap_release_id(&instance_p->id_manager, ue_id);

  itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, msg);

  return 0;
}

static
int x2ap_gNB_handle_ENDC_sGNB_release_request(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{
  /* the logic in this function is the following: when receiving a release
   * request, the UE may exist in X2. Or not. If it's not in X2, it can
   * nevertheless be in RRC.
   * The release request message may contain the gNB id. Or not. It's better
   * to look for the UE using the gNB rather than the eNB id. So if we have
   * the gNB id we use it. If not, we use the eNB id.
   */
  X2AP_SgNBReleaseRequest_t     *req;
  X2AP_SgNBReleaseRequest_IEs_t *ie;

  x2ap_eNB_instance_t           *instance_p;
  x2ap_eNB_data_t               *x2ap_eNB_data;
  MessageDef                    *msg;

  int                           id_source;
  int                           id_target;
  int                           rnti;
  int                           ue_id;

  DevAssert (pdu != NULL);
  req = &pdu->choice.initiatingMessage.value.choice.SgNBReleaseRequest;

  /*if (stream == 0) {
    X2AP_ERROR ("Received new x2 SgNB Addition request on stream == 0\n");
    // TODO: send a x2 failure response
    return 0;
  }*/

  X2AP_DEBUG ("Received X2 SgNB Release Request message\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  X2AP_INFO("X2AP Association id: %d \n",assoc_id);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  /* X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBReleaseRequest_IEs_t, ie, req,
                             X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_source = ie->value.choice.UE_X2AP_ID;

  /* X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBReleaseRequest_IEs_t, ie, req,
                             X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID, false);
  if (ie != NULL)
    id_target = ie->value.choice.SgNB_UE_X2AP_ID;
  else
    id_target = -1;

  /* find ue_id, preferably from id_target if id_target is known */
  ue_id = -1;
  if (id_target != -1)
    ue_id = x2ap_find_id_from_id_target(&instance_p->id_manager, id_target);
  if (ue_id == -1)
    ue_id = x2ap_find_id_from_id_source(&instance_p->id_manager, id_source);

  if (ue_id == -1) {
    X2AP_WARN("%s %d: no UE found for id_source %d id_target %d\n",
              __FILE__, __LINE__, id_source, id_target);
  }

  /* the UE may not exist in X2 anymore but exist in RRC
   * in which case its rnti is simply the id_target
   */
  if (ue_id != -1) {
    rnti = x2ap_id_get_rnti(&instance_p->id_manager, ue_id);

    /* remove ue_id */
    x2ap_release_id(&instance_p->id_manager, ue_id);
  } else {
    rnti = id_target;
  }

  if (rnti != -1) {
    msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_ENDC_SGNB_RELEASE_REQUEST);

    X2AP_ENDC_SGNB_RELEASE_REQUEST(msg).rnti = rnti;

    itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, msg);
  }

  /* this call should maybe not be here, we should check that the
   * SgNB Release Request message has been executed properly
   */
  x2ap_gNB_generate_ENDC_x2_SgNB_release_request_acknowledge(instance_p, x2ap_eNB_data,
                                                             id_source,
                                                             rnti != -1 ? rnti : 0);

  return 0;
}

static
int x2ap_gNB_handle_ENDC_sGNB_release_request_acknowledge(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{
  /* nothing to do - may change at some point */
  X2AP_INFO("received release request acknowledge\n");
  return 0;
}

static
int x2ap_gNB_handle_ENDC_sGNB_release_required(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{
  /* the logic in this function is the following: when receiving a release
   * required we don't care about the UE in X2. We simply send a message
   * to RRC with the SgNB id (which is SgNB's RNTI). RRC will look for the
   * UE using this RNTI and deal with it. In the meanwhile the X2 layer
   * immediately replies with "release confirm" no matter what happens
   * in RRC.
   */
  X2AP_SgNBReleaseRequired_t     *req;
  X2AP_SgNBReleaseRequired_IEs_t *ie;

  x2ap_eNB_instance_t           *instance_p;
  x2ap_eNB_data_t               *x2ap_eNB_data;
  MessageDef                    *msg;

  int                           id_source;
  int                           id_target;
  int                           gnb_rnti;

  DevAssert (pdu != NULL);
  req = &pdu->choice.initiatingMessage.value.choice.SgNBReleaseRequired;

  /*if (stream == 0) {
    X2AP_ERROR ("Received new x2 SgNB Addition request on stream == 0\n");
    // TODO: send a x2 failure response
    return 0;
  }*/

  X2AP_DEBUG ("Received X2 SgNB Release Required message\n");

  x2ap_eNB_data = x2ap_get_eNB(NULL, assoc_id, 0);
  DevAssert(x2ap_eNB_data != NULL);

  X2AP_INFO("X2AP Association id: %d \n",assoc_id);

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  /* X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBReleaseRequired_IEs_t, ie, req,
                             X2AP_ProtocolIE_ID_id_MeNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_source = ie->value.choice.UE_X2AP_ID;

  /* X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID */
  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_SgNBReleaseRequired_IEs_t, ie, req,
                             X2AP_ProtocolIE_ID_id_SgNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_target = ie->value.choice.SgNB_UE_X2AP_ID;

  gnb_rnti = id_target;

  msg = itti_alloc_new_message(TASK_X2AP, 0, X2AP_ENDC_SGNB_RELEASE_REQUIRED);
  X2AP_ENDC_SGNB_RELEASE_REQUIRED(msg).gnb_rnti = gnb_rnti;
  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  /* this call should maybe not be here, we should check that the
   * SgNB Release Required message has been executed properly
   */
  x2ap_gNB_generate_ENDC_x2_SgNB_release_confirm(instance_p, x2ap_eNB_data,
                                                 id_source, id_target);

  return 0;
}

static
int x2ap_gNB_handle_ENDC_sGNB_release_confirm(instance_t instance,
                                          uint32_t assoc_id,
                                          uint32_t stream,
                                          X2AP_X2AP_PDU_t *pdu)
{
  /* nothing to do - may change at some point */
  X2AP_INFO("received release confirm\n");
  return 0;
}
