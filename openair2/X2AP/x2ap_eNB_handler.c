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

/* Handlers matrix. Only eNB related procedure present here */
x2ap_message_decoded_callback x2ap_messages_callback[][3] = {
  { x2ap_eNB_handle_handover_preparation, x2ap_eNB_handle_handover_response, 0 }, /* handoverPreparation */
  { x2ap_eNB_handle_handover_cancel, 0, 0 }, /* handoverCancel */
  { 0, 0, 0 }, /* loadIndication */
  { 0, 0, 0 }, /* errorIndication */
  { 0, 0, 0 }, /* snStatusTransfer */
  { x2ap_eNB_handle_ue_context_release, 0, 0 }, /* uEContextRelease */
  { x2ap_eNB_handle_x2_setup_request, x2ap_eNB_handle_x2_setup_response, x2ap_eNB_handle_x2_setup_failure }, /* x2Setup */
  { 0, 0, 0 }, /* reset */
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
  { 0, 0, 0 }, /* seNBAdditionPreparation */
  { 0, 0, 0 }, /* seNBReconfigurationCompletion */
  { 0, 0, 0 }, /* meNBinitiatedSeNBModificationPreparation */
  { 0, 0, 0 }, /* seNBinitiatedSeNBModification */
  { 0, 0, 0 }, /* meNBinitiatedSeNBRelease */
  { 0, 0, 0 }, /* seNBinitiatedSeNBRelease */
  { 0, 0, 0 }, /* seNBCounterCheck */
  { 0, 0, 0 }  /* retrieveUEContext */
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

        message_p = itti_alloc_new_message(TASK_X2AP, X2AP_DEREGISTERED_ENB_IND);
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

      message_p = itti_alloc_new_message(TASK_X2AP, X2AP_REGISTER_ENB_CNF);
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
    if (pdu.choice.initiatingMessage.procedureCode > sizeof(x2ap_messages_callback) / (3 * sizeof(
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
    if (pdu.choice.successfulOutcome.procedureCode > sizeof(x2ap_messages_callback) / (3 * sizeof(
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
    if (pdu.choice.unsuccessfulOutcome.procedureCode > sizeof(x2ap_messages_callback) / (3 * sizeof(
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
  if (ie->value.choice.ServedCells.list.count > 0) {
    x2ap_eNB_data->num_cc = ie->value.choice.ServedCells.list.count;
    for (int i=0; i<ie->value.choice.ServedCells.list.count;i++) {
      servedCellMember = (ServedCells__Member *)ie->value.choice.ServedCells.list.array[i];
      x2ap_eNB_data->Nid_cell[i] = servedCellMember->servedCellInfo.pCI;
    }
  }

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

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

  if (ie->value.choice.ServedCells.list.count > 0) {
    x2ap_eNB_data->num_cc = ie->value.choice.ServedCells.list.count;
    for (int i=0; i<ie->value.choice.ServedCells.list.count;i++) {
      servedCellMember = (ServedCells__Member *)ie->value.choice.ServedCells.list.array[i];
      x2ap_eNB_data->Nid_cell[i] = servedCellMember->servedCellInfo.pCI;
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
    X2AP_WARN("Received X2 setup failure for eNB ... eNB is not ready\n");
  } else {
    X2AP_ERROR("Received x2 setup failure for eNB... please check your parameters\n");
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

  msg = itti_alloc_new_message(TASK_X2AP, X2AP_HANDOVER_REQ);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequest_IEs_t, ie, x2HandoverRequest,
                             X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID, true);
  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
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

  TBCD_TO_MCC_MNC(&ie->value.choice.ECGI.pLMN_Identity, X2AP_HANDOVER_REQ(msg).ue_gummei.mcc,
                  X2AP_HANDOVER_REQ(msg).ue_gummei.mnc, X2AP_HANDOVER_REQ(msg).ue_gummei.mnc_len);
  OCTET_STRING_TO_INT8(&ie->value.choice.GUMMEI.mME_Code, X2AP_HANDOVER_REQ(msg).ue_gummei.mme_code);
  OCTET_STRING_TO_INT16(&ie->value.choice.GUMMEI.gU_Group_ID.mME_Group_ID, X2AP_HANDOVER_REQ(msg).ue_gummei.mme_group_id);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequest_IEs_t, ie, x2HandoverRequest,
                             X2AP_ProtocolIE_ID_id_UE_ContextInformation, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
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

  if (c->size > 1024 /* TODO: this is the size of rrc_buffer in struct x2ap_handover_req_ack_s*/)
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

  msg = itti_alloc_new_message(TASK_X2AP, X2AP_HANDOVER_REQ_ACK);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequestAcknowledge_IEs_t, ie, x2HandoverRequestAck,
                             X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_source = ie->value.choice.UE_X2AP_ID;

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_HandoverRequestAcknowledge_IEs_t, ie, x2HandoverRequestAck,
                             X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_target = ie->value.choice.UE_X2AP_ID_1;

  ue_id = id_source;

  if (ue_id != x2ap_find_id_from_id_source(&instance_p->id_manager, id_source)) {
    X2AP_WARN("incorrect/unknown X2AP IDs for UE (old ID %d new ID %d), ignoring handover response\n",
              id_source, id_target);
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
                             X2AP_ProtocolIE_ID_id_TargeteNBtoSource_eNBTransparentContainer, true);

  X2AP_TargeteNBtoSource_eNBTransparentContainer_t *c = &ie->value.choice.TargeteNBtoSource_eNBTransparentContainer;

  if (c->size > 1024 /* TODO: this is the size of rrc_buffer in struct x2ap_handover_req_ack_s*/)
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

  msg = itti_alloc_new_message(TASK_X2AP, X2AP_UE_CONTEXT_RELEASE);

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_UEContextRelease_IEs_t, ie, x2UEContextRelease,
                             X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_source = ie->value.choice.UE_X2AP_ID;

  X2AP_FIND_PROTOCOLIE_BY_ID(X2AP_UEContextRelease_IEs_t, ie, x2UEContextRelease,
                             X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID, true);

  if (ie == NULL ) {
    X2AP_ERROR("%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  id_target = ie->value.choice.UE_X2AP_ID_1;

  ue_id = id_source;
  if (ue_id != x2ap_find_id_from_id_source(&instance_p->id_manager, id_source)) {
    X2AP_WARN("incorrect/unknown X2AP IDs for UE (old ID %d new ID %d), ignoring UE context release\n",
              id_source, id_target);
    return 0;
  }

  if (id_target != x2ap_id_get_id_target(&instance_p->id_manager, ue_id)) {
    X2AP_ERROR("UE context release: bad id_target for UE %x (id_source %d) expected %d got %d, ignoring message\n",
               x2ap_id_get_rnti(&instance_p->id_manager, ue_id),
               id_source,
               x2ap_id_get_id_target(&instance_p->id_manager, ue_id),
               id_target);
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

  msg = itti_alloc_new_message(TASK_X2AP, X2AP_HANDOVER_CANCEL);

  X2AP_HANDOVER_CANCEL(msg).rnti = x2ap_id_get_rnti(&instance_p->id_manager, ue_id);
  X2AP_HANDOVER_CANCEL(msg).cause = cause;

  itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

  x2ap_release_id(&instance_p->id_manager, ue_id);

  return 0;
}
