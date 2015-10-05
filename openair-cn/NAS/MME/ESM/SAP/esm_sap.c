/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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


static int _esm_sap_recv(int msg_type, int is_standalone, emm_data_context_t *ctx,
                         const OctetString *req, OctetString *rsp, esm_sap_error_t *err);
static int _esm_sap_send(int msg_type, int is_standalone, emm_data_context_t *ctx,
                         int pti, int ebi, const esm_sap_data_t *data, OctetString *rsp);

/*
 * String representation of ESM-SAP primitives
 */
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

/*
 * Buffer used to encode ESM messages before being returned to the EPS
 * Mobility Management sublayer in order to be sent onto the network
 */
#define ESM_SAP_BUFFER_SIZE 4096
static char _esm_sap_buffer[ESM_SAP_BUFFER_SIZE];

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
int esm_sap_send(esm_sap_t *msg)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;
  int pid;

  /* Check the ESM-SAP primitive */
  esm_primitive_t primitive = msg->primitive;
  assert( (primitive > ESM_START) && (primitive < ESM_END));

  LOG_TRACE(INFO, "ESM-SAP   - Received primitive %s (%d)",
            _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);

  switch (primitive) {
  case ESM_PDN_CONNECTIVITY_REQ:

  /* The MME received a PDN connectivity request message */
  rc = _esm_sap_recv(PDN_CONNECTIVITY_REQUEST, msg->is_standalone,
                     msg->ctx, msg->recv, &msg->send, &msg->err);
  break;

  case ESM_PDN_CONNECTIVITY_REJ:
    /* PDN connectivity locally failed */
    pid = esm_proc_default_eps_bearer_context_failure(msg->ctx);

    if (pid != RETURNerror) {
      rc = esm_proc_pdn_connectivity_failure(msg->ctx, pid);
    }
    break;

  case ESM_PDN_DISCONNECT_REQ:
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
    break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF:
    /* The MME received activate default ESP bearer context accept */
    rc = _esm_sap_recv(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT,
                       msg->is_standalone, msg->ctx,
                       msg->recv, &msg->send, &msg->err);
    break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REJ:
    /* The MME received activate default ESP bearer context reject */
    rc = _esm_sap_recv(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT,
                       msg->is_standalone, msg->ctx,
                       msg->recv, &msg->send, &msg->err);
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
    rc = esm_proc_eps_bearer_context_deactivate(msg->ctx, TRUE,
         msg->data.eps_bearer_context_deactivate.ebi,
         &pid, &bid, NULL);
  }
  break;

  case ESM_EPS_BEARER_CONTEXT_DEACTIVATE_CNF:
    break;

  case ESM_UNITDATA_IND:
    rc = _esm_sap_recv(-1, msg->is_standalone, msg->ctx,
                       msg->recv, &msg->send, &msg->err);
    break;

  default:
    break;
  }


  if (rc != RETURNok) {
    LOG_TRACE(ERROR, "ESM-SAP   - Failed to process primitive %s (%d)",
              _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);
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
 **      Others:    _esm_sap_buffer                            **
 **                                                                        **
 ***************************************************************************/
static int _esm_sap_recv(int msg_type, int is_standalone,
                         emm_data_context_t *ctx,
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
  int is_discarded = FALSE;

  if (esm_cause != ESM_CAUSE_SUCCESS) {
    LOG_TRACE(ERROR, "ESM-SAP   - Failed to decode expected ESM message "
              "0x%x", msg_type);
  }
  /* Process the received ESM message */
  else switch (esm_msg.header.message_type) {


    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate default EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_activate_default_eps_bearer_context_accept(
                    ctx, pti, ebi,
                    &esm_msg.activate_default_eps_bearer_context_accept);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* 3GPP TS 24.301, section 7.3.1, case f
         * Ignore ESM message received with reserved PTI value
         * 3GPP TS 24.301, section 7.3.2, case f
         * Ignore ESM message received with reserved or assigned
         * value that does not match an existing EPS bearer context
         */
        is_discarded = TRUE;
      }

      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process activate default EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_activate_default_eps_bearer_context_reject(
                    ctx, pti, ebi,
                    &esm_msg.activate_default_eps_bearer_context_reject);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* 3GPP TS 24.301, section 7.3.1, case f
         * Ignore ESM message received with reserved PTI value
         * 3GPP TS 24.301, section 7.3.2, case f
         * Ignore ESM message received with reserved or assigned
         * value that does not match an existing EPS bearer context
         */
        is_discarded = TRUE;
      }

      break;

    case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process deactivate EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_deactivate_eps_bearer_context_accept(
                    ctx, pti, ebi,
                    &esm_msg.deactivate_eps_bearer_context_accept);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* 3GPP TS 24.301, section 7.3.1, case f
         * Ignore ESM message received with reserved PTI value
         * 3GPP TS 24.301, section 7.3.2, case f
         * Ignore ESM message received with reserved or assigned
         * value that does not match an existing EPS bearer context
         */
        is_discarded = TRUE;
      }

      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate dedicated EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_activate_dedicated_eps_bearer_context_accept(
                    ctx, pti, ebi,
                    &esm_msg.activate_dedicated_eps_bearer_context_accept);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* 3GPP TS 24.301, section 7.3.1, case f
         * Ignore ESM message received with reserved PTI value
         * 3GPP TS 24.301, section 7.3.2, case f
         * Ignore ESM message received with reserved or assigned
         * value that does not match an existing EPS bearer context
         */
        is_discarded = TRUE;
      }

      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process activate dedicated EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_activate_dedicated_eps_bearer_context_reject(
                    ctx, pti, ebi,
                    &esm_msg.activate_dedicated_eps_bearer_context_reject);

      if ( (esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) ||
           (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) ) {
        /* 3GPP TS 24.301, section 7.3.1, case f
         * Ignore ESM message received with reserved PTI value
         * 3GPP TS 24.301, section 7.3.2, case f
         * Ignore ESM message received with reserved or assigned
         * value that does not match an existing EPS bearer context
         */
        is_discarded = TRUE;
      }

      break;

    case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REJECT:
      break;

    case PDN_CONNECTIVITY_REQUEST: {
      esm_proc_data_t data;
      memset(&data, 0, sizeof(esm_proc_data_t));

      LOG_TRACE(DEBUG, "ESM-SAP   - PDN_CONNECTIVITY_REQUEST pti %u ebi %u", pti, ebi);

      /*
       * Process PDN connectivity request message received from the UE
       */
      esm_cause = esm_recv_pdn_connectivity_request(ctx, pti, ebi,
                  &esm_msg.pdn_connectivity_request,
                  &ebi, &data);

      if (esm_cause != ESM_CAUSE_SUCCESS) {
        /* Return reject message */
        rc = esm_send_pdn_connectivity_reject(pti,
                                              &esm_msg.pdn_connectivity_reject, esm_cause);
        /* Setup the callback function used to send PDN connectivity
         * reject message onto the network */
        esm_procedure = esm_proc_pdn_connectivity_reject;
        /* No ESM status message should be returned */
        esm_cause = ESM_CAUSE_SUCCESS;
      } else {
#if defined(ORIGINAL_CODE)
        /* Setup PDN type */
        int pdn_type = -1;

        if (data.pdn_type == ESM_PDN_TYPE_IPV4) {
          pdn_type = PDN_VALUE_TYPE_IPV4;
        } else if (data.pdn_type == ESM_PDN_TYPE_IPV6) {
          pdn_type = PDN_VALUE_TYPE_IPV6;
        } else if (data.pdn_type == ESM_PDN_TYPE_IPV4V6) {
          pdn_type = PDN_VALUE_TYPE_IPV4V6;
        }

        /* Setup EPS bearer level Quality of Service */
        EpsQualityOfService qos;
        qos.bitRatesPresent = 1;
        qos.bitRatesExtPresent = 0;
        qos.qci = data.qos.qci;
        qos.bitRates.maxBitRateForUL = data.qos.mbrUL;
        qos.bitRates.maxBitRateForDL = data.qos.mbrDL;
        qos.bitRates.guarBitRateForUL = data.qos.gbrUL;
        qos.bitRates.guarBitRateForDL = data.qos.gbrDL;

        /* Return default EPS bearer context request message */
        rc = esm_send_activate_default_eps_bearer_context_request(
               pti, ebi,
               &esm_msg.activate_default_eps_bearer_context_request,
               &data.apn, pdn_type, &data.pdn_addr, &qos, esm_cause);
#if 0
        PacketFilters pkfs;
        pkfs[0].identifier = 1;
        pkfs[0].direction = TRAFFIC_FLOW_TEMPLATE_DOWNLINK_ONLY;
        pkfs[0].eval_precedence = 2;
        pkfs[0].packetfilter.flags =
          (TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG |
           TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG |
           TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG |
           TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG);
        pkfs[0].packetfilter.ipv4remoteaddr[0].addr = 192;
        pkfs[0].packetfilter.ipv4remoteaddr[1].addr = 168;
        pkfs[0].packetfilter.ipv4remoteaddr[2].addr = 12;
        pkfs[0].packetfilter.ipv4remoteaddr[3].addr = 1;
        pkfs[0].packetfilter.ipv4remoteaddr[0].mask = 255;
        pkfs[0].packetfilter.ipv4remoteaddr[1].mask = 255;
        pkfs[0].packetfilter.ipv4remoteaddr[2].mask = 255;
        pkfs[0].packetfilter.ipv4remoteaddr[3].mask = 0;
        pkfs[0].packetfilter.protocolidentifier_nextheader = 17;
        pkfs[0].packetfilter.singlelocalport = 10001;
        pkfs[0].packetfilter.singleremoteport = 12001;
        pkfs[1].identifier = 2;
        pkfs[1].direction = TRAFFIC_FLOW_TEMPLATE_UPLINK_ONLY;
        pkfs[1].eval_precedence = 3;
        pkfs[1].packetfilter.flags =
          (TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG |
           TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG |
           TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG |
           TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG);
        pkfs[1].packetfilter.ipv4remoteaddr[0].addr = 192;
        pkfs[1].packetfilter.ipv4remoteaddr[1].addr = 168;
        pkfs[1].packetfilter.ipv4remoteaddr[2].addr = 12;
        pkfs[1].packetfilter.ipv4remoteaddr[3].addr = 1;
        pkfs[1].packetfilter.ipv4remoteaddr[0].mask = 255;
        pkfs[1].packetfilter.ipv4remoteaddr[1].mask = 255;
        pkfs[1].packetfilter.ipv4remoteaddr[2].mask = 255;
        pkfs[1].packetfilter.ipv4remoteaddr[3].mask = 0;
        pkfs[1].packetfilter.protocolidentifier_nextheader = 17;
        pkfs[1].packetfilter.singlelocalport = 10002;
        pkfs[1].packetfilter.singleremoteport = 12002;
        /* Return dedicated EPS bearer context request message */
        rc = esm_send_activate_dedicated_eps_bearer_context_request(
               pti, ebi,
               &esm_msg.activate_dedicated_eps_bearer_context_request,
               ebi, &qos, &pkfs, 2);
#endif
        esm_procedure = esm_proc_default_eps_bearer_context_request;
#else
        esm_cause = ESM_CAUSE_SUCCESS;
#endif
        /* Setup the callback function used to send default EPS bearer
         * context request message onto the network */
        //esm_procedure = esm_proc_default_eps_bearer_context_request;
      }

      break;
    }


    case PDN_DISCONNECT_REQUEST:
      /*
       * Process PDN disconnect request message received from the UE
       */
      esm_cause = esm_recv_pdn_disconnect_request(ctx, pti, ebi,
                  &esm_msg.pdn_disconnect_request, &ebi);

      if (esm_cause != ESM_CAUSE_SUCCESS) {
        /* Return reject message */
        rc = esm_send_pdn_disconnect_reject(pti,
                                            &esm_msg.pdn_disconnect_reject, esm_cause);
        /* Setup the callback function used to send PDN connectivity
         * reject message onto the network */
        esm_procedure = esm_proc_pdn_disconnect_reject;
        /* No ESM status message should be returned */
        esm_cause = ESM_CAUSE_SUCCESS;
      } else {
        /* Return deactivate EPS bearer context request message */
        rc = esm_send_deactivate_eps_bearer_context_request(pti, ebi,
             &esm_msg.deactivate_eps_bearer_context_request,
             ESM_CAUSE_REGULAR_DEACTIVATION);
        /* Setup the callback function used to send deactivate EPS
         * bearer context request message onto the network */
        esm_procedure = esm_proc_eps_bearer_context_deactivate_request;
      }

      break;

    case BEARER_RESOURCE_ALLOCATION_REQUEST:
      break;

    case BEARER_RESOURCE_MODIFICATION_REQUEST:
      break;

    case ESM_INFORMATION_RESPONSE:
      break;

    case ESM_STATUS:
      /*
       * Process received ESM status message
       */
      esm_cause = esm_recv_status(ctx, pti, ebi, &esm_msg.esm_status);
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
      is_discarded = TRUE;
    }
  } else {
    /* ESM message processing succeed */
    *err = ESM_SAP_SUCCESS;
    rc = RETURNok;
  }

  if ( (rc != RETURNerror) && (esm_procedure != NULL) ) {
    /* Encode the returned ESM response message */
    int size = esm_msg_encode(&esm_msg, (uint8_t *)_esm_sap_buffer,
                              ESM_SAP_BUFFER_SIZE);

    if (size > 0) {
      rsp->length = size;
      rsp->value = (uint8_t *)(_esm_sap_buffer);
    }

    /* Complete the relevant ESM procedure */
    rc = (*esm_procedure)(is_standalone, ctx, ebi, rsp, triggered_by_ue);

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
 **      Others:    _esm_sap_buffer                            **
 **                                                                        **
 ***************************************************************************/
static int _esm_sap_send(int msg_type, int is_standalone,
                         emm_data_context_t *ctx,
                         int pti, int ebi, const esm_sap_data_t *data,
                         OctetString *rsp)
{
  LOG_FUNC_IN;

  esm_proc_procedure_t esm_procedure = NULL;
  int rc = RETURNok;

  /* Indicate whether the message is sent by the UE or the MME */
  int sent_by_ue = FALSE;

  ESM_msg esm_msg;
  memset(&esm_msg, 0 , sizeof(ESM_msg));

  /* Process the ESM message to send */
  switch (msg_type) {



  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    break;

  case PDN_CONNECTIVITY_REJECT:
    break;

  case PDN_DISCONNECT_REJECT:
    break;

  case BEARER_RESOURCE_ALLOCATION_REJECT:
    break;

  case BEARER_RESOURCE_MODIFICATION_REJECT:
    break;

  default:
    LOG_TRACE(WARNING, "ESM-SAP   - Send unexpected ESM message 0x%x",
              msg_type);
    break;
  }

  if (rc != RETURNerror) {
    /* Encode the returned ESM response message */
    int size = esm_msg_encode(&esm_msg, (uint8_t *)_esm_sap_buffer,
                              ESM_SAP_BUFFER_SIZE);

    if (size > 0) {
      rsp->length = size;
      rsp->value = (uint8_t *)(_esm_sap_buffer);
    }

    /* Execute the relevant ESM procedure */
    if (esm_procedure) {
      rc = (*esm_procedure)(is_standalone, ctx, pti, rsp, sent_by_ue);
    }
  }

  LOG_FUNC_RETURN(rc);
}

