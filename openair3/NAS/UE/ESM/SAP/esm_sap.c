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

/*****************************************************************************
Source      esm_sap.c

Version     0.1

Date        2012/11/22

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the ESM Service Access Points at which the EPS
        Session Management sublayer provides procedures for the
        EPS bearer context handling and resources allocation.

*****************************************************************************/

#include "esm_sap.h"
#include "esm_recv.h"
#include "esm_send.h"
#include "commonDef.h"
#include "nas_log.h"

#include "OctetString.h"
#include "TLVDecoder.h"
#include "esm_msgDef.h"
#include "esm_msg.h"

#include "esm_cause.h"
#include "esm_proc.h"

#include <string.h> // memset, strlen
#include <assert.h>

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static int _esm_sap_recv(nas_user_t *user, int msg_type, bool is_standalone,
                         const OctetString *req, OctetString *rsp, esm_sap_error_t *err);
static int _esm_sap_send(nas_user_t *user, int msg_type, bool is_standalone, int pti, int ebi,
                         const esm_sap_data_t *data, OctetString *rsp);


/*
 * String representation of ESM-SAP primitives
 */

const char *esm_sap_primitive2str(int esmsapp) {
static const char *_esm_sap_primitive_str[] = {
  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ",
  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF",
  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REJ",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REQ",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_CNF",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REJ",
  "ESM_EPS_BEARER_CONTEXT_MODIFY_REQ",
  "ESM_EPS_BEARER_CONTEXT_MODIFY_CNF",
  "ESM_EPS_BEARER_CONTEXT_MODIFY_REJ",
  "ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ",
  "ESM_EPS_BEARER_CONTEXT_DEACTIVATE_CNF",
  "ESM_PDN_CONNECTIVITY_REQ",
  "ESM_PDN_CONNECTIVITY_REJ",
  "ESM_PDN_DISCONNECT_REQ",
  "ESM_PDN_DISCONNECT_REJ",
  "ESM_BEARER_RESOURCE_ALLOCATE_REQ",
  "ESM_BEARER_RESOURCE_ALLOCATE_REJ",
  "ESM_BEARER_RESOURCE_MODIFY_REQ",
  "ESM_BEARER_RESOURCE_MODIFY_REJ",
  "ESM_UNITDATA_IND",
};
  return _esm_sap_primitive_str[esmsapp];
}

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_sap_initialize()                                      **
 **                                                                        **
 ** Description: Initializes the ESM Service Access Point state machine    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    NONE                                       **
 **                                                                        **
 ***************************************************************************/
