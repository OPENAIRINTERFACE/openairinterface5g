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
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    nas_process.c

Version   0.1

Date    2013/04/16

Product   Access-Stratum sublayer simulator

Subsystem NAS message processing

Author    Frederic Maurel

Description Defines functions executed by the Access-Stratum sublayer
    upon receiving NAS messages.

*****************************************************************************/

#include "nas_process.h"

#include "nas_data.h"
#include "nas_message.h"

#include <stdio.h>  // snprintf
#include <string.h> // memset

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 *-----------------------------------------------------------------------------
 *     Functions used to process EPS Mobility Management messages
 *-----------------------------------------------------------------------------
 */
static int _nas_process_emm(char* buffer, int length, const EMM_msg* msg);

static int _attach_request(char* buffer, int length, const attach_request_msg* msg);
static int _attach_accept(char* buffer, int length, const attach_accept_msg* msg);
static int _attach_reject(char* buffer, int length, const attach_reject_msg* msg);
static int _attach_complete(char* buffer, int length, const attach_complete_msg* msg);

static int _detach_request(char* buffer, int length, const detach_request_msg* msg);
static int _detach_accept(char* buffer, int length, const detach_accept_msg* msg);

static int _identity_request(char* buffer, int length, const identity_request_msg* msg);
static int _identity_response(char* buffer, int length, const identity_response_msg* msg);

static int _authentication_request(char* buffer, int length, const authentication_request_msg* msg);
static int _authentication_response(char* buffer, int length, const authentication_response_msg* msg);
static int _authentication_failure(char* buffer, int length, const authentication_failure_msg* msg);
static int _authentication_reject(char* buffer, int length, const authentication_reject_msg* msg);

static int _security_mode_command(char* buffer, int length, const security_mode_command_msg* msg);
static int _security_mode_complete(char* buffer, int length, const security_mode_complete_msg* msg);
static int _security_mode_reject(char* buffer, int length, const security_mode_reject_msg* msg);

static int _esm_message_container(char* buffer, int length, const EsmMessageContainer* msg);

static int _emm_status(char* buffer, int length, const emm_status_msg* msg);

/*
 *-----------------------------------------------------------------------------
 *     Functions used to process EPS Session Management messages
 *-----------------------------------------------------------------------------
 */
static int _nas_process_esm(char* buffer, int length, const ESM_msg* msg);

static int _pdn_connectivity_request(char* buffer, int length, const pdn_connectivity_request_msg* msg);
static int _pdn_connectivity_reject(char* buffer, int length, const pdn_connectivity_reject_msg* msg);

static int _pdn_disconnect_request(char* buffer, int length, const pdn_disconnect_request_msg* msg);
static int _pdn_disconnect_reject(char* buffer, int length, const pdn_disconnect_reject_msg* msg);

static int _activate_default_eps_bearer_context_request(char* buffer, int length, const activate_default_eps_bearer_context_request_msg* msg);
static int _activate_default_eps_bearer_context_accept(char* buffer, int length, const activate_default_eps_bearer_context_accept_msg* msg);
static int _activate_default_eps_bearer_context_reject(char* buffer, int length, const activate_default_eps_bearer_context_reject_msg* msg);

static int _activate_dedicated_eps_bearer_context_request(char* buffer, int length, const activate_dedicated_eps_bearer_context_request_msg* msg);
static int _activate_dedicated_eps_bearer_context_accept(char* buffer, int length, const activate_dedicated_eps_bearer_context_accept_msg* msg);
static int _activate_dedicated_eps_bearer_context_reject(char* buffer, int length, const activate_dedicated_eps_bearer_context_reject_msg* msg);

static int _deactivate_eps_bearer_context_request(char* buffer, int length, const deactivate_eps_bearer_context_request_msg* msg);
static int _deactivate_eps_bearer_context_accept(char* buffer, int length, const deactivate_eps_bearer_context_accept_msg* msg);

static int _esm_status(char* buffer, int length, const esm_status_msg* msg);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 *-----------------------------------------------------------------------------
 *                              Process NAS message
 *-----------------------------------------------------------------------------
 */
