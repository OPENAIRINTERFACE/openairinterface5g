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
Source    esm_msg.h

Version   0.1

Date    2012/09/27

Product   NAS stack

Subsystem EPS Session Management

Author    Frederic Maurel

Description Defines EPS Session Management messages and functions used
    to encode and decode

*****************************************************************************/
#ifndef __ESM_MSG_H__
#define __ESM_MSG_H__

#include "esm_msgDef.h"

#include "ActivateDedicatedEpsBearerContextRequest.h"
#include "ActivateDedicatedEpsBearerContextAccept.h"
#include "ActivateDedicatedEpsBearerContextReject.h"
#include "ActivateDefaultEpsBearerContextRequest.h"
#include "ActivateDefaultEpsBearerContextAccept.h"
#include "ActivateDefaultEpsBearerContextReject.h"
#include "ModifyEpsBearerContextRequest.h"
#include "ModifyEpsBearerContextAccept.h"
#include "ModifyEpsBearerContextReject.h"
#include "DeactivateEpsBearerContextRequest.h"
#include "DeactivateEpsBearerContextAccept.h"
#include "PdnDisconnectRequest.h"
#include "PdnDisconnectReject.h"
#include "PdnConnectivityRequest.h"
#include "PdnConnectivityReject.h"
#include "BearerResourceAllocationRequest.h"
#include "BearerResourceAllocationReject.h"
#include "BearerResourceModificationRequest.h"
#include "BearerResourceModificationReject.h"
#include "EsmInformationRequest.h"
#include "EsmInformationResponse.h"
#include "EsmStatus.h"

#include <stdint.h>

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Structure of ESM plain NAS message
 * ----------------------------------
 */
typedef union {
  esm_msg_header_t header;
  activate_default_eps_bearer_context_request_msg activate_default_eps_bearer_context_request;
  activate_default_eps_bearer_context_accept_msg activate_default_eps_bearer_context_accept;
  activate_default_eps_bearer_context_reject_msg activate_default_eps_bearer_context_reject;
  activate_dedicated_eps_bearer_context_request_msg activate_dedicated_eps_bearer_context_request;
  activate_dedicated_eps_bearer_context_accept_msg activate_dedicated_eps_bearer_context_accept;
  activate_dedicated_eps_bearer_context_reject_msg activate_dedicated_eps_bearer_context_reject;
  modify_eps_bearer_context_request_msg modify_eps_bearer_context_request;
  modify_eps_bearer_context_accept_msg modify_eps_bearer_context_accept;
  modify_eps_bearer_context_reject_msg modify_eps_bearer_context_reject;
  deactivate_eps_bearer_context_request_msg deactivate_eps_bearer_context_request;
  deactivate_eps_bearer_context_accept_msg deactivate_eps_bearer_context_accept;
  pdn_connectivity_request_msg pdn_connectivity_request;
  pdn_connectivity_reject_msg pdn_connectivity_reject;
  pdn_disconnect_request_msg pdn_disconnect_request;
  pdn_disconnect_reject_msg pdn_disconnect_reject;
  bearer_resource_allocation_request_msg bearer_resource_allocation_request;
  bearer_resource_allocation_reject_msg bearer_resource_allocation_reject;
  bearer_resource_modification_request_msg bearer_resource_modification_request;
  bearer_resource_modification_reject_msg bearer_resource_modification_reject;
  esm_information_request_msg esm_information_request;
  esm_information_response_msg esm_information_response;
  esm_status_msg esm_status;
} ESM_msg;


/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int esm_msg_decode(ESM_msg *msg, uint8_t *buffer, uint32_t len);

int esm_msg_encode(ESM_msg *msg, uint8_t *buffer, uint32_t len);

#endif /* __ESM_MSG_H__ */