void esm_sap_initialize(void)
{
  LOG_FUNC_IN;

  /* Initialize ESM state machine */
  //esm_fsm_initialize();

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_sap_send()                                            **
 **                                                                        **
 ** Description: Processes the ESM Service Access Point primitive          **
 **                                                                        **
 ** Inputs:  msg:       The ESM-SAP primitive to process           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_sap_send(nas_user_t *user, esm_sap_t *msg)
{
  LOG_FUNC_IN;
  esm_data_t *esm_data = user->esm_data;
  msg->send.value = esm_data->send_buffer;

  int rc = RETURNerror;
  int pid;

  /* Check the ESM-SAP primitive */
  esm_primitive_t primitive = msg->primitive;
  assert( (primitive > ESM_START) && (primitive < ESM_END));

  LOG_TRACE(INFO, "ESM-SAP   - Received primitive %s (%d)",
            esm_sap_primitive2str(primitive - ESM_START - 1), primitive);

  switch (primitive) {
  case ESM_PDN_CONNECTIVITY_REQ:
  {
    esm_pdn_connectivity_t *pdn_connect = &msg->data.pdn_connect;

    if ( !msg->is_standalone || !pdn_connect->is_defined ) {
      OctetString apn = {0, NULL};

      if (pdn_connect->apn) {
        apn.length = strlen(pdn_connect->apn);
        apn.value = (uint8_t *)pdn_connect->apn;
      }

      /* Define new PDN context */
      rc = esm_proc_pdn_connectivity(user, pdn_connect->cid, true,
                                     pdn_connect->pdn_type, &apn,
                                     pdn_connect->is_emergency, NULL);

      if ( msg->is_standalone || (rc != RETURNok) ) {
        break;
      }
    }

    if (pdn_connect->is_defined) {
      unsigned int pti;
      /* Assign new procedure transaction identity */
      rc = esm_proc_pdn_connectivity(user, pdn_connect->cid, true,
                                     pdn_connect->pdn_type, NULL,
                                     pdn_connect->is_emergency, &pti);

      if (rc != RETURNerror) {
        /* Send PDN connectivity request */
        rc = _esm_sap_send(user, PDN_CONNECTIVITY_REQUEST,
                           msg->is_standalone,
                           pti, EPS_BEARER_IDENTITY_UNASSIGNED,
                           &msg->data, &msg->send);
      }
    }
  }
  break;

  case ESM_PDN_CONNECTIVITY_REJ:

    {
      esm_pdn_connectivity_t *pdn_connect = &msg->data.pdn_connect;

      if ( msg->is_standalone && pdn_connect->is_defined ) {
        /* Undefine the specified PDN context */
        rc = esm_proc_pdn_connectivity(user, pdn_connect->cid, false,
                                       pdn_connect->pdn_type, NULL,
                                       pdn_connect->is_emergency, NULL);
      } else if (msg->recv != NULL) {
        /* The UE received a PDN connectivity reject message */
        rc = _esm_sap_recv(user, PDN_CONNECTIVITY_REJECT, msg->is_standalone,
                           msg->recv, &msg->send, &msg->err);
      } else {
        /* The PDN connectivity procedure locally failed */
        rc = esm_proc_pdn_connectivity_failure(user, true);
      }
    }
    break;

  case ESM_PDN_DISCONNECT_REQ:
  {
    unsigned int pti, ebi;
    /* Get the procedure transaction identity and the EPS bearer
     * identity of the default bearer assigned to the PDN to
     * disconnect from */
    rc = esm_proc_pdn_disconnect(esm_data, msg->data.pdn_disconnect.cid,
                                 &pti, &ebi);

    if (rc != RETURNerror) {
      /* Send PDN disconnect request */
      rc = _esm_sap_send(user, PDN_DISCONNECT_REQUEST, true, pti, ebi,
                         &msg->data, &msg->send);
    }
  }
  break;

  case ESM_PDN_DISCONNECT_REJ:
    break;

  case ESM_BEARER_RESOURCE_ALLOCATE_REQ:
    break;

  case ESM_BEARER_RESOURCE_ALLOCATE_REJ:
    break;

  case ESM_BEARER_RESOURCE_MODIFY_REQ:
    break;

  case ESM_BEARER_RESOURCE_MODIFY_REJ:
    break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ:
    /* The UE received activate default ESP bearer context request */
    rc = _esm_sap_recv(user, ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST,
                       msg->is_standalone,
                       msg->recv, &msg->send, &msg->err);
    break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF:
    /*
     * The activate default ESP bearer context accept message
     * has been successfully delivered to the other side
     */
    rc = esm_proc_default_eps_bearer_context_complete(user->default_eps_bearer_context_data);

    if (rc != RETURNerror) {
      rc = esm_proc_pdn_connectivity_complete(user);
    }

    break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REJ:
    /*
     * Default ESP bearer context activation procedure locally failed
     */
    rc = esm_proc_default_eps_bearer_context_failure(user);

    if (rc != RETURNerror) {
      rc = esm_proc_pdn_connectivity_failure(user, false);
    }

    break;

  case ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REQ:
    break;

  case ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_CNF:
    break;

  case ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REJ:
    break;

  case ESM_EPS_BEARER_CONTEXT_MODIFY_REQ:
    break;

  case ESM_EPS_BEARER_CONTEXT_MODIFY_CNF:
    break;

  case ESM_EPS_BEARER_CONTEXT_MODIFY_REJ:
    break;

  case ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ: {
    int bid;
    /*
     * Locally deactivate EPS bearer context
     */
    rc = esm_proc_eps_bearer_context_deactivate(user, true,
         msg->data.eps_bearer_context_deactivate.ebi, &pid, &bid);
  }
  break;

  case ESM_EPS_BEARER_CONTEXT_DEACTIVATE_CNF:
    break;

  case ESM_UNITDATA_IND:
    rc = _esm_sap_recv(user, -1, msg->is_standalone, msg->recv,
                       &msg->send, &msg->err);
    break;

  default:
    break;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_sap_recv()                                           **
 **                                                                        **
 ** Description: Processes ESM messages received from the network: Decodes **
 **      the message and checks whether it is of the expected ty-  **
 **      pe, checks the validity of the procedure transaction iden-**
 **      tity, checks the validity of the EPS bearer identity, and **
 **      parses the message content.                               **
 **      If no protocol error is found the ESM response message is **
 **      returned in order to be sent back onto the network upon   **
 **      the relevant ESM procedure completion.                    **
 **      If a protocol error is found the ESM status message is    **
 **      returned including the value of the ESM cause code.       **
 **                                                                        **
 ** Inputs:  msg_type:  Expected type of the received ESM message  **
 **      is_standalone: Indicates whether the ESM message has been **
 **             received standalone or together within EMM **
 **             attach related message                     **
 **      ueid:      UE identifier within the MME               **
 **      req:       The encoded ESM request message to process **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     rsp:       The encoded ESM response message to be re- **
 **             turned upon ESM procedure completion       **
 **      err:       Error code of the ESM procedure            **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int _esm_sap_recv(nas_user_t *user, int msg_type, bool is_standalone,
                         const OctetString *req, OctetString *rsp,
                         esm_sap_error_t *err)
{
  esm_proc_procedure_t esm_procedure = NULL;
  int esm_cause = ESM_CAUSE_SUCCESS;
  int rc = RETURNerror;
  int decoder_rc;

  ESM_msg esm_msg;

  LOG_FUNC_IN;

  memset(&esm_msg, 0 , sizeof(ESM_msg));

  /* Decode the received ESM message */
  decoder_rc = esm_msg_decode(&esm_msg, req->value, req->length);

  /* Process decoding errors */
  if (decoder_rc < 0) {
    /* 3GPP TS 24.301, section 7.2
     * Ignore received message that is too short to contain a complete
     * message type information element */
    if (decoder_rc == TLV_DECODE_BUFFER_TOO_SHORT) {
      LOG_TRACE(WARNING, "ESM-SAP   - Discard message too short to "
                "contain a complete message type IE");
      /* Return indication that received message has been discarded */
      *err = ESM_SAP_DISCARDED;
      LOG_FUNC_RETURN (RETURNok);
    }
    /* 3GPP TS 24.301, section 7.2
     * Unknown or unforeseen message type */
    else if (decoder_rc == TLV_DECODE_WRONG_MESSAGE_TYPE) {
      esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
    }
    /* 3GPP TS 24.301, section 7.7.2
     * Conditional IE errors */
    else if (decoder_rc == TLV_DECODE_UNEXPECTED_IEI) {
      esm_cause = ESM_CAUSE_CONDITIONAL_IE_ERROR;
    } else {
      esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    }
  }
  /* Check the type of the ESM message actually received */
  else if ( (msg_type > 0) && (msg_type != esm_msg.header.message_type) ) {
    if (esm_msg.header.message_type != ESM_STATUS) {
      /* Semantically incorrect ESM message */
      LOG_TRACE(ERROR, "ESM-SAP   - Received ESM message 0x%x is not "
                "of the expected type (0x%x)",
                esm_msg.header.message_type, msg_type);
      esm_cause = ESM_CAUSE_SEMANTICALLY_INCORRECT;
    }
  }

  /* Get the procedure transaction identity */
  unsigned int pti = esm_msg.header.procedure_transaction_identity;
  /* Get the EPS bearer identity */
  unsigned int ebi = esm_msg.header.eps_bearer_identity;
  /* Indicate whether the ESM bearer context related procedure was triggered
   * by the receipt of a transaction related request message from the UE or
   * was triggered network-internally */
  int triggered_by_ue = (pti != PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
  /* Indicate whether the received message shall be ignored */
  bool is_discarded = false;

  if (esm_cause != ESM_CAUSE_SUCCESS) {
    LOG_TRACE(ERROR, "ESM-SAP   - Failed to decode expected ESM message "
              "0x%x", msg_type);
  }
  /* Process the received ESM message */
  else switch (esm_msg.header.message_type) {

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
      /*
       * Process activate default EPS bearer context request message
       * received from the MME
       */
      esm_cause = esm_recv_activate_default_eps_bearer_context_request(
                    user, pti, ebi,
                    &esm_msg.activate_default_eps_bearer_context_request);

      if ( (esm_cause == ESM_CAUSE_SUCCESS) ||
           (esm_cause == ESM_CAUSE_PTI_ALREADY_IN_USE) ) {
        /* Return accept message */
        rc = esm_send_activate_default_eps_bearer_context_accept(ebi,
             &esm_msg.activate_default_eps_bearer_context_accept);
        /* Setup the callback function used to send activate default
         * EPS bearer context accept message onto the network */
        esm_procedure = esm_proc_default_eps_bearer_context_accept;

        if (esm_cause == ESM_CAUSE_PTI_ALREADY_IN_USE) {
          /* 3GPP TS 24.301, section 7.3.1, case g
           * Return accept message, even to network retransmission
           * of already accepted activate default EPS bearer context
           * request message
           */
          is_discarded = true;
        }
      } else {
        /* Return reject message */
        rc = esm_send_activate_default_eps_bearer_context_reject(ebi,
             &esm_msg.activate_default_eps_bearer_context_reject,
             esm_cause);
        /* Setup the callback function used to send activate default
         * EPS bearer context reject message onto the network */
        esm_procedure = esm_proc_default_eps_bearer_context_reject;
      }

      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
      /*
       * Process activate dedicated EPS bearer context request message
       * received from the MME
       */
      esm_cause = esm_recv_activate_dedicated_eps_bearer_context_request(
                    user, pti, ebi,
                    &esm_msg.activate_dedicated_eps_bearer_context_request);

      if ( (esm_cause == ESM_CAUSE_SUCCESS) ||
           (esm_cause == ESM_CAUSE_PTI_ALREADY_IN_USE) ) {
        /* Return accept message */
        rc = esm_send_activate_dedicated_eps_bearer_context_accept(ebi,
             &esm_msg.activate_dedicated_eps_bearer_context_accept);
        /* Setup the callback function used to send activate dedicated
         * EPS bearer context accept message onto the network */
        esm_procedure = esm_proc_dedicated_eps_bearer_context_accept;

        if (esm_cause == ESM_CAUSE_PTI_ALREADY_IN_USE) {
          /* 3GPP TS 24.301, section 7.3.1, case i
           * Return accept message, even to network retransmission
           * of already accepted activate dedicated EPS bearer context
           * request message
           */
          is_discarded = true;
        }
      } else {
        /* Return reject message */
        rc = esm_send_activate_dedicated_eps_bearer_context_reject(ebi,
             &esm_msg.activate_dedicated_eps_bearer_context_reject,
             esm_cause);
        /* Setup the callback function used to send activate dedicated
         * EPS bearer context reject message onto the network */
        esm_procedure = esm_proc_dedicated_eps_bearer_context_reject;
      }

      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
      break;

    case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
      /*
       * Process deactivate EPS bearer context request message
       * received from the MME
       */
      esm_cause = esm_recv_deactivate_eps_bearer_context_request(user, pti, ebi,
                  &esm_msg.deactivate_eps_bearer_context_request);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_PTI_MISMATCH) ) {
        /* 3GPP TS 24.301, section 7.3.1, case m
         * Ignore deactivate EPS bearer context request message
         * received with PTI reserved value, or assigned value
         * that does not match any PTI in use
         */
        is_discarded = true;
      } else if ( (esm_cause == ESM_CAUSE_SUCCESS) ||
                  (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* Return accept message */
        rc = esm_send_deactivate_eps_bearer_context_accept(ebi,
             &esm_msg.deactivate_eps_bearer_context_accept);
        /* Setup the callback function used to send deactivate
         * EPS bearer context accept message onto the network */
        esm_procedure = esm_proc_eps_bearer_context_deactivate_accept;

        if (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) {
          /* 3GPP TS 24.301, section 7.3.2, case j
           * Respond with a deactivate EPS bearer context accept
           * message with the EPS bearer identity set to the received
           * EPS bearer identity
           */
          is_discarded = true;
        }
      }

      break;

    case PDN_CONNECTIVITY_REJECT:
      /*
       * Process PDN connectivity reject message received from the MME
       */
      esm_cause = esm_recv_pdn_connectivity_reject(user, pti, ebi,
                  &esm_msg.pdn_connectivity_reject);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_PTI_MISMATCH) ||
           (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* 3GPP TS 24.301, section 7.3.1, case a
         * Ignore PDN connectivity reject message received with
         * reserved or unassigned PTI value, or assigned value that
         * does not match any PTI in use
         * 3GPP TS 24.301, section 7.3.2, case a
         * Ignore PDN connectivity reject message received with
         * reserved or unassigned EPS bearer identity value
         */
        is_discarded = true;
      }

      break;

    case PDN_DISCONNECT_REJECT:
      /*
       * Process PDN disconnect reject message received from the MME
       */
      esm_cause = esm_recv_pdn_disconnect_reject(user, pti, ebi,
                  &esm_msg.pdn_disconnect_reject);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_PTI_MISMATCH) ||
           (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* 3GPP TS 24.301, section 7.3.1, case b
         * Ignore PDN disconnect reject message received with
         * reserved or unassigned PTI value, or assigned value that
         * does not match any PTI in use
         * 3GPP TS 24.301, section 7.3.2, case b
         * Ignore PDN disconnect reject message received with
         * reserved or unassigned EPS bearer identity value
         */
        is_discarded = true;
      }

      break;

    case BEARER_RESOURCE_ALLOCATION_REJECT:
      break;

    case BEARER_RESOURCE_MODIFICATION_REJECT:
      break;

    case ESM_INFORMATION_REQUEST:
      break;

    case ESM_STATUS:
      /*
       * Process received ESM status message
       */
      esm_cause = esm_recv_status(pti, ebi, &esm_msg.esm_status);
      break;

    default:
      LOG_TRACE(WARNING, "ESM-SAP   - Received unexpected ESM message "
                "0x%x", esm_msg.header.message_type);
      esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
      break;
    }

  if ( (esm_cause != ESM_CAUSE_SUCCESS) && (esm_procedure == NULL) ) {
    /* ESM message processing failed */
    if (!is_discarded) {
      /* 3GPP TS 24.301, section 7.1
       * Handling of unknown, unforeseen, and erroneous protocol data */
      LOG_TRACE(WARNING, "ESM-SAP   - Received ESM message is not valid "
                "(cause=%d)", esm_cause);
      /* Return an ESM status message */
      rc = esm_send_status(pti, ebi, &esm_msg.esm_status, esm_cause);
      /* Setup the callback function used to send ESM status message
       * onto the network */
      esm_procedure = esm_proc_status;
      /* Discard received ESM message */
      is_discarded = true;
    }
  } else {
    /* ESM message processing succeed */
    *err = ESM_SAP_SUCCESS;
    rc = RETURNok;
  }

  if ( (rc != RETURNerror) && (esm_procedure != NULL) ) {
    /* Encode the returned ESM response message */
    int size = esm_msg_encode(&esm_msg, rsp->value,
                              ESM_SAP_BUFFER_SIZE);

    rsp->length = size;

    /* Complete the relevant ESM procedure */
    rc = (*esm_procedure)(user, is_standalone, ebi, rsp, triggered_by_ue);

    if (is_discarded) {
      /* Return indication that received message has been discarded */
      *err = ESM_SAP_DISCARDED;
    } else if (rc != RETURNok) {
      /* Return indication that ESM procedure failed */
      *err = ESM_SAP_FAILED;
    }
  } else if (is_discarded) {
    LOG_TRACE(WARNING, "ESM-SAP   - Silently discard message type 0x%x",
              esm_msg.header.message_type);
    /* Return indication that received message has been discarded */
    *err = ESM_SAP_DISCARDED;
    rc = RETURNok;
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_sap_send()                                           **
 **                                                                        **
 ** Description: Processes ESM messages to send onto the network: Encoded  **
 **      the message and execute the relevant ESM procedure.       **
 **                                                                        **
 ** Inputs:  msg_type:  Type of the ESM message to be sent         **
 **      is_standalone: Indicates whether the ESM message has to   **
 **             be sent standalone or together within EMM  **
 **             attach related message                     **
 **      ueid:      UE identifier within the MME               **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      data:      Data required to build the message         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     rsp:       The encoded ESM response message to be re- **
 **             turned upon ESM procedure completion       **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int _esm_sap_send(nas_user_t *user, int msg_type, bool is_standalone,
                         int pti, int ebi, const esm_sap_data_t *data,
                         OctetString *rsp)
{
  LOG_FUNC_IN;

  esm_proc_procedure_t esm_procedure = NULL;
  int rc = RETURNok;

  /* Indicate whether the message is sent by the UE or the MME */
  bool sent_by_ue = true;

  ESM_msg esm_msg;
  memset(&esm_msg, 0 , sizeof(ESM_msg));

  /* Process the ESM message to send */
  switch (msg_type) {

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
    break;

  case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REJECT:
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
    break;

  case PDN_CONNECTIVITY_REQUEST: {
    /*
     * Process PDN connectivity request message to send to the MME
     */
    const esm_pdn_connectivity_t *msg = &data->pdn_connect;
    rc = esm_send_pdn_connectivity_request(pti, msg->is_emergency,
                                           msg->pdn_type, msg->apn,
                                           &esm_msg.pdn_connectivity_request);
    /* Setup callback function used to send PDN connectivity request
     * message onto the network */
    esm_procedure = esm_proc_pdn_connectivity_request;
    break;
  }

  case PDN_DISCONNECT_REQUEST:
    /*
     * Process PDN disconnect request message to send to the MME
     */
    rc = esm_send_pdn_disconnect_request(pti, ebi,
                                         &esm_msg.pdn_disconnect_request);
    /* Setup callback function used to send PDN disconnect request
     * message onto the network */
    esm_procedure = esm_proc_pdn_disconnect_request;
    break;

  case BEARER_RESOURCE_ALLOCATION_REQUEST:
    break;

  case BEARER_RESOURCE_MODIFICATION_REQUEST:
    break;

  default:
    LOG_TRACE(WARNING, "ESM-SAP   - Send unexpected ESM message 0x%x",
              msg_type);
    break;
  }

  if (rc != RETURNerror) {
    /* Encode the returned ESM response message */
    int size = esm_msg_encode(&esm_msg, rsp->value,
                              ESM_SAP_BUFFER_SIZE);

    rsp->length = size;
    /* Execute the relevant ESM procedure */
    if (esm_procedure) {
      rc = (*esm_procedure)(user, is_standalone, pti, rsp, sent_by_ue);
    }

  }

  LOG_FUNC_RETURN(rc);
}