int
nas_process(
  char* buffer,
  int length,
  const char* msg,
  int size)
{
  int index = 0;
  int bytes;
  nas_message_t nas_msg;

  /* Decode NAS message */
  memset(&nas_msg, 0, sizeof(nas_message_t));
  bytes = nas_message_decode(msg, &nas_msg, size);

  if (bytes < 0) {
    printf("ERROR\t: %s - Failed to decode NAS message (err=%d)\n",
           __FUNCTION__, bytes);
    return (RETURNerror);
  }

  int protocol_discriminator = nas_msg.header.protocol_discriminator;

  if (protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    /* Process EPS Mobility Management NAS message */
    index += _nas_process_emm(buffer + index,
                              length - index,
                              &nas_msg.plain.emm);
  } else if (protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
    /* Process EPS Session Management NAS message */
    index += _nas_process_esm(buffer + index,
                              length - index,
                              &nas_msg.plain.esm);
  } else {
    printf("ERROR\t: %s - Protocol discriminator is not valid (%d)\n",
           __FUNCTION__, protocol_discriminator);
    return (RETURNerror);
  }

  buffer[index] = '\0';

  return (bytes);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * =============================================================================
 *      Functions used to process EPS Mobility Management messages
 * =============================================================================
 */

/*
 *-----------------------------------------------------------------------------
 *                  Process EPS Mobility Management NAS message
 *-----------------------------------------------------------------------------
 */
static int
_nas_process_emm(
  char* buffer,
  int length,
  const EMM_msg* msg)
{
  int index = 0;
  const EsmMessageContainer* esm_container = NULL;

  printf("INFO\t: Process %s\n", emmMsgType(msg->header.message_type));
  index += snprintf(buffer + index, length - index, "%s (",
                    emmMsgType(msg->header.message_type));

  switch (msg->header.message_type) {
  case ATTACH_REQUEST:
    esm_container = &msg->attach_request.esmmessagecontainer;
    index += _attach_request(buffer + index, length - index,
                             &msg->attach_request);
    break;

  case ATTACH_ACCEPT:
    esm_container = &msg->attach_accept.esmmessagecontainer;
    index += _attach_accept(buffer + index, length - index,
                            &msg->attach_accept);
    break;

  case ATTACH_REJECT:
    esm_container = &msg->attach_reject.esmmessagecontainer;
    index += _attach_reject(buffer + index, length - index,
                            &msg->attach_reject);
    break;

  case ATTACH_COMPLETE:
    esm_container = &msg->attach_complete.esmmessagecontainer;
    index += _attach_complete(buffer + index, length - index,
                              &msg->attach_complete);
    break;

  case DETACH_REQUEST:
    index += _detach_request(buffer + index, length - index,
                             &msg->detach_request);
    break;

  case DETACH_ACCEPT:
    index += _detach_accept(buffer + index, length - index,
                            &msg->detach_accept);
    break;

  case IDENTITY_REQUEST:
    index += _identity_request(buffer + index, length - index,
                               &msg->identity_request);
    break;

  case IDENTITY_RESPONSE:
    index += _identity_response(buffer + index, length - index,
                                &msg->identity_response);
    break;

  case AUTHENTICATION_REQUEST:
    index += _authentication_request(buffer + index, length - index,
                                     &msg->authentication_request);
    break;

  case AUTHENTICATION_RESPONSE:
    index += _authentication_response(buffer + index, length - index,
                                      &msg->authentication_response);
    break;

  case AUTHENTICATION_FAILURE:
    index += _authentication_failure(buffer + index, length - index,
                                     &msg->authentication_failure);
    break;

  case AUTHENTICATION_REJECT:
    index += _authentication_reject(buffer + index, length - index,
                                    &msg->authentication_reject);
    break;

  case SECURITY_MODE_COMMAND:
    index += _security_mode_command(buffer + index, length - index,
                                    &msg->security_mode_command);
    break;

  case SECURITY_MODE_COMPLETE:
    index += _security_mode_complete(buffer + index, length - index,
                                     &msg->security_mode_complete);
    break;

  case SECURITY_MODE_REJECT:
    index += _security_mode_reject(buffer + index, length - index,
                                   &msg->security_mode_reject);
    break;

  case EMM_STATUS:
    index += _emm_status(buffer + index, length - index,
                         &msg->emm_status);
    break;

  default:
    printf("WARNING\t: %s - EMM NAS message is not valid (0x%x)\n",
           __FUNCTION__, msg->header.message_type);
    break;
  }

  index += snprintf(buffer + index, length - index, ")");

  /* Process ESM message container */
  if (esm_container) {
    index += _esm_message_container(buffer + index,
                                    length - index,
                                    esm_container);
  }

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *                              Process Attach Request
 *-----------------------------------------------------------------------------
 */
static int
_attach_request(
  char* buffer,
  int length,
  const attach_request_msg* msg)
{
  int index = 0;

  /* EPS attach type */
  index += snprintf(buffer + index, length - index, "Type = %s",
                    attachType(&msg->epsattachtype));
  /* NAS key set identifier */
  index += snprintf(buffer + index, length - index, ", KSI = ");
  index += nasKeySetIdentifier(buffer + index, length - index,
                               &msg->naskeysetidentifier);
  /* EPS mobile identity */
  index += snprintf(buffer + index, length - index, ", ");
  index += epsIdentity(buffer + index, length - index,
                       &msg->oldgutiorimsi);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *                              Process Attach Accept
 *-----------------------------------------------------------------------------
 */
static int
_attach_accept(
  char* buffer,
  int length,
  const attach_accept_msg* msg)
{
  int index = 0;

  /* T3412 timer value (TBD) */
  /* TAI list (TBD) */

  /* GUTI */
  if (msg->presencemask & ATTACH_ACCEPT_GUTI_PRESENT) {
    index += epsIdentity(buffer + index, length - index, &msg->guti);
  }

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Attach Reject
 *-----------------------------------------------------------------------------
 */
static int
_attach_reject(
  char* buffer,
  int length,
  const attach_reject_msg* msg)
{
  int index = 0;

  /* EMM cause code */
  index += snprintf(buffer + index, length - index, "EmmCause = %s (%d)",
                    emmCauseCode(msg->emmcause), msg->emmcause);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Attach Complete
 *-----------------------------------------------------------------------------
 */
static int
_attach_complete(
  char* buffer,
  int length,
  const attach_complete_msg* msg)
{
  int index = 0;

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Identity Request
 *-----------------------------------------------------------------------------
 */
static int
_identity_request(
  char* buffer,
  int length,
  const identity_request_msg* msg)
{
  int index = 0;

  /* Type of requested identity */
  index += snprintf(buffer + index, length - index, "Type = %s",
                    identityType(&msg->identitytype));

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Detach Request
 *-----------------------------------------------------------------------------
 */
static int
_detach_request(
  char* buffer,
  int length,
  const detach_request_msg* msg)
{
  int index = 0;

  /* EPS attach type */
  index += snprintf(buffer + index, length - index, "Type = %s",
                    detachType(&msg->detachtype));
  /* NAS key set identifier */
  index += snprintf(buffer + index, length - index, ", KSI = ");
  index += nasKeySetIdentifier(buffer + index, length - index,
                               &msg->naskeysetidentifier);
  /* EPS mobile identity */
  index += snprintf(buffer + index, length - index, ", ");
  index += epsIdentity(buffer + index, length - index,
                       &msg->gutiorimsi);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Detach Accept
 *-----------------------------------------------------------------------------
 */
static int
_detach_accept(
  char* buffer,
  int length,
  const detach_accept_msg* msg)
{
  int index = 0;

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Identity Response
 *-----------------------------------------------------------------------------
 */
static int
_identity_response(
  char* buffer,
  int length,
  const identity_response_msg* msg)
{
  int index = 0;

  /* Mobile identity */
  index += mobileIdentity(buffer + index, length - index,
                          &msg->mobileidentity);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Authentication Request
 *-----------------------------------------------------------------------------
 */
static int
_authentication_request(
  char* buffer,
  int length,
  const authentication_request_msg* msg)
{
  int index = 0;

  /* NAS key set identifier ASME */
  index += snprintf(buffer + index, length - index, "KSIasme = ");
  index += nasKeySetIdentifier(buffer + index, length - index,
                               &msg->naskeysetidentifierasme);
  /* Authentication parameter RAND */
  index += snprintf(buffer + index, length - index, "\\nRAND = ");
  index += authenticationParameter(buffer + index, length - index,
                                   &msg->authenticationparameterrand.rand);
  /* Authentication parameter AUTN */
  index += snprintf(buffer + index, length - index, ", AUTN = ");
  index += authenticationParameter(buffer + index, length - index,
                                   &msg->authenticationparameterautn.autn);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Authentication Response
 *-----------------------------------------------------------------------------
 */
static int
_authentication_response(
  char* buffer,
  int length,
  const authentication_response_msg* msg)
{
  int index = 0;

  /* Authentication response parameter */
  index += snprintf(buffer + index, length - index, "RES = ");
  index += authenticationParameter(buffer + index, length - index,
                                   &msg->authenticationresponseparameter.res);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process Authentication Failure
 *-----------------------------------------------------------------------------
 */
static int
_authentication_failure(
  char* buffer,
  int length,
  const authentication_failure_msg* msg)
{
  int index = 0;

  /* EMM cause */
  index += snprintf(buffer + index, length - index, "EmmCause = %s (%d)",
                    emmCauseCode(msg->emmcause), msg->emmcause);

  /* Authenticattion failure parameter */
  if (msg->presencemask &
      AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT) {
    index += snprintf(buffer + index, length - index, "AUTS = ");
    index += authenticationParameter(buffer + index, length - index,
                                     &msg->authenticationfailureparameter.auts);
  }

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *          Process Authentication Reject
 *-----------------------------------------------------------------------------
 */
static int
_authentication_reject(
  char* buffer,
  int length,
  const authentication_reject_msg* msg)
{
  int index = 0;

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *          Process Security Mode Command
 *-----------------------------------------------------------------------------
 */
static int
_security_mode_command(
  char* buffer,
  int length,
  const security_mode_command_msg* msg)
{
  int index = 0;

  /* Selected NAS ciphering algorithm */
  index += snprintf(buffer + index, length - index, "%s (%d)",
                    nasCipheringAlgorithm(&msg->selectednassecurityalgorithms),
                    msg->selectednassecurityalgorithms.typeofcipheringalgorithm);

  /* Selected NAS integrity algorithm */
  index += snprintf(buffer + index, length - index, ", %s (%d)",
                    nasIntegrityAlgorithm(&msg->selectednassecurityalgorithms),
                    msg->selectednassecurityalgorithms.typeofintegrityalgorithm);

  /* NAS key set identifier */
  index += snprintf(buffer + index, length - index, ", KSI = ");
  index += nasKeySetIdentifier(buffer + index, length - index,
                               &msg->naskeysetidentifier);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *          Process Security Mode Complete
 *-----------------------------------------------------------------------------
 */
static int
_security_mode_complete(
  char* buffer,
  int length,
  const security_mode_complete_msg* msg)
{
  int index = 0;

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *          Process Security Mode Reject
 *-----------------------------------------------------------------------------
 */
static int
_security_mode_reject(
  char* buffer,
  int length,
  const security_mode_reject_msg* msg)
{
  int index = 0;

  /* EMM cause code */
  index += snprintf(buffer + index, length - index, "EmmCause = %s (%d)",
                    emmCauseCode(msg->emmcause), msg->emmcause);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        ESM message container
 *-----------------------------------------------------------------------------
 */
static int
_esm_message_container(
  char* buffer,
  int length,
  const EsmMessageContainer* msg)
{
  int index = 0;
  int bytes;

  if (msg->esmmessagecontainercontents.length > 0) {
    nas_message_t nas_msg;
    memset(&nas_msg, 0, sizeof(nas_message_t));

    /* Decode ESM message container */
    bytes = nas_message_decode((char*)msg->esmmessagecontainercontents.value,
                               &nas_msg, msg->esmmessagecontainercontents.length);

    if (bytes < 0) {
      printf("ERROR\t: %s - Failed to decode ESM message container "
             "(err=%d)\n", __FUNCTION__, bytes);
      return (0);
    }

    index += snprintf(buffer + index, length - index, "\\n + ");

    /* Process EPS Session Management NAS message */
    index += _nas_process_esm(buffer + index, length - index,
                              &nas_msg.plain.esm);
  }

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process EMM Status
 *-----------------------------------------------------------------------------
 */
static int
_emm_status(
  char* buffer,
  int length,
  const emm_status_msg* msg)
{
  int index = 0;

  /* EMM cause code */
  index += snprintf(buffer + index, length - index, "EmmCause = %s (%d)",
                    emmCauseCode(msg->emmcause), msg->emmcause);

  return (index);
}
/*
 * =============================================================================
 *     Functions used to process EPS Session Management messages
 * =============================================================================
 */

/*
 *-----------------------------------------------------------------------------
 *        Process EPS Session MAnagement NAS message
 *-----------------------------------------------------------------------------
 */
static int
_nas_process_esm(
  char* buffer,
  int length,
  const ESM_msg* msg)
{
  int index = 0;

  printf("INFO\t: Process %s\n", esmMsgType(msg->header.message_type));
  index += snprintf(buffer + index, length - index, "%s (pti=%d, ebi=%d",
                    esmMsgType(msg->header.message_type),
                    msg->header.procedure_transaction_identity,
                    msg->header.eps_bearer_identity);

  switch (msg->header.message_type) {
  case PDN_CONNECTIVITY_REQUEST:
    index += _pdn_connectivity_request(buffer + index, length - index,
                                       &msg->pdn_connectivity_request);
    break;

  case PDN_CONNECTIVITY_REJECT:
    index += _pdn_connectivity_reject(buffer + index, length - index,
                                      &msg->pdn_connectivity_reject);
    break;

  case PDN_DISCONNECT_REQUEST:
    index += _pdn_disconnect_request(buffer + index, length - index,
                                     &msg->pdn_disconnect_request);
    break;

  case PDN_DISCONNECT_REJECT:
    index += _pdn_disconnect_reject(buffer + index, length - index,
                                    &msg->pdn_disconnect_reject);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
    index += _activate_default_eps_bearer_context_request(
               buffer + index, length - index,
               &msg->activate_default_eps_bearer_context_request);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    index += _activate_default_eps_bearer_context_accept(
               buffer + index, length - index,
               &msg->activate_default_eps_bearer_context_accept);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    index += _activate_default_eps_bearer_context_reject(
               buffer + index, length - index,
               &msg->activate_default_eps_bearer_context_reject);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
    index += _activate_dedicated_eps_bearer_context_request(
               buffer + index, length - index,
               &msg->activate_dedicated_eps_bearer_context_request);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
    index += _activate_dedicated_eps_bearer_context_accept(
               buffer + index, length - index,
               &msg->activate_dedicated_eps_bearer_context_accept);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
    index += _activate_dedicated_eps_bearer_context_reject(
               buffer + index, length - index,
               &msg->activate_dedicated_eps_bearer_context_reject);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    index += _deactivate_eps_bearer_context_request(buffer + index,
             length - index,
             &msg->deactivate_eps_bearer_context_request);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
    index += _deactivate_eps_bearer_context_accept(buffer + index,
             length - index,
             &msg->deactivate_eps_bearer_context_accept);
    break;

  case ESM_STATUS:
    index += _esm_status(buffer + index, length - index,
                         &msg->esm_status);
    break;

  default:
    printf("WARNING\t: %s - ESM NAS message is not valid (0x%x)\n",
           __FUNCTION__, msg->header.message_type);
    break;
  }

  index += snprintf(buffer + index, length - index, ")");

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *                          Process PDN Connectivity Request
 *-----------------------------------------------------------------------------
 */
static int
_pdn_connectivity_request(
  char* buffer,
  int length,
  const pdn_connectivity_request_msg* msg)
{
  int index = 0;

  /* PDN request type and PDN type */
  index += snprintf(buffer + index, length - index,
                    ", RequestType = %s, PdnType = %s",
                    requestType(&msg->requesttype),
                    pdnType(&msg->pdntype));

  /* Access Point Name */
  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    index += snprintf(buffer + index, length - index, ", APN = %s",
                      (char*)msg->accesspointname.accesspointnamevalue.value);
  }

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *                          Process PDN Connectivity Reject
 *-----------------------------------------------------------------------------
 */
static int
_pdn_connectivity_reject(
  char* buffer,
  int length,
  const pdn_connectivity_reject_msg* msg)
{
  int index = 0;

  /* ESM cause code */
  index += snprintf(buffer + index, length - index, ", EsmCause = %s (%d)",
                    esmCauseCode(msg->esmcause), msg->esmcause);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *          Process PDN Disconnect Request
 *-----------------------------------------------------------------------------
 */
static int
_pdn_disconnect_request(
  char* buffer,
  int length,
  const pdn_disconnect_request_msg* msg)
{
  int index = 0;

  /* Linked EPS bearer identity */
  index += snprintf(buffer + index, length - index, ", Linked EBI = %d",
                    msg->linkedepsbeareridentity);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *                          Process PDN Disconnect Reject
 *-----------------------------------------------------------------------------
 */
static int
_pdn_disconnect_reject(
  char* buffer,
  int length,
  const pdn_disconnect_reject_msg* msg)
{
  int index = 0;

  /* ESM cause code */
  index += snprintf(buffer + index, length - index, ", EsmCause = %s (%d)",
                    esmCauseCode(msg->esmcause), msg->esmcause);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *              Process Activate Default EPS Bearer Context Request
 *-----------------------------------------------------------------------------
 */
static int
_activate_default_eps_bearer_context_request(
  char* buffer,
  int length,
  const activate_default_eps_bearer_context_request_msg* msg)
{
  int index = 0;

  /* Access Point Name */
  index += snprintf(buffer + index, length - index, ", APN = %s",
                    (char*)msg->accesspointname.accesspointnamevalue.value);

  /* PDN address */
  index += snprintf(buffer + index, length - index, ", PDN addr = ");
  index += pdnAddress(buffer + index, length - index, &msg->pdnaddress);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *    Process Activate Default EPS Bearer Context Accept
 *-----------------------------------------------------------------------------
 */
static int
_activate_default_eps_bearer_context_accept(
  char* buffer,
  int length,
  const activate_default_eps_bearer_context_accept_msg* msg)
{
  int index = 0;

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *    Process Activate Default EPS Bearer Context Reject
 *-----------------------------------------------------------------------------
 */
static int
_activate_default_eps_bearer_context_reject(
  char* buffer,
  int length,
  const activate_default_eps_bearer_context_reject_msg* msg)
{
  int index = 0;

  /* ESM cause code */
  index += snprintf(buffer + index, length - index, ", EsmCause = %s (%d)",
                    esmCauseCode(msg->esmcause), msg->esmcause);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *    Process Activate Dedicated EPS Bearer Context Request
 *-----------------------------------------------------------------------------
 */
static int
_activate_dedicated_eps_bearer_context_request(
  char* buffer,
  int length,
  const activate_dedicated_eps_bearer_context_request_msg* msg)
{
  int index = 0;

  /* Linked EPS bearer identity */
  index += snprintf(buffer + index, length - index, ", Linked EBI = %d",
                    msg->linkedepsbeareridentity);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *    Process Activate Dedicated EPS Bearer Context Accept
 *-----------------------------------------------------------------------------
 */
static int
_activate_dedicated_eps_bearer_context_accept(
  char* buffer,
  int length,
  const activate_dedicated_eps_bearer_context_accept_msg* msg)
{
  int index = 0;

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *    Process Activate Dedicated EPS Bearer Context Reject
 *-----------------------------------------------------------------------------
 */
static int
_activate_dedicated_eps_bearer_context_reject(
  char* buffer,
  int length,
  const activate_dedicated_eps_bearer_context_reject_msg* msg)
{
  int index = 0;

  /* ESM cause code */
  index += snprintf(buffer + index, length - index, ", EsmCause = %s (%d)",
                    esmCauseCode(msg->esmcause), msg->esmcause);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *    Process Deactivate EPS Bearer Context Request
 *-----------------------------------------------------------------------------
 */
static int _deactivate_eps_bearer_context_request(
  char* buffer,
  int length,
  const deactivate_eps_bearer_context_request_msg* msg)
{
  int index = 0;

  /* ESM cause code */
  index += snprintf(buffer + index, length - index, ", EsmCause = %s (%d)",
                    esmCauseCode(msg->esmcause), msg->esmcause);

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *    Process Deactivate EPS Bearer Context Accept
 *-----------------------------------------------------------------------------
 */
static int
_deactivate_eps_bearer_context_accept(
  char* buffer,
  int length,
  const deactivate_eps_bearer_context_accept_msg* msg)
{
  int index = 0;

  return (index);
}

/*
 *-----------------------------------------------------------------------------
 *        Process ESM Status
 *-----------------------------------------------------------------------------
 */
static int
_esm_status(
  char* buffer,
  int length,
  const esm_status_msg* msg)
{
  int index = 0;

  /* ESM cause code */
  index += snprintf(buffer + index, length - index, ", EsmCause = %s (%d)",
                    esmCauseCode(msg->esmcause), msg->esmcause);

  return (index);
}

