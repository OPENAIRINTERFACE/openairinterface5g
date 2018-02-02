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
Source      esm_send.c

Version     0.1

Date        2013/02/11

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions executed at the ESM Service Access
        Point to send EPS Session Management messages to the
        EPS Mobility Management sublayer.

*****************************************************************************/

#include "esm_send.h"
#include "commonDef.h"
#include "nas_log.h"

#include "networkDef.h"

#include "esm_msgDef.h"
#include "esm_cause.h"

#include <string.h> // strlen

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * Functions executed by both the UE and the MME to send ESM messages
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_status()                                         **
 **                                                                        **
 ** Description: Builds ESM status message                                 **
 **                                                                        **
 **      The ESM status message is sent by the network or the UE   **
 **      to pass information on the status of the indicated EPS    **
 **      bearer context and report certain error conditions.       **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_status(int pti, int ebi, esm_status_msg *msg, int esm_cause)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = ESM_STATUS;
  msg->proceduretransactionidentity = pti;

  /* Mandatory - ESM cause code */
  msg->esmcause = esm_cause;

  LOG_TRACE(WARNING, "ESM-SAP   - Send ESM Status message (pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}

/*
 * --------------------------------------------------------------------------
 * Functions executed by the UE to send ESM message to the network
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_pdn_connectivity_request()                       **
 **                                                                        **
 ** Description: Builds PDN Connectivity Request message                   **
 **                                                                        **
 **      The PDN connectivity request message is sent by the UE to **
 **      the network to initiate establishment of a PDN connection.**
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity assigned to **
 **             the PDN connection                         **
 **      is_emergency:  Indicates whether the PDN connectivity is  **
 **             requested for emergency bearer services    **
 **      pdn_type:  PDN address type                           **
 **      apn:       Access Point Name of the PDN to connect to **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_pdn_connectivity_request(int pti, int is_emergency, int pdn_type,
                                      const char *apn,
                                      pdn_connectivity_request_msg *msg)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = EPS_BEARER_IDENTITY_UNASSIGNED;
  msg->messagetype = PDN_CONNECTIVITY_REQUEST;
  msg->proceduretransactionidentity = pti;

  /* Mandatory - PDN connectivity request type */
  if (is_emergency) {
    msg->requesttype = REQUEST_TYPE_EMERGENCY;
  } else {
    msg->requesttype = REQUEST_TYPE_INITIAL_REQUEST;
  }

  /* Mandatory - PDN type */
  if (pdn_type == NET_PDN_TYPE_IPV4) {
    msg->pdntype = PDN_TYPE_IPV4;
  } else if (pdn_type == NET_PDN_TYPE_IPV6) {
    msg->pdntype = PDN_TYPE_IPV6;
  } else if (pdn_type == NET_PDN_TYPE_IPV4V6) {
    msg->pdntype = PDN_TYPE_IPV4V6;
  }

  /* Optional - Access Point Name */
  msg->presencemask = 0;

  if (apn) {
    size_t len = strlen(apn);

    if (len > 0) {
      msg->presencemask |=
        PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT;
      msg->accesspointname.accesspointnamevalue.length = len;
      msg->accesspointname.accesspointnamevalue.value = (uint8_t *)apn;
    }
  }

  /* Add PCO */
# define CONFIGURATION_PROTOCOL_PPP          0
# define PROTOCOL_ID_IPCP                    0x8021
# define PROTOCOL_ID_DNS_SERVER_IPV4_ADDRESS 0x000D
# define PROTOCOL_ID_IP_ADDR_NAS_SIGNALING   0x000A

  msg->presencemask |= PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  msg->protocolconfigurationoptions.configurationprotol = CONFIGURATION_PROTOCOL_PPP;
  msg->protocolconfigurationoptions.num_protocol_id_or_container_id = 3;
  msg->protocolconfigurationoptions.protocolid[0] = PROTOCOL_ID_IPCP;
  msg->protocolconfigurationoptions.lengthofprotocolid[0] = 16; /* Size of PROTOCOL_ID_IPCP */
  msg->protocolconfigurationoptions.protocolidcontents[0].value = (uint8_t *)
  /* PROTOCOL_ID_IPCP data */
  /* the format is as of RFC 1331 (see page 27) */
  "\x01"                  /* Configure-Request                    */
  "\x00"                  /* Indentifier (set to 0)               */
  "\x00\x10"              /* Length (16 bytes)                    */
    "\x81"                /* Type = Primary DNS - see RFC 1877    */
    "\x06"                /* length                               */
    "\x00\x00\x00\x00"    /* IP address set to 0                  */
    "\x83"                /* Type = Secondary DNS - see RFC 1877  */
    "\x06"                /* length                               */
    "\x00\x00\x00\x00";   /* IP address set to 0                  */
  msg->protocolconfigurationoptions.protocolidcontents[0].length = 16;
  msg->protocolconfigurationoptions.protocolid[1] = PROTOCOL_ID_DNS_SERVER_IPV4_ADDRESS;
  msg->protocolconfigurationoptions.lengthofprotocolid[1] = 0x00;
  msg->protocolconfigurationoptions.protocolid[2] = PROTOCOL_ID_IP_ADDR_NAS_SIGNALING;
  msg->protocolconfigurationoptions.lengthofprotocolid[2] = 0x00;

  LOG_TRACE(INFO, "ESM-SAP   - Send PDN Connectivity Request message "
            "(pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_pdn_disconnect_request()                         **
 **                                                                        **
 ** Description: Builds PDN Disconnect Request message                     **
 **                                                                        **
 **      The PDN disconnect request message is sent by the UE to   **
 **      the network to initiate release of a PDN connection.      **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity assigned to **
 **             the PDN connection                         **
 **      ebi:       EPS bearer identity of the default bearer  **
 **             associated with the PDN to disconnect from **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_pdn_disconnect_request(int pti, int ebi,
                                    pdn_disconnect_request_msg *msg)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = EPS_BEARER_IDENTITY_UNASSIGNED;
  msg->messagetype = PDN_DISCONNECT_REQUEST;
  msg->proceduretransactionidentity = pti;

  /* Mandatory - Linked EPS bearer identity */
  msg->linkedepsbeareridentity = ebi;
  /* Optional IEs  */
  msg->presencemask = 0;

  LOG_TRACE(INFO, "ESM-SAP   - Send PDN Disconnect Request message "
            "(pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_activate_default_eps_bearer_context_accept()     **
 **                                                                        **
 ** Description: Builds Activate Default EPS Bearer Context Accept message **
 **                                                                        **
 **      The activate default EPS bearer context accept message is **
 **      sent by the UE to the network to acknowledge activation   **
 **      of a default EPS bearer context.                          **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_activate_default_eps_bearer_context_accept(int ebi,
    activate_default_eps_bearer_context_accept_msg *msg)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT;
  msg->proceduretransactionidentity = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
  /* Mandatory IEs */
  /* Optional IEs  */
  msg->presencemask = 0;

  LOG_TRACE(INFO, "ESM-SAP   - Send Activate Default EPS Bearer Context "
            "Accept message (pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_activate_default_eps_bearer_context_reject()     **
 **                                                                        **
 ** Description: Builds Activate Default EPS Bearer Context Reject message **
 **                                                                        **
 **      The activate default EPS bearer context reject message is **
 **      sent by UE to the network to reject activation of a de-   **
 **      fault EPS bearer context.                                 **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_activate_default_eps_bearer_context_reject(int ebi,
    activate_default_eps_bearer_context_reject_msg *msg, int esm_cause)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT;
  msg->proceduretransactionidentity = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
  /* Mandatory - ESM cause code */
  msg->esmcause = esm_cause;
  /* Optional IEs  */
  msg->presencemask = 0;

  LOG_TRACE(INFO, "ESM-SAP   - Send Activate Default EPS Bearer Context "
            "Reject message (pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_activate_dedicated_eps_bearer_context_accept()   **
 **                                                                        **
 ** Description: Builds Activate Dedicated EPS Bearer Context Accept       **
 **      message                                                   **
 **                                                                        **
 **      The activate dedicated EPS bearer context accept message  **
 **      is sent by the UE to the network to acknowledge activa-   **
 **      tion of a dedicated EPS bearer context associated with    **
 **      the same PDN address(es) and APN as an already active EPS **
 **      bearer context.                                           **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_activate_dedicated_eps_bearer_context_accept(int ebi,
    activate_dedicated_eps_bearer_context_accept_msg *msg)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT;
  msg->proceduretransactionidentity = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
  /* Mandatory IEs */
  /* Optional IEs  */
  msg->presencemask = 0;

  LOG_TRACE(INFO, "ESM-SAP   - Send Activate Dedicated EPS Bearer Context "
            "Accept message (pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_activate_dedicated_eps_bearer_context_reject()   **
 **                                                                        **
 ** Description: Builds Activate Dedicated EPS Bearer Context Reject       **
 **       message                                                  **
 **                                                                        **
 **      The activate dedicated EPS bearer context reject message  **
 **      is sent by UE to the network to reject activation of a    **
 **      dedicated EPS bearer context.                             **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_activate_dedicated_eps_bearer_context_reject(int ebi,
    activate_dedicated_eps_bearer_context_reject_msg *msg, int esm_cause)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT;
  msg->proceduretransactionidentity = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
  /* Mandatory - ESM cause code */
  msg->esmcause = esm_cause;
  /* Optional IEs  */
  msg->presencemask = 0;

  LOG_TRACE(INFO, "ESM-SAP   - Send Activate Dedicated EPS Bearer Context "
            "Reject message (pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_deactivate_eps_bearer_context_accept()           **
 **                                                                        **
 ** Description: Builds Deactivate EPS Bearer Context Accept message       **
 **                                                                        **
 **      The deactivate EPS bearer context accept message is sent  **
 **      by the UE to acknowledge deactivation of an EPS bearer    **
 **      context.                                                  **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_send_deactivate_eps_bearer_context_accept(int ebi,
    deactivate_eps_bearer_context_accept_msg *msg)
{
  LOG_FUNC_IN;

  /* Mandatory - ESM message header */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT;
  msg->proceduretransactionidentity = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
  /* Mandatory IEs */
  /* Optional IEs  */
  msg->presencemask = 0;

  LOG_TRACE(INFO, "ESM-SAP   - Send Deactivate EPS Bearer Context Accept"
            " message (pti=%d, ebi=%d)",
            msg->proceduretransactionidentity, msg->epsbeareridentity);

  LOG_FUNC_RETURN(RETURNok);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

