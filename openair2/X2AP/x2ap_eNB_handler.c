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
#include <stdint.h>

#include "intertask_interface.h"

#include "asn1_conversions.h"

#include "x2ap_common.h"
#include "x2ap_eNB_defs.h"
#include "x2ap_eNB_handler.h"
#include "x2ap_eNB_decoder.h"

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

/* Handlers matrix. Only eNB related procedure present here */
x2ap_message_decoded_callback x2ap_messages_callback[][3] = {
  { 0, 0, 0 }, /* handoverPreparation */
  { 0, 0, 0 }, /* handoverCancel */
  { 0, 0, 0 }, /* loadIndication */
  { 0, 0, 0 }, /* errorIndication */
  { 0, 0, 0 }, /* snStatusTransfer */
  { 0, 0, 0 }, /* uEContextRelease */
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

void x2ap_handle_x2_setup_message(x2ap_eNB_data_t *enb_desc_p, int sctp_shutdown)
{
  if (sctp_shutdown) {
    /* A previously connected eNB has been shutdown */

    /* TODO check if it was used by some eNB and send a message to inform these eNB if there is no more associated eNB */
    if (enb_desc_p->state == X2AP_ENB_STATE_CONNECTED) {
      enb_desc_p->state = X2AP_ENB_STATE_DISCONNECTED;

      if (enb_desc_p->x2ap_eNB_instance-> x2_target_enb_associated_nb > 0) {
        /* Decrease associated eNB number */
        enb_desc_p->x2ap_eNB_instance-> x2_target_enb_associated_nb --;
      }

      /* If there are no more associated eNB, inform eNB app */
      if (enb_desc_p->x2ap_eNB_instance->x2_target_enb_associated_nb == 0) {
        MessageDef                 *message_p;

        message_p = itti_alloc_new_message(TASK_X2AP, X2AP_DEREGISTERED_ENB_IND);
        X2AP_DEREGISTERED_ENB_IND(message_p).nb_x2 = 0;
        itti_send_msg_to_task(TASK_ENB_APP, enb_desc_p->x2ap_eNB_instance->instance, message_p);
      }
    }
  } else {
    /* Check that at least one setup message is pending */
    DevCheck(enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb > 0,
             enb_desc_p->x2ap_eNB_instance->instance,
             enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb, 0);

    if (enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb > 0) {
      /* Decrease pending messages number */
      enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb --;
    }

    /* If there are no more pending messages, inform eNB app */
    if (enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb == 0) {
      MessageDef                 *message_p;

      message_p = itti_alloc_new_message(TASK_X2AP, X2AP_REGISTER_ENB_CNF);
      X2AP_REGISTER_ENB_CNF(message_p).nb_x2 = enb_desc_p->x2ap_eNB_instance->x2_target_enb_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, enb_desc_p->x2ap_eNB_instance->instance, message_p);
    }
  }
}


int x2ap_eNB_handle_message(instance_t instance, uint32_t assoc_id, int32_t stream,
                                const uint8_t *const data, const uint32_t data_length)
{
  X2AP_X2AP_PDU_t pdu;
  int ret;

  DevAssert(data != NULL);

  memset(&pdu, 0, sizeof(pdu));

  if (x2ap_eNB_decode_pdu(&pdu, data, data_length) < 0) {
    X2AP_ERROR("Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (pdu.choice.initiatingMessage.procedureCode > sizeof(x2ap_messages_callback) / (3 * sizeof(
        x2ap_message_decoded_callback))
      || (pdu.present > X2AP_X2AP_PDU_PR_unsuccessfulOutcome)) {
    X2AP_ERROR("[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu.choice.initiatingMessage.procedureCode, pdu.present);
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

      DevAssert(x2ap_eNB_data->x2ap_eNB_instance != NULL);
      x2ap_eNB_generate_x2_setup_failure (x2ap_eNB_data->x2ap_eNB_instance->instance,
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

  return x2ap_eNB_generate_x2_setup_response(x2ap_eNB_data);
}

static
int x2ap_eNB_handle_x2_setup_response(instance_t instance,
                                      uint32_t assoc_id,
                                      uint32_t stream,
                                      X2AP_X2AP_PDU_t *pdu)
{

  X2AP_X2SetupResponse_t              *x2SetupResponse;
  X2AP_X2SetupResponse_IEs_t          *ie;

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

  /* Optionaly set the target eNB name */

  /* The association is now ready as source and target eNBs know parameters of each other.
   * Mark the association as connected.
   */
  x2ap_eNB_data->state = X2AP_ENB_STATE_READY;
  x2ap_eNB_data->x2ap_eNB_instance->x2_target_enb_associated_nb ++;
  x2ap_handle_x2_setup_message(x2ap_eNB_data, 0);

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


  // need a FSM to handle all cases
  if ((ie->value.choice.Cause.present == X2AP_Cause_PR_misc) &&
      (ie->value.choice.Cause.choice.misc == X2AP_CauseMisc_unspecified)) {
    X2AP_WARN("Received X2 setup failure for eNB ... eNB is not ready\n");
  } else {
    X2AP_ERROR("Received x2 setup failure for eNB... please check your parameters\n");
  }

  x2ap_eNB_data->state = X2AP_ENB_STATE_WAITING;
  x2ap_handle_x2_setup_message(x2ap_eNB_data, 0);

  return 0;
}
