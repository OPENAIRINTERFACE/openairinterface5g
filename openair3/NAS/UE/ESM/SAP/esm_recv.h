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
Source      esm_recv.h

Version     0.1

Date        2013/02/06

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions executed at the ESM Service Access
        Point upon receiving EPS Session Management messages
        from the EPS Mobility Management sublayer.

*****************************************************************************/
#ifndef __ESM_RECV_H__
#define __ESM_RECV_H__

#include "EsmStatus.h"
#include "emmData.h"
#include "user_defs.h"

#include "PdnConnectivityReject.h"
#include "PdnDisconnectReject.h"
#include "BearerResourceAllocationReject.h"
#include "BearerResourceModificationReject.h"

#include "ActivateDefaultEpsBearerContextRequest.h"
#include "ActivateDedicatedEpsBearerContextRequest.h"
#include "ModifyEpsBearerContextRequest.h"
#include "DeactivateEpsBearerContextRequest.h"

#include "EsmInformationRequest.h"

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
 * Functions executed by both the UE and the MME upon receiving ESM messages
 * --------------------------------------------------------------------------
 */
int esm_recv_status(int pti, int ebi, const esm_status_msg *msg);


/*
 * --------------------------------------------------------------------------
 * Functions executed by the UE upon receiving ESM message from the network
 * --------------------------------------------------------------------------
 */
/*
 * Transaction related messages
 * ----------------------------
 */
int esm_recv_pdn_connectivity_reject(nas_user_t *user, int pti, int ebi,
                                     const pdn_connectivity_reject_msg *msg);

int esm_recv_pdn_disconnect_reject(nas_user_t *user, int pti, int ebi,
                                   const pdn_disconnect_reject_msg *msg);

/*
 * Messages related to EPS bearer contexts
 * ---------------------------------------
 */
int esm_recv_activate_default_eps_bearer_context_request(nas_user_t *user, int pti, int ebi,
    const activate_default_eps_bearer_context_request_msg *msg);

int esm_recv_activate_dedicated_eps_bearer_context_request(nas_user_t *user, int pti, int ebi,
    const activate_dedicated_eps_bearer_context_request_msg *msg);

int esm_recv_deactivate_eps_bearer_context_request(nas_user_t *user, int pti, int ebi,
    const deactivate_eps_bearer_context_request_msg *msg);


#endif /* __ESM_RECV_H__*/
