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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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
