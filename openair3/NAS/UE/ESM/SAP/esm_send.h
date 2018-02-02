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
Source      esm_send.h

Version     0.1

Date        2013/02/11

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions executed at the ESM Service Access
        Point to send EPS Session Management messages to the
        EPS Mobility Management sublayer.

*****************************************************************************/
#ifndef __ESM_SEND_H__
#define __ESM_SEND_H__

#include "EsmStatus.h"


#include "PdnConnectivityRequest.h"
#include "PdnDisconnectRequest.h"
#include "BearerResourceAllocationRequest.h"
#include "BearerResourceModificationRequest.h"

#include "ActivateDefaultEpsBearerContextAccept.h"
#include "ActivateDefaultEpsBearerContextReject.h"
#include "ActivateDedicatedEpsBearerContextAccept.h"
#include "ActivateDedicatedEpsBearerContextReject.h"
#include "ModifyEpsBearerContextAccept.h"
#include "ModifyEpsBearerContextReject.h"
#include "DeactivateEpsBearerContextAccept.h"

#include "EsmInformationResponse.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * Functions executed by both the UE and the MME to send ESM messages
 * --------------------------------------------------------------------------
 */
int esm_send_status(int pti, int ebi, esm_status_msg *msg, int esm_cause);

/*
 * --------------------------------------------------------------------------
 * Functions executed by the UE to send ESM message to the network
 * --------------------------------------------------------------------------
 */
/*
 * Transaction related messages
 * ----------------------------
 */
int esm_send_pdn_connectivity_request(int pti, int is_emergency, int pdn_type,
                                      const char *apn, pdn_connectivity_request_msg *msg);
int esm_send_pdn_disconnect_request(int pti, int ebi,
                                    pdn_disconnect_request_msg *msg);

/*
 * Messages related to EPS bearer contexts
 * ---------------------------------------
 */
int esm_send_activate_default_eps_bearer_context_accept(int ebi,
    activate_default_eps_bearer_context_accept_msg *msg);
int esm_send_activate_default_eps_bearer_context_reject(int ebi,
    activate_default_eps_bearer_context_reject_msg *msg, int esm_cause);

int esm_send_activate_dedicated_eps_bearer_context_accept(int ebi,
    activate_dedicated_eps_bearer_context_accept_msg *msg);
int esm_send_activate_dedicated_eps_bearer_context_reject(int ebi,
    activate_dedicated_eps_bearer_context_reject_msg *msg, int esm_cause);

int esm_send_deactivate_eps_bearer_context_accept(int ebi,
    deactivate_eps_bearer_context_accept_msg *msg);


#endif /* __ESM_SEND_H__*/
