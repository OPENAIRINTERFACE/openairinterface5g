/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
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
